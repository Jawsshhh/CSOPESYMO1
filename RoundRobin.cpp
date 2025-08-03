#include "RoundRobin.h"
#include <chrono>
#include <iostream>
#include <cmath>
#include <algorithm>

RRScheduler::RRScheduler(int numCores, int quantum, int delays_per_exec,
    size_t maxMemory, size_t frameSize)
    : Scheduler(numCores, maxMemory, frameSize),
    quantum(quantum),
    delays_per_exec(delays_per_exec),
    frameSize(frameSize) {

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

    size_t memoryNeeded = process->getMemoryNeeded();
    int pagesNeeded = static_cast<int>((memoryNeeded + frameSize - 1) / frameSize);

    std::vector<int> assignedPages;

    try {
        for (int i = 0; i < pagesNeeded; ++i) {
            int pageId = memoryManager.getNextGlobalPageId();
            assignedPages.push_back(pageId);
            memoryManager.initializePageData(pageId, "Init data for page " + std::to_string(pageId));
        }

        memoryManager.registerProcessPages(process->getId(), assignedPages);
        process->assignPages(assignedPages);
        readyQueue.push(process);
        cv.notify_all();

    }
    catch (const std::runtime_error& e) {
        // std::cerr << "[RRScheduler::addProcess] " << e.what() << "\n";
    }
}

void RRScheduler::schedulerLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (!readyQueue.empty()) {
            cv.notify_all();
        }
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void RRScheduler::workerLoop(int coreId) {
    while (running) {
        std::shared_ptr<Process> process = nullptr;

        // Wait for this core's turn
        {
            std::unique_lock<std::mutex> turnLock(coreTurnMutex);
            bool gotTurn = cv.wait_for(turnLock, std::chrono::milliseconds(100), [&]() {
                return coreId == nextCoreId || !running;
                });

            if (!running) break;

            if (!gotTurn) {
                nextCoreId = (nextCoreId + 1) % coreAvailable.size();
                cv.notify_all();
                continue;
            }
        }

        // Try to get a process to run
        bool gotProcess = false;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (!readyQueue.empty()) {
                process = readyQueue.front();
                readyQueue.pop();
                process->setAssignedCore(coreId);
                coreAvailable[coreId] = false;
                processHandler.insertProcess(process);
                gotProcess = true;
            }
        }

        // If no process was found, pass turn to next core
        if (!gotProcess) {
            std::lock_guard<std::mutex> turnLock(coreTurnMutex);
            nextCoreId = (nextCoreId + 1) % coreAvailable.size();
            cv.notify_all();
            continue;
        }

        // Execute the process
        bool processFinished = false;
        bool requeueProcess = true;

        try {
            if (process->getCurrentInstructionIndex() < process->getInstructionCount()) {
                const auto& assignedPages = process->getAssignedPages();
                if (!assignedPages.empty()) {
                    int pageLocal = process->getCurrentInstructionIndex() / static_cast<int>(frameSize);
                    if (pageLocal < assignedPages.size()) {
                        int globalPage = assignedPages[pageLocal];

                        bool pageAccessible = false;
                        for (int retry = 0; retry < 3 && !pageAccessible; retry++) {
                            pageAccessible = memoryManager.accessPage(globalPage);
                            if (!pageAccessible) {
                                if (!memoryManager.pageFault(globalPage)) {
                                    throw std::runtime_error("Page fault handling failed");
                                }
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            }
                        }

                        if (pageAccessible) {
                            process->executeNextInstruction();
                            std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
                        }
                        else {
                            throw std::runtime_error("Page access failed after retries");
                        }
                    }
                }
            }

            processFinished = process->getCurrentInstructionIndex() >= process->getInstructionCount();
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Error executing process " << process->getId() << ": " << e.what() << std::endl;
            processFinished = true;
            requeueProcess = false;
        }

        // If process finished, clean it up
        if (processFinished) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                process->setIsFinished(true);
                processHandler.markProcessFinished(process->getId());
                try {
                    memoryManager.releaseProcessPages(process->getId());
                }
                catch (const std::exception& e) {
                    std::cerr << "[ERROR] Failed to release pages for process " << process->getId()
                        << ": " << e.what() << std::endl;
                }
                requeueProcess = false;
            }
        }

        // Requeue the process if needed
        if (requeueProcess) {
            std::lock_guard<std::mutex> lock(queueMutex);
            readyQueue.push(process);
        }

        // Mark core as available
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            coreAvailable[coreId] = true;
        }

        // Pass turn to next core
        {
            std::lock_guard<std::mutex> turnLock(coreTurnMutex);
            nextCoreId = (nextCoreId + 1) % coreAvailable.size();
            cv.notify_all();
        }
    }
}
