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
    for (int i = 0; i < pagesNeeded; ++i) {
        assignedPages.push_back(globalPageCounter++);
    }

    process->assignPages(assignedPages);
    memoryManager.registerProcessPages(process->getId(), assignedPages); 
    readyQueue.push(process);
    cv.notify_one();
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
    constexpr int estimatedInstructionSize = 4;

    while (running) {
        std::shared_ptr<Process> process = nullptr;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this]() { return !readyQueue.empty() || !running; });
            if (!running) break;

            {
                std::unique_lock<std::mutex> turnLock(coreTurnMutex);
                if (coreId != nextCoreId) {
                    continue;
                }
            }

            auto candidate = readyQueue.front();
            readyQueue.pop();

            process = candidate;
            process->setAssignedCore(coreId);
            coreAvailable[coreId] = false;
            processHandler.insertProcess(process);

            {
                std::unique_lock<std::mutex> turnLock(coreTurnMutex);
                nextCoreId = (nextCoreId + 1) % numCores;
            }
        }

        if (process) {
            unsigned cyclesUsed = 0;
            const auto& pages = process->getAssignedPages();

            while (cyclesUsed < quantum && !process->isFinished() && running) {
                int instructionIndex = process->getCurrentInstructionIndex();
                int pageChunkSize = std::max(1, static_cast<int>(frameSize / estimatedInstructionSize));
                int pageLocal = instructionIndex / pageChunkSize;

                if (pageLocal >= pages.size()) break;

                int globalPage = pages[pageLocal];

                if (!memoryManager.accessPage(globalPage)) {
                    if (!memoryManager.pageFault(globalPage)) {
                        std::cerr << "[PAGE FAULT] Could not resolve page fault for page " << globalPage << "\n";
                        break;
                    }
                }

                process->executeNextInstruction();
                cyclesUsed++;
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

            cv.notify_all();

            if (coreId == 0 && cyclesUsed > 0) {
                std::lock_guard<std::mutex> snapLock(snapshotMutex);
                std::string filename = "memory_stamp_" + std::to_string(quantumCycle++) + ".txt";
                memoryManager.generateSnapshot(filename, quantumCycle);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
