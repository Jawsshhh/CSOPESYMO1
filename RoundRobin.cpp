#include "RoundRobin.h"
#include <chrono>

RRScheduler::RRScheduler(int numCores, int quantum) : Scheduler(numCores), quantum(quantum) {
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

        cv.wait(lock, [this]() {
            return !readyQueue.empty() || !running;
            });

        if (!running) break;

        for (int core = 0; core < numCores && !readyQueue.empty(); ++core) {
            if (coreAvailable[core]) {
                auto process = readyQueue.front();
                readyQueue.pop();

                process->setAssignedCore(core);
                coreAvailable[core] = false;
                runningProcesses.push_back(process);
                cv.notify_all();
            }
        }

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void RRScheduler::workerLoop(int coreId) {
    while (running) {
        std::shared_ptr<Process> process;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this, coreId]() {
                for (const auto& p : runningProcesses) {
                    if (p->getAssignedCore() == coreId && !p->isFinished()) {
                        return true;
                    }
                }
                return !running;
                });

            if (!running) break;

            for (auto it = runningProcesses.begin(); it != runningProcesses.end(); ++it) {
                if ((*it)->getAssignedCore() == coreId && !(*it)->isFinished()) {
                    process = *it;
                    break;
                }
            }
        }

        if (process) {
            unsigned cyclesUsed = 0;

            while (cyclesUsed < quantum && !process->isFinished() && running) {
                process->executeNextInstruction();
                cyclesUsed++;

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (!process->isFinished() && running) {
                std::lock_guard<std::mutex> lock(queueMutex);
                readyQueue.push(process);
                cv.notify_one();
            }
            else if (process->isFinished()) {
                std::lock_guard<std::mutex> lock(queueMutex);
                finishedProcesses.push_back(process);
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                runningProcesses.erase(
                    std::remove(runningProcesses.begin(), runningProcesses.end(), process),
                    runningProcesses.end());
                coreAvailable[coreId] = true;
            }
            cv.notify_all();
        }
    }
}