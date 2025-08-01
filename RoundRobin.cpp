#include "RoundRobin.h"
#include <chrono>
#include <iostream>
#include <cmath>         // For page calculation
#include <algorithm>     // For std::max

RRScheduler::RRScheduler(int numCores, int quantum, int delays_per_exec,
    size_t maxMemory, size_t frameSize)
    : Scheduler(numCores, maxMemory, frameSize),
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

    size_t memoryNeeded = process->getMemoryNeeded();
    int pagesNeeded = static_cast<int>((memoryNeeded + frameSize - 1) / frameSize);

    std::vector<int> assignedPages;

    try {
        for (int i = 0; i < pagesNeeded; ++i) {
            int pageId = memoryManager.getNextGlobalPageId();
            assignedPages.push_back(pageId);
            memoryManager.initializePageData(pageId, "Init data for page " + std::to_string(pageId));
        }

        memoryManager.registerProcessPages(process->getId(), assignedPages);
        process->assignPages(assignedPages);
        readyQueue.push(process);
        cv.notify_one();

    }
    catch (const std::runtime_error& e) {
      //  std::cerr << "[RRScheduler::addProcess] " << e.what() << "\n";
    }
}






void RRScheduler::schedulerLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this]() { return !readyQueue.empty() || !running; });
        if (!running) break;

        cv.notify_all(); // Wake up all workers
        lock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void RRScheduler::workerLoop(int coreId) {
    while (running) {
        std::shared_ptr<Process> process = nullptr;

        // === Wait for your core's turn BEFORE grabbing the queue ===
        {
            std::unique_lock<std::mutex> turnLock(coreTurnMutex);
            while (coreId != nextCoreId && running) {
                cv.wait(turnLock);
            }
            if (!running) break;
        }

        // === Now grab a process safely ===
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (readyQueue.empty()) {
                continue;  // nothing to do — loop and wait again
            }

            process = readyQueue.front();
            readyQueue.pop();

            process->setAssignedCore(coreId);
            coreAvailable[coreId] = false;
            processHandler.insertProcess(process);
        }

        if (process) {
            unsigned cyclesUsed = 0;
            const auto& assignedPages = process->getAssignedPages();

            while (cyclesUsed < quantum && !process->isFinished() && running) {
                int instructionIndex = process->getCurrentInstructionIndex();
                int pageLocal = instructionIndex / static_cast<int>(frameSize);

                if (pageLocal >= assignedPages.size()) {
                   
                    break;
                }

                int globalPage = assignedPages[pageLocal];

                if (!memoryManager.accessPage(globalPage)) {
                    if (!memoryManager.pageFault(globalPage)) {
                        break;
                    }
                }

                process->executeNextInstruction();
                ++cyclesUsed;
                std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);

                if (process->isFinished()) {
                    processHandler.markProcessFinished(process->getId());
                }
                else {
                    readyQueue.push(process);
                }

                coreAvailable[coreId] = true;
            }

            // === Rotate turn ===
            {
                std::lock_guard<std::mutex> turnLock(coreTurnMutex);
                nextCoreId = (nextCoreId + 1) % coreAvailable.size();
            }

            cv.notify_all();

            if (coreId == 0 && cyclesUsed > 0) {
                std::lock_guard<std::mutex> snapLock(snapshotMutex);
                std::string filename = "memory_stamp_" + std::to_string(quantumCycle++) + ".txt";
               // memoryManager.generateSnapshot(filename, quantumCycle);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
