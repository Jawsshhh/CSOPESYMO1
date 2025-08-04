#include "RoundRobin.h"
#include <chrono>
#include <iostream>
#include <cmath>
#include <algorithm>

RRScheduler::RRScheduler(int numCores, int quantum, int delays_per_exec,
    size_t maxMemory, size_t frameSize)
    : Scheduler(numCores, maxMemory, frameSize, "csopesy-backing-store.txt"),
    quantum(quantum),
    delays_per_exec(delays_per_exec),
    frameSize(frameSize) {

    for (int i = 0; i < numCores; ++i) {
        coreAvailable[i] = true;
        workerThreads.emplace_back(&RRScheduler::workerLoop, this, i);
    }
    schedulerThread = std::thread(&RRScheduler::schedulerLoop, this);
}

RRScheduler::~RRScheduler() {
    stop();
}

void RRScheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(queueMutex);

    // Try to allocate pages with retry mechanism
    if (tryAllocateProcessPages(process)) {
        readyQueue.push(process);
        cv.notify_all();
        //std::cout << "[INFO] Process " << process->getId() << " added to ready queue" << std::endl;
    }
    else {
        // Add to pending queue for retry
        pendingProcesses.push(process);
        //std::cout << "[WARN] Process " << process->getId() << " queued for retry (memory allocation failed)" << std::endl;
    }
}

bool RRScheduler::tryAllocateProcessPages(std::shared_ptr<Process> process) {
    size_t memoryNeeded = process->getMemoryNeeded();
    int pagesNeeded = static_cast<int>((memoryNeeded + frameSize - 1) / frameSize);

    std::vector<int> assignedPages;

    try {
        // Try to allocate all pages first
        for (int i = 0; i < pagesNeeded; ++i) {
            int pageId = memoryManager.getNextGlobalPageId();
            assignedPages.push_back(pageId);
        }

        // Initialize page data for all allocated pages
        for (int i = 0; i < assignedPages.size(); ++i) {
            int pageId = assignedPages[i];
            memoryManager.initializePageData(pageId, "Init data for page " + std::to_string(pageId));
        }

        // Register all pages with the process
        memoryManager.registerProcessPages(process->getId(), assignedPages);
        process->assignPages(assignedPages);

        //std::cout << "[INFO] Successfully allocated " << pagesNeeded << " pages for process " << process->getId() << std::endl;
        return true;

    }
    catch (const std::runtime_error& e) {
        // Clean up any partially allocated pages
        for (int pageId : assignedPages) {
            try {
                memoryManager.releasePage(pageId);
            }
            catch (...) {
                // Ignore cleanup errors
            }
        }

        
        return false;
    }
}

void RRScheduler::schedulerLoop() {
    while (running) {
       
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            auto it = sleepingProcesses.begin();
            while (it != sleepingProcesses.end()) {
                auto& proc = *it;
                proc->executeNextInstruction();
                if (!proc->getIsSleeping()) {
                    readyQueue.push(proc);
                    it = sleepingProcesses.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            std::queue<std::shared_ptr<Process>> still;
            while (!pendingProcesses.empty()) {
                auto p = pendingProcesses.front(); pendingProcesses.pop();
                if (tryAllocateProcessPages(p))
                    readyQueue.push(p);
                else
                    still.push(p);
            }
            pendingProcesses = std::move(still);
            if (!readyQueue.empty())
                cv.notify_all();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void RRScheduler::workerLoop(int coreId) {
    while (running) {
        totalCpuTicks.fetch_add(1);

        bool gotTurn = false;
        {
            std::unique_lock<std::mutex> turnLock(coreTurnMutex);
            gotTurn = cv.wait_for(turnLock,
                std::chrono::milliseconds(100),
                [&] { return !running.load() || coreId == nextCoreId; });
        }

        if (!running) {
            break;
        }

        if (!gotTurn) {
            {
                std::lock_guard<std::mutex> guard(coreTurnMutex);
                nextCoreId = (nextCoreId + 1) % coreAvailable.size();
            }
            cv.notify_all();
            idleCpuTicks.fetch_add(1);
            continue;
        }
        
        std::shared_ptr<Process> process = nullptr;
        {
            std::lock_guard<std::mutex> lg(queueMutex);
            if (!readyQueue.empty()) {
                process = readyQueue.front();
                readyQueue.pop();
                process->setAssignedCore(coreId);
                coreAvailable[coreId] = false;
                processHandler.insertProcess(process);
            }
        }

        if (!process) {
            {
                std::lock_guard<std::mutex> gl(coreTurnMutex);
                nextCoreId = (nextCoreId + 1) % coreAvailable.size();
            }
            cv.notify_all();
            idleCpuTicks.fetch_add(1);
            continue;
        }

      
        bool processFinished = false;
        bool requeueProcess = true;
        int instructionsExecuted = 0;

        try {
            while (instructionsExecuted < quantum &&
                   process->getCurrentInstructionIndex() < process->getInstructionCount())
            {
                activeCpuTicks.fetch_add(1);

                // Ensure page loaded
                const auto& pages = process->getAssignedPages();
                if (!pages.empty()) {
                    int pg = pages[process->getCurrentInstructionIndex() / frameSize];
                    bool ok = memoryManager.accessPage(pg);
                    if (!ok) {
                        if (!memoryManager.pageFault(pg))
                            break;  // unable to fault in
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }

                process->executeNextInstruction();
                ++instructionsExecuted;
                if (process->getIsSleeping())
                    break;
                if (delays_per_exec > 0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
            }
            requeueProcess = process->getCurrentInstructionIndex() >= process->getInstructionCount();
        }
        catch (...) {
            processFinished = true;
            requeueProcess = false;
        }

        if (process->getIsSleeping()) {
            std::lock_guard<std::mutex> lg(queueMutex);
            coreAvailable[coreId] = true;
            sleepingProcesses.push_back(process);
            cv.notify_all();
            continue;
        }

        // Handle process completion
        if (processFinished) {
            {
                std::lock_guard<std::mutex> lg(queueMutex);
                process->setIsFinished(true);
                processHandler.markProcessFinished(process->getId());
                memoryManager.releaseProcessPages(process->getId());
                requeueProcess = false;
            }
        }

        // Requeue the process if it's not finished and quantum expired
        if (requeueProcess && !processFinished) {
            std::lock_guard<std::mutex> lock(queueMutex);
            readyQueue.push(process);
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            coreAvailable[coreId] = true;
        }

        {
            std::lock_guard<std::mutex> turnLock(coreTurnMutex);
            nextCoreId = (nextCoreId + 1) % coreAvailable.size();
           
        }
        cv.notify_all();
    }
}