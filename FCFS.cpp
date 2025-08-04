#include "FCFS.h"
#include <chrono>
#include <iostream>


FCFSScheduler::FCFSScheduler(int numCores, size_t maxMemory, size_t frameSize, int delays_per_exec)
    : Scheduler(numCores, maxMemory, frameSize, "csopesy-backing-store.txt"), delays_per_exec(delays_per_exec) {

    for (int i = 0; i < numCores; ++i) {
        coreAvailable[i] = true;
        workerThreads.emplace_back(&FCFSScheduler::workerLoop, this, i);
    }
    schedulerThread = std::thread(&FCFSScheduler::schedulerLoop, this);
}


FCFSScheduler::~FCFSScheduler() {
    stop();
}

void FCFSScheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(queueMutex);

    if (allocateMemoryForProcess(process)) {
        processQueue.push(process);
        processHandler.insertProcess(process);
    }
    else {
        std::lock_guard<std::mutex> waitLock(waitingQueueMutex);
        waitingQueue.push(process);
    }

    cv.notify_one();
}

void FCFSScheduler::schedulerLoop() {
    while (running) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            auto it = sleepingProcesses.begin();
            while (it != sleepingProcesses.end()) {
                auto proc = *it;
                proc->executeNextInstruction();
                if (!proc->getIsSleeping()) {
                    // Move back into ready queue
                    processQueue.push(proc);
                    it = sleepingProcesses.erase(it);
                }
                else {
                    ++it;
                }
            }

            std::lock_guard<std::mutex> waitLock(waitingQueueMutex);
            std::queue<std::shared_ptr<Process>> tempQueue;
            while (!waitingQueue.empty()) {
                auto process = waitingQueue.front();
                waitingQueue.pop();
                if (allocateMemoryForProcess(process)) {
                    processQueue.push(process);
                    processHandler.insertProcess(process);
                }
                else {
                    tempQueue.push(process);
                }
            }
            waitingQueue = tempQueue;
        }

        std::unique_lock<std::mutex> lock(queueMutex);
        if (processQueue.empty()) {
            cv.wait(lock);
            continue;
        }

      
        int availableCore = findAvailableCore();
        if (availableCore == -1) {
            cv.wait(lock);
            continue;
        }

     
        auto process = processQueue.front();
        processQueue.pop();
        process->setAssignedCore(availableCore);
        coreAvailable[availableCore] = false;
        runningProcesses.push_back(process);

        lock.unlock();
        cv.notify_all();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void FCFSScheduler::workerLoop(int coreId) {
    while (running) {
        totalCpuTicks.fetch_add(1);
        std::shared_ptr<Process> currentProcess = nullptr;

        {   
            std::lock_guard<std::mutex> lock(queueMutex);
            auto it = std::find_if(runningProcesses.begin(),
                runningProcesses.end(),
                [coreId](const auto& p) {
                    return p->getAssignedCore() == coreId
                        && !p->getIsFinished()
                        && !p->getIsSleeping();
                });
            if (it != runningProcesses.end()) {
                currentProcess = *it;
            }
        }

        if (!currentProcess) {
            idleCpuTicks.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        bool canExecute = handleMemoryAccess(currentProcess);
        if (!canExecute) {
            idleCpuTicks.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
        activeCpuTicks.fetch_add(1);
        bool nextInstruction = currentProcess->executeNextInstruction();

        if (currentProcess->getIsSleeping()) {
            std::lock_guard<std::mutex> lock(queueMutex);
            coreAvailable[coreId] = true;
            
            auto it = std::find(runningProcesses.begin(), runningProcesses.end(), currentProcess);
            if (it != runningProcesses.end()) {
                runningProcesses.erase(it);
            }
            
            sleepingProcesses.push_back(currentProcess);
            cv.notify_all();
            continue;
        }

        if (!nextInstruction) {
            std::lock_guard<std::mutex> lock(queueMutex);
            currentProcess->setIsFinished(true);
            coreAvailable[coreId] = true;

            auto it = std::find(runningProcesses.begin(), runningProcesses.end(), currentProcess);
            if (it != runningProcesses.end()) {
                finishedProcesses.push_back(*it);
                runningProcesses.erase(it);
            }

            processHandler.markProcessFinished(currentProcess->getId());
            deallocateMemoryForProcess(currentProcess);
            cv.notify_all();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool FCFSScheduler::handleMemoryAccess(std::shared_ptr<Process> process) {
    const std::vector<int>& pages = process->getAssignedPages();
    if (pages.empty()) {
        return true;
    }

    int currentInstruction = process->getCurrentInstructionIndex();
    int pageIndex = currentInstruction % pages.size();
    int globalPageId = pages[pageIndex];

    if (memoryManager.accessPage(globalPageId)) {
        return true; 
    }

    bool pageFaultHandled = false;
    int maxRetries = 5;

    for (int retry = 0; retry < maxRetries && !pageFaultHandled; retry++) {
        pageFaultHandled = memoryManager.pageFault(globalPageId);

        if (!pageFaultHandled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    if (!pageFaultHandled) {
      
        return false;
    }

    return true;
}

bool FCFSScheduler::allocateMemoryForProcess(std::shared_ptr<Process> process)
{
    size_t memoryRequired = process->getMemoryNeeded();
    size_t frameSize = memoryManager.getFrameSize();
    int pagesNeeded = (memoryRequired + frameSize - 1) / frameSize; 

    try {
        std::vector<int> pageIds;
        for (int i = 0; i < pagesNeeded; ++i) {
            int pageId = memoryManager.getNextGlobalPageId();
            pageIds.push_back(pageId);
            
            std::string pageData = "Process_" + process->getName() + "_Page_" + std::to_string(i);
            memoryManager.initializePageData(pageId, pageData);
        }

        process->assignPages(pageIds);
        memoryManager.registerProcessPages(process->getId(), pageIds);

        return true;
    }
    catch (const std::runtime_error& e) {
        return false;
    }
}

void FCFSScheduler::deallocateMemoryForProcess(std::shared_ptr<Process> process)
{
    const std::vector<int>& pages = process->getAssignedPages();
    memoryManager.releaseProcessPages(process->getId());
}



