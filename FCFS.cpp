#include "FCFS.h"
#include <chrono>
#include <iostream>


#include "FCFS.h"
#include <chrono>
#include <iostream>

FCFSScheduler::FCFSScheduler(int numCores,
    size_t maxMemory,
    size_t frameSize,
    int delays_per_exec)
    : Scheduler(numCores, maxMemory, frameSize, "csopesy-backing-store.txt"),
    delays_per_exec(delays_per_exec)
{
    running = true;
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
            // wake up sleeping processes
            for (auto it = sleepingProcesses.begin(); it != sleepingProcesses.end(); ) {
                auto proc = *it;
                proc->executeNextInstruction();
                if (!proc->getIsSleeping()) {
                    processQueue.push(proc);
                    it = sleepingProcesses.erase(it);
                }
                else {
                    ++it;
                }
            }
            // retry waitingQueue for memory
            std::lock_guard<std::mutex> waitLock(waitingQueueMutex);
            std::queue<std::shared_ptr<Process>> temp;
            while (!waitingQueue.empty()) {
                auto p = waitingQueue.front(); waitingQueue.pop();
                if (allocateMemoryForProcess(p)) {
                    processQueue.push(p);
                    processHandler.insertProcess(p);
                }
                else {
                    temp.push(p);
                }
            }
            waitingQueue = std::move(temp);
        }

        std::unique_lock<std::mutex> lock(queueMutex);
        if (processQueue.empty()) {
            cv.wait(lock);
            continue;
        }

        int core = findAvailableCore();
        if (core == -1) {
            cv.wait(lock);
            continue;
        }

        auto proc = processQueue.front(); processQueue.pop();
        proc->setAssignedCore(core);
        coreAvailable[core] = false;
        runningProcesses.push_back(proc);

        lock.unlock();
        cv.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void FCFSScheduler::workerLoop(int coreId) {
    while (running) {
        totalCpuTicks.fetch_add(1);
        std::shared_ptr<Process> proc = nullptr;

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            auto it = std::find_if(runningProcesses.begin(),
                runningProcesses.end(),
                [coreId](auto& p) {
                    return p->getAssignedCore() == coreId
                        && !p->getIsFinished()
                        && !p->getIsSleeping();
                });
            if (it != runningProcesses.end())
                proc = *it;
        }

        if (!proc) {
            idleCpuTicks.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (!handleMemoryAccess(proc)) {
            idleCpuTicks.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
        activeCpuTicks.fetch_add(1);

        bool hasMore = proc->executeNextInstruction();

        // if it went to sleep, pull it out of running
        if (proc->getIsSleeping()) {
            std::lock_guard<std::mutex> lock(queueMutex);
            coreAvailable[coreId] = true;
            runningProcesses.erase(
                std::remove(runningProcesses.begin(), runningProcesses.end(), proc),
                runningProcesses.end());
            sleepingProcesses.push_back(proc);
            cv.notify_all();
            continue;
        }

        // if no more instructions, finish it
        if (!hasMore || proc->getCurrentInstructionIndex() >= proc->getInstructionCount()) {
            std::lock_guard<std::mutex> lock(queueMutex);
            proc->setIsFinished(true);
            coreAvailable[coreId] = true;
            runningProcesses.erase(
                std::remove(runningProcesses.begin(), runningProcesses.end(), proc),
                runningProcesses.end());
            finishedProcesses.push_back(proc);
            processHandler.markProcessFinished(proc->getId());
            deallocateMemoryForProcess(proc);
            cv.notify_all();
            continue;
        }

        // otherwise loop
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

    while (!memoryManager.pageFault(globalPageId)) {
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



