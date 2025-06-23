#include "Scheduler.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

FCFSScheduler::FCFSScheduler() : running(false) {
}

FCFSScheduler::~FCFSScheduler() {
    stop();
}

void FCFSScheduler::start() {
    running = true;
    coreAvailable.resize(4, true); // assuming 4 cores, all initially free
    for (int i = 0; i < 4; ++i) {
        workerThreads.emplace_back(&FCFSScheduler::workerLoop, this, i);
    }
    schedulerThread = std::thread(&FCFSScheduler::schedulerLoop, this);
}


void FCFSScheduler::stop() {
    running = false;
    cv.notify_all();
    if (schedulerThread.joinable()) schedulerThread.join();
    for (auto& thread : workerThreads) {
        if (thread.joinable()) thread.join();
    }
}

void FCFSScheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(queueMutex);
    processQueue.push(process);
    cv.notify_one();
}

void FCFSScheduler::listProcesses() {
    std::lock_guard<std::mutex> lock(queueMutex);

    // Filter out finished processes from running list
    runningProcesses.erase(
        std::remove_if(runningProcesses.begin(), runningProcesses.end(),
            [](const auto& p) { return p->isFinished(); }),
        runningProcesses.end());

    std::cout << "\nRunning processes:\n";
    for (const auto& process : runningProcesses) {
        if (!process->isFinished()) {  // Extra safety check
            std::cout << "process_" << process->getId()
                << " (" << process->getCreationTime() << ") "
                << "Core: " << process->getAssignedCore() << "\n";
        }
    }

    std::cout << "\nFinished processes:\n";
    for (const auto& process : finishedProcesses) {
        std::cout << "process_" << process->getId()
            << " (" << process->getCreationTime() << ") "
            << "Finished\n";
    }
    std::cout << std::endl;
}
void FCFSScheduler::schedulerLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);

        cv.wait(lock, [this]() {
            return !processQueue.empty() || !running;
            });

        if (!running) break;

        if (!processQueue.empty()) {
            auto it = std::find(coreAvailable.begin(), coreAvailable.end(), true);
            if (it != coreAvailable.end()) {
                int availableCore = static_cast<int>(std::distance(coreAvailable.begin(), it));


                auto process = processQueue.front();
                processQueue.pop();

                process->setAssignedCore(availableCore);
                coreAvailable[availableCore] = false;
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
                process->executePrintCommand(coreId, process->getName());
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            std::lock_guard<std::mutex> lock(queueMutex);
            runningProcesses.erase(
                std::remove(runningProcesses.begin(), runningProcesses.end(), process),
                runningProcesses.end());
            finishedProcesses.push_back(process);
            coreAvailable[coreId] = true; // mark core as available
            cv.notify_all();
        }
    }
}
