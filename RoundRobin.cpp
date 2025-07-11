#include "RoundRobin.h"
#include <chrono>
#include <iostream>

RRScheduler::RRScheduler(int numCores, int quantum, int delays_per_exec,
    size_t maxMemory, size_t frameSize)
    : Scheduler(numCores, maxMemory, frameSize),
    quantum(quantum),
    delays_per_exec(delays_per_exec) {
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
    readyQueue.push(process);
    cv.notify_one();
}

void RRScheduler::schedulerLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this]() { return !readyQueue.empty() || !running; });
        if (!running) break;

        cv.notify_all();  // Wake up all worker threads
        lock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}



void RRScheduler::workerLoop(int coreId) {
    while (running) {
        std::shared_ptr<Process> process = nullptr;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this]() { return !readyQueue.empty() || !running; });
            if (!running) break;

            // Fair turn-based access per core
            {
                std::unique_lock<std::mutex> turnLock(coreTurnMutex);
                if (coreId != nextCoreId) {
                    continue;  // Not this core's turn
                }
            }

            while (!readyQueue.empty()) {
                auto candidate = readyQueue.front();
                readyQueue.pop();

                if (!memoryManager.isInMemory(candidate->getId())) {
                    if (!memoryManager.allocateMemory(candidate->getId(), candidate->getMemoryNeeded())) {
                        readyQueue.push(candidate);
                        continue;
                    }
                }

                process = candidate;
                process->setAssignedCore(coreId);
                coreAvailable[coreId] = false;
                processHandler.insertProcess(process);
                break;
            }

            // Round-robin core access
            {
                std::unique_lock<std::mutex> turnLock(coreTurnMutex);
                nextCoreId = (nextCoreId + 1) % numCores;
            }
        }

        if (process) {
            unsigned cyclesUsed = 0;
            while (cyclesUsed < quantum && !process->isFinished() && running) {
                process->executeNextInstruction();
                cyclesUsed++;
                std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);

                if (process->isFinished()) {
                    processHandler.markProcessFinished(process->getId());
                    memoryManager.deallocateMemory(process->getId());
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
                memoryManager.generateMemorySnapshot(filename, quantumCycle);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
