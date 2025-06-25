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
    std::lock_guard<std::mutex> lock(queueMutex);

    runningProcesses.erase(
        std::remove_if(runningProcesses.begin(), runningProcesses.end(),
            [](const auto& p) { return p->isFinished(); }),
        runningProcesses.end());

    std::cout << "\n=== Process List ===\n";
    std::cout << "CPU Cores: " << numCores << " (";
    for (bool available : coreAvailable) {
        std::cout << (available ? "free" : "busy") << " ";
    }
    std::cout << ")\n";

    std::cout << "\nRunning processes (" << runningProcesses.size() << "):\n";
    for (const auto& process : runningProcesses) {
        std::cout << "  " << process->getName()
            << " (ID: " << process->getId()
            << ") on Core: " << process->getAssignedCore()
            << "\n";
    }

    std::cout << "\nFinished processes (" << finishedProcesses.size() << "):\n";
    for (const auto& process : finishedProcesses) {
        std::cout << "  " << process->getName()
            << " (ID: " << process->getId() << ")\n";
    }
    std::cout << "===================\n";
}


