#include "RoundRobin.h"
#include <chrono>
#include <iostream>
#include <cmath> // for instruction/page conversion
#include <algorithm>


RRScheduler::RRScheduler(int numCores, int quantum, int delays_per_exec,
    size_t maxMemory, size_t frameSize)
    : Scheduler(numCores, maxMemory, frameSize),
    quantum(quantum),
    delays_per_exec(delays_per_exec),
    frameSize(frameSize) { // store frameSize locally
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
    constexpr int estimatedInstructionSize = 4; // You may fine-tune this later

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

            // Try to find a process to run
            size_t attempts = 0;
            const size_t queueSize = readyQueue.size();

            while (attempts < queueSize) {
                auto candidate = readyQueue.front();
                readyQueue.pop();
                attempts++;

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
                int instructionIndex = process->getCurrentInstructionIndex();
                int pageChunkSize = std::max(1, static_cast<int>(frameSize / estimatedInstructionSize));
                int page = instructionIndex / pageChunkSize;

               
                if (!memoryManager.accessPage(process->getId(), page)) {
                    memoryManager.pageFault(process->getId(), page);
                    continue; // Retry after page fault
                }

                process->executeNextInstruction();
                cyclesUsed++;
                std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);

                if (process->isFinished()) {
                    processHandler.markProcessFinished(process->getId());
                    // No deallocateMemory() — paging handles this dynamically
                }
                else {
                    readyQueue.push(process); // Requeue if not finished
                }

                coreAvailable[coreId] = true;
            }

            cv.notify_all();

            // Generate snapshot from core 0
            if (coreId == 0 && cyclesUsed > 0) {
                std::lock_guard<std::mutex> snapLock(snapshotMutex);
                std::string filename = "memory_stamp_" + std::to_string(quantumCycle++) + ".txt";
                memoryManager.generateSnapshot(filename, quantumCycle);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
