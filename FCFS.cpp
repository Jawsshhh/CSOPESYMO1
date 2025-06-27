#include "FCFS.h"
#include <chrono>
#include <iostream>
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

        while (!processQueue.empty()) {
            int core = findAvailableCore();
            if (core == -1) break; // No cores available

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
                if (!running) return true;

                auto coreProcs = processHandler.getProcessesByCore(coreId);
                return std::any_of(coreProcs.begin(), coreProcs.end(),
                    [](const auto& p) { return !p->isFinished(); });
                });

            if (!running) break;

            auto coreProcs = processHandler.getProcessesByCore(coreId);
            auto it = std::find_if(coreProcs.begin(), coreProcs.end(),
                [](const auto& p) { return !p->isFinished(); });
            if (it != coreProcs.end()) {
                process = *it;
            }
        }

        if (process) {
            while (!process->isFinished() && running) {
                process->executeNextInstruction();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                process->setFinished(true);
                coreAvailable[coreId] = true;
                cv.notify_all();
            }
        }
    }
}