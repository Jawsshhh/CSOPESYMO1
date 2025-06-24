#include "FCFS.h"
#include <chrono>

FCFSScheduler::FCFSScheduler(int numCores) : Scheduler(numCores) {
    for (int i = 0; i < numCores; ++i) {
        workerThreads.emplace_back(&FCFSScheduler::workerLoop, this, i);
    }
    schedulerThread = std::thread(&FCFSScheduler::schedulerLoop, this);
}

FCFSScheduler::~FCFSScheduler() {
    stop();
}

void FCFSScheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(queueMutex);
    processQueue.push(process);
    cv.notify_one();
}

void FCFSScheduler::schedulerLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this]() { return !processQueue.empty() || !running; });

        if (!running) break;

        for (int core = 0; core < numCores && !processQueue.empty(); ++core) {
            if (coreAvailable[core]) {
                auto process = processQueue.front();
                processQueue.pop();

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

void FCFSScheduler::workerLoop(int coreId) {
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
            while (!process->isFinished() && running) {
                process->executeNextInstruction();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                runningProcesses.erase(
                    std::remove(runningProcesses.begin(), runningProcesses.end(), process),
                    runningProcesses.end());
                finishedProcesses.push_back(process);
                coreAvailable[coreId] = true;
            }
            cv.notify_all();
        }
    }
}