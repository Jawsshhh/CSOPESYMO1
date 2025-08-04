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

    // Try to allocate pages with retry mechanism
    if (tryAllocateProcessPages(process)) {
        readyQueue.push(process);
        cv.notify_all();
        //std::cout << "[INFO] Process " << process->getId() << " added to ready queue" << std::endl;
    }
    else {
        // Add to pending queue for retry
        pendingProcesses.push(process);
        //std::cout << "[WARN] Process " << process->getId() << " queued for retry (memory allocation failed)" << std::endl;
    }
}

bool RRScheduler::tryAllocateProcessPages(std::shared_ptr<Process> process) {
    size_t memoryNeeded = process->getMemoryNeeded();
    int pagesNeeded = static_cast<int>((memoryNeeded + frameSize - 1) / frameSize);

    std::vector<int> assignedPages;

    try {
        // Try to allocate all pages first
        for (int i = 0; i < pagesNeeded; ++i) {
            int pageId = memoryManager.getNextGlobalPageId();
            assignedPages.push_back(pageId);
        }

        // Initialize page data for all allocated pages
        for (int i = 0; i < assignedPages.size(); ++i) {
            int pageId = assignedPages[i];
            memoryManager.initializePageData(pageId, "Init data for page " + std::to_string(pageId));
        }

        // Register all pages with the process
        memoryManager.registerProcessPages(process->getId(), assignedPages);
        process->assignPages(assignedPages);

        //std::cout << "[INFO] Successfully allocated " << pagesNeeded << " pages for process " << process->getId() << std::endl;
        return true;

    }
    catch (const std::runtime_error& e) {
        // Clean up any partially allocated pages
        for (int pageId : assignedPages) {
            try {
                memoryManager.releasePage(pageId);
            }
            catch (...) {
                // Ignore cleanup errors
            }
        }

        
        return false;
    }
}

void RRScheduler::schedulerLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);

        // Try to process pending processes (retry page allocation)
        if (!pendingProcesses.empty()) {
            std::queue<std::shared_ptr<Process>> stillPending;

            while (!pendingProcesses.empty()) {
                auto process = pendingProcesses.front();
                pendingProcesses.pop();

                if (tryAllocateProcessPages(process)) {
                    readyQueue.push(process);
                    //std::cout << "[INFO] Retry successful: Process " << process->getId() << " moved to ready queue" << std::endl;
                }
                else {
                    stillPending.push(process);
                }
            }

            // Put back processes that still can't be allocated
            pendingProcesses = stillPending;
        }

        if (!readyQueue.empty()) {
            cv.notify_all();
        }
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Slightly longer delay for retry logic
    }
}

void RRScheduler::workerLoop(int coreId) {
    while (running) {
        totalCpuTicks++;
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
                idleCpuTicks++;
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
            idleCpuTicks++;
            continue;
        }

        // Execute the process for the quantum
        bool processFinished = false;
        bool requeueProcess = true;
        int instructionsExecuted = 0;

        try {
            // Execute for quantum cycles or until process finishes
            while (instructionsExecuted < quantum &&
                process->getCurrentInstructionIndex() < process->getInstructionCount()) {
                activeCpuTicks++;

                const auto& assignedPages = process->getAssignedPages();
                if (!assignedPages.empty()) {
                    // Calculate which page is needed for current instruction
                    int pageLocal = process->getCurrentInstructionIndex() / static_cast<int>(frameSize);
                    if (pageLocal < assignedPages.size()) {
                        int globalPage = assignedPages[pageLocal];

                        // Try to access the page
                        bool pageAccessible = false;
                        for (int retry = 0; retry < 3 && !pageAccessible; retry++) {
                            pageAccessible = memoryManager.accessPage(globalPage);
                            if (!pageAccessible) {
                                if (!memoryManager.pageFault(globalPage)) {
                                    throw std::runtime_error("Page fault handling failed for page " + std::to_string(globalPage));
                                }
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            }
                        }

                        if (pageAccessible) {
                            process->executeNextInstruction();
                            instructionsExecuted++;

                            if (delays_per_exec > 0) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
                            }
                        }
                        else {
                          //  throw std::runtime_error("Page access failed after retries for page " + std::to_string(globalPage));
                            idleCpuTicks++;
                        }
                    }
                    else {
                        throw std::runtime_error("Invalid page index: " + std::to_string(pageLocal) +
                            " for process with " + std::to_string(assignedPages.size()) + " pages");

                    }
                }
                else {
                    throw std::runtime_error("Process has no assigned pages");
                }
            }

            // Check if process finished
            processFinished = process->getCurrentInstructionIndex() >= process->getInstructionCount();
        }
        catch (const std::exception& e) {
           // std::cerr << "[ERROR] Error executing process " << process->getId() << ": " << e.what() << std::endl;
            processFinished = true;
            requeueProcess = false;
        }

        // Handle process completion
        if (processFinished) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                process->setIsFinished(true);
                processHandler.markProcessFinished(process->getId());

                // Release all process pages when finished
                try {
                    memoryManager.releaseProcessPages(process->getId());
                   // std::cout << "[INFO] Process " << process->getId() << " completed - pages released for reuse" << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[ERROR] Failed to release pages for process " << process->getId()
                        << ": " << e.what() << std::endl;
                }
                requeueProcess = false;
            }
        }

        // Requeue the process if it's not finished and quantum expired
        if (requeueProcess && !processFinished) {
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