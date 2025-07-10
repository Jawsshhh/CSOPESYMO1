#include "RoundRobin.h"
#include <chrono>
#include <iostream>

RRScheduler::RRScheduler(int numCores, int quantum, int delays_per_exec,
    size_t maxMemory, size_t frameSize, size_t procMemory)
    : Scheduler(numCores, maxMemory, frameSize, procMemory),
    quantum(quantum),
    delays_per_exec(delays_per_exec) {
    for (int i = 0; i < numCores; ++i) {
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

        for (int core = 0; core < numCores && !readyQueue.empty(); ++core) {
            if (coreAvailable[core]) {
                auto process = readyQueue.front();

                //  try allocating memory before assigning to core
                if (memoryManager.allocateMemory(process->getId())) {
                    readyQueue.pop();
                    process->setAssignedCore(core);
                    coreAvailable[core] = false;
                    processHandler.insertProcess(process);
                    cv.notify_all();
                }
                else {
                    readyQueue.pop();
                    readyQueue.push(process);
                }
            }
        }

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


void RRScheduler::workerLoop(int coreId) {
    while (running) {
        std::shared_ptr<Process> process = nullptr;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this]() {
                return !readyQueue.empty() || !running;
                });

            if (!running) break;

            // Always pop the next available process for this quantum
            process = readyQueue.front();
            readyQueue.pop();

            // Try to allocate memory (only if process is new)
            if (!memoryManager.isInMemory(process->getId())) {
                if (!memoryManager.allocateMemory(process->getId())) {
                    std::cout << "[WARN] PID " << process->getId()
                        << " couldn't allocate memory, requeuing\n";
                    // No memory: move to back and skip this round
                    readyQueue.push(process);
                    process = nullptr;
                }
            }

            if (process) {
                process->setAssignedCore(coreId);
                coreAvailable[coreId] = false;
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
                    // Push back for RR rotation
                    readyQueue.push(process);
                }

                coreAvailable[coreId] = true;
            }

            cv.notify_all();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
