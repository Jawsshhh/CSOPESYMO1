#include "RoundRobin.h"
#include <chrono>
#include <iostream>
#include <cmath>
#include <algorithm>

RRScheduler::RRScheduler(int numCores,
    int quantum,
    int delays_per_exec,
    size_t maxMemory,
    size_t frameSize)
    : Scheduler(numCores, maxMemory, frameSize, "csopesy-backing-store.txt"),
    quantum(quantum),
    delays_per_exec(delays_per_exec),
    frameSize(frameSize)
{
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
    if (tryAllocateProcessPages(process)) {
        readyQueue.push(process);
        cv.notify_all();
    }
    else {
        pendingProcesses.push(process);
    }
}

bool RRScheduler::tryAllocateProcessPages(std::shared_ptr<Process> process) {
    size_t memoryRequired = process->getMemoryNeeded();
    size_t frameSize = memoryManager.getFrameSize();
    int pagesNeeded = (memoryRequired + frameSize - 1) / frameSize;

    try {
        std::vector<int> pageIds;
        pageIds.reserve(pagesNeeded);
        for (int i = 0; i < pagesNeeded; ++i) {
            int pid = memoryManager.getNextGlobalPageId();
            pageIds.push_back(pid);
        }
        // initialize and register
        for (int pid : pageIds) {
            memoryManager.initializePageData(
                pid,
                "Process_" + process->getName() + "_Page_" + std::to_string(pid)
            );
        }
        process->assignPages(pageIds);
        memoryManager.registerProcessPages(process->getId(), pageIds);
        return true;
    }
    catch (const std::runtime_error&) {
        return false;
    }
}


void RRScheduler::schedulerLoop() {
    while (running) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            // wake up sleeping processes
            for (auto it = sleepingProcesses.begin(); it != sleepingProcesses.end(); ) {
                auto& p = *it;
                p->executeNextInstruction();
                if (!p->getIsSleeping()) {
                    readyQueue.push(p);
                    it = sleepingProcesses.erase(it);
                }
                else {
                    ++it;
                }
            }
            // retry pending for memory
            std::queue<std::shared_ptr<Process>> still;
            while (!pendingProcesses.empty()) {
                auto p = pendingProcesses.front(); pendingProcesses.pop();
                if (tryAllocateProcessPages(p))
                    readyQueue.push(p);
                else
                    still.push(p);
            }
            pendingProcesses = std::move(still);
            if (!readyQueue.empty()) cv.notify_all();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void RRScheduler::workerLoop(int coreId) {
    while (running) {
        totalCpuTicks.fetch_add(1);

        // wait for our turn
        bool gotTurn = false;
        {
            std::unique_lock<std::mutex> turnLock(coreTurnMutex);
            gotTurn = cv.wait_for(turnLock,
                std::chrono::milliseconds(100),
                [&] { return !running.load() || coreId == nextCoreId; });
        }
        if (!running) break;
        if (!gotTurn) {
            std::lock_guard<std::mutex> g(coreTurnMutex);
            nextCoreId = (nextCoreId + 1) % coreAvailable.size();
            cv.notify_all();
            idleCpuTicks.fetch_add(1);
            continue;
        }

        std::shared_ptr<Process> proc = nullptr;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!readyQueue.empty()) {
                proc = readyQueue.front(); readyQueue.pop();
                proc->setAssignedCore(coreId);
                coreAvailable[coreId] = false;
                processHandler.insertProcess(proc);
            }
        }
        if (!proc) {
            std::lock_guard<std::mutex> g(coreTurnMutex);
            nextCoreId = (nextCoreId + 1) % coreAvailable.size();
            cv.notify_all();
            idleCpuTicks.fetch_add(1);
            continue;
        }

        bool finished = false;
        int executed = 0;
        try {
            while (executed < quantum &&
                proc->getCurrentInstructionIndex() < proc->getInstructionCount())
            {
                activeCpuTicks.fetch_add(1);
                // page-in if needed
                const auto& pages = proc->getAssignedPages();
                if (!pages.empty()) {
                    int pg = pages[proc->getCurrentInstructionIndex() / frameSize];
                    while (!memoryManager.accessPage(pg)) {
                        memoryManager.pageFault(pg);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }

                proc->executeNextInstruction();
                ++executed;
                if (proc->getIsSleeping()) break;
                if (delays_per_exec > 0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
            }
            // detect normal completion
            if (proc->getCurrentInstructionIndex() >= proc->getInstructionCount())
                finished = true;
        }
        catch (...) {
            finished = true;
        }

        // sleeping?
        if (proc->getIsSleeping()) {
            std::lock_guard<std::mutex> lock(queueMutex);
            coreAvailable[coreId] = true;
            sleepingProcesses.push_back(proc);
            cv.notify_all();
            continue;
        }

        // if done, teardown
        if (finished) {
            std::lock_guard<std::mutex> lock(queueMutex);
            proc->setIsFinished(true);
            processHandler.markProcessFinished(proc->getId());
            memoryManager.releaseProcessPages(proc->getId());
            coreAvailable[coreId] = true;
        }
        // else requeue for next round
        else {
            std::lock_guard<std::mutex> lock(queueMutex);
            readyQueue.push(proc);
            coreAvailable[coreId] = true;
        }

        // advance turn
        {
            std::lock_guard<std::mutex> g(coreTurnMutex);
            nextCoreId = (nextCoreId + 1) % coreAvailable.size();
        }
        cv.notify_all();
    }
}