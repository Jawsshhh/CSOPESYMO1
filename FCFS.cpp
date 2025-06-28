#include "FCFS.h"
#include <chrono>
#include <iostream>
FCFSScheduler::FCFSScheduler(int numCores, int delays_per_exec) : Scheduler(numCores), delays_per_exec (delays_per_exec) {
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

        while (!processQueue.empty()) {
            int core = findAvailableCore();
            if (core == -1) break;

            auto process = processQueue.front();
            processQueue.pop();
            process->setAssignedCore(core);
            coreAvailable[core] = false;
            processHandler.insertProcess(process);
            cv.notify_all();
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
                return !running || processHandler.hasUnfinishedProcessOnCore(coreId);
                });
            if (!running) break;

           
            auto allProcs = processHandler.getAllProcesses();
            for (auto& p : allProcs) {
                if (p->getAssignedCore() == coreId && p->isSleeping()) {
                    p->updateSleep();
                    if (!p->isSleeping() && !p->isFinished()) {
                        std::cout << "Process " << p->getId() << " woke up\n";
                    }
                }
            }

            auto coreProcs = processHandler.getProcessesByCore(coreId);
            auto it = std::find_if(coreProcs.begin(), coreProcs.end(), [](const auto& p) {
                return !p->isFinished() && !p->isSleeping();
                });
            if (it != coreProcs.end()) {
                process = *it;
            }
        }

        if (process) {
            while (!process->isFinished() && running) {
                if (process->isSleeping()) break;
                process->executeNextInstruction();
                if (process->isSleeping()) break;

                auto start = std::chrono::high_resolution_clock::now();
                while (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - start).count() < delays_per_exec) {
                }
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (process->isFinished()) {
                    process->setFinished(true);
                }
                coreAvailable[coreId] = true;
                cv.notify_all();
            }
        }
    }
}

