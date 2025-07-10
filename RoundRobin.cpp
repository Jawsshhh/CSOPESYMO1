#include "RoundRobin.h"
#include <chrono>
#include <iostream>

RRScheduler::RRScheduler(int numCores, int quantum, int delays_per_exec)
    : Scheduler(numCores), quantum(quantum), delays_per_exec(delays_per_exec) {
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
            cv.wait(lock, [this]() {
                return !running || !readyQueue.empty();
                });
            if (!running) break;

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
        }
    }
}
