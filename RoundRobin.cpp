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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void RRScheduler::workerLoop(int coreId) {
    while (running) {
        std::shared_ptr<Process> process = nullptr;

        {
            std::unique_lock<std::mutex> turnLock(coreTurnMutex);
            cv.wait(turnLock, [&]() { return coreId == nextCoreId || !running; });
            if (!running) break;
        }

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (readyQueue.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            process = readyQueue.front();
            readyQueue.pop();

            process->setAssignedCore(coreId);
            coreAvailable[coreId] = false;
            processHandler.insertProcess(process);
        }

        if (process) {
            const auto& assignedPages = process->getAssignedPages();

            if (process->isSleeping()) {
                process->setSleeping(true, process->getRemainingSleepTicks() - 1);
                std::lock_guard<std::mutex> lock(queueMutex);
                readyQueue.push(process);
            }
            else {
                int instructionIndex = process->getCurrentInstructionIndex();
                int pageLocal = instructionIndex / static_cast<int>(frameSize);

                if (pageLocal < assignedPages.size()) {
                    int globalPage = assignedPages[pageLocal];

                    int retries = 3;
                    while (!memoryManager.accessPage(globalPage) && retries-- > 0) {
                        memoryManager.pageFault(globalPage);
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }

                    if (!memoryManager.accessPage(globalPage)) {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        readyQueue.push(process);
                    }
                    else if (process->getCurrentInstructionIndex() < process->getInstructionCount()) {
                        process->executeNextInstruction();
                        std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
                        std::lock_guard<std::mutex> lock(queueMutex);
                        if (process->isFinished()) {
                            processHandler.markProcessFinished(process->getId());
                        }
                        else {
                            readyQueue.push(process);
                        }
					}
                    else if (process->getCurrentInstructionIndex() >= process->getInstructionCount()) {
                        process->setFinished(true);
                        processHandler.markProcessFinished(process->getId());
					}

                    else if (!process->isFinished()) {
                        process->executeNextInstruction();
                        std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));

                        std::lock_guard<std::mutex> lock(queueMutex);
                        if (process->isFinished()) {
                            processHandler.markProcessFinished(process->getId());
                        }
                        else {
                            readyQueue.push(process);
                        }
                    }
                }
                else {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    readyQueue.push(process);
                }
            }

            coreAvailable[coreId] = true;
        }

        {
            std::lock_guard<std::mutex> turnLock(coreTurnMutex);
            nextCoreId = (nextCoreId + 1) % coreAvailable.size();
            cv.notify_all();
        }

        if (coreId == 0 && process && process->getInstructionCount() > 0) {
            std::lock_guard<std::mutex> snapLock(snapshotMutex);
            std::string filename = "memory_stamp_" + std::to_string(quantumCycle++) + ".txt";
            // memoryManager.generateSnapshot(filename, quantumCycle);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
