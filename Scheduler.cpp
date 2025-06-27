#include "Scheduler.h"
#include <iostream>
#include <algorithm>

Scheduler::Scheduler(int numCores) : numCores(numCores), running(false) {
    coreAvailable.resize(numCores, true); 
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (!running) {
        running = true;
    }
}

void Scheduler::stop() {
    if (running) {
        running = false;
        cv.notify_all();  

        if (schedulerThread.joinable()) {
            schedulerThread.join();
        }

        for (auto& thread : workerThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
}

void Scheduler::listProcesses() {
    std::lock_guard<std::mutex> lock(queueMutex);  // Ensure thread-safe access

    // Get current process states
    auto runningProcs = processHandler.getRunningProcesses();
    auto finishedProcs = processHandler.getFinishedProcesses();

    std::cout << "\n=== Process List ===\n";
    std::cout << "CPU Cores: " << numCores << " (";
    for (int i = 0; i < numCores; ++i) {
        std::cout << (coreAvailable[i] ? "free" : "busy");
        if (i != numCores - 1) std::cout << " ";
    }
    std::cout << ")\n";

    std::cout << "\nRunning processes (" << runningProcs.size() << "):\n";
    for (const auto& process : runningProcs) {
        std::cout << "  " << process->getName()
            << " (ID: " << process->getId()
            << ") on Core: " << process->getAssignedCore()
            << "\n";
    }

    std::cout << "\nFinished processes (" << finishedProcs.size() << "):\n";
    for (const auto& process : finishedProcs) {
        std::cout << "  " << process->getName()
            << " (ID: " << process->getId() << ")\n";
    }
    std::cout << "===================\n";
}

