#include "RoundRobin.h"
#include "Instruction.h"
#include <chrono>
#include <iostream>

RRScheduler::RRScheduler(int numCores, int quantum, int delays_per_exec) : Scheduler(numCores), quantum(quantum), delays_per_exec (delays_per_exec){
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

        // Distribute processes to all available cores
        for (int core = 0; core < numCores && !readyQueue.empty(); ++core) {
            if (coreAvailable[core]) {
                auto process = readyQueue.front();
                readyQueue.pop();

                process->setAssignedCore(core);
                coreAvailable[core] = false;
                processHandler.insertProcess(process);

                

                cv.notify_all();
            }
        }
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void RRScheduler::workerLoop(int coreId) {
    while (running) {
        std::shared_ptr<Process> process;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            // Wait until there's work to do or we're stopping
            cv.wait(lock, [this, coreId]() {
                return !running ||
                    !readyQueue.empty() ||
                    processHandler.hasUnfinishedProcessOnCore(coreId);
                });

            if (!running) break;

            // First handle sleeping processes
            auto coreProcs = processHandler.getProcessesByCore(coreId);
            for (auto& p : coreProcs) {
                if (p->isSleeping()) {
                    p->updateSleep();
                    if (!p->isSleeping() && !p->isFinished()) {
                        readyQueue.push(p); // Requeue awakened processes
                        std::cout << "Process " << p->getId() << " woke up\n";
                    }
                }
            }

            // Get next process to execute
            if (!readyQueue.empty()) {
                process = readyQueue.front();
                readyQueue.pop();
                process->setAssignedCore(coreId);
                coreAvailable[coreId] = false;
            }
        }

        if (process) {
            unsigned cyclesUsed = 0;
            while (cyclesUsed < quantum && !process->isFinished() && running) {
                if (process->isSleeping()) {
                    break; // Skip sleeping processes
                }

                process->executeNextInstruction();
                cyclesUsed++;

                if (process->isSleeping()) {
                    std::cout << "Process " << process->getId()
                        << " started sleeping\n";
                    break;
                }

                // Add small delay between instructions
                std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (process->isFinished()) {
                    processHandler.markProcessFinished(process->getId());
                    //std::cout << "Process " << process->getId() << " completed\n";
                }
                else if (!process->isSleeping()) {
                    readyQueue.push(process); // Requeue if not finished or sleeping
                }
                coreAvailable[coreId] = true;
            }
            cv.notify_all();
        }
    }
}