#include "Scheduler.h"
#include "Instruction.h"
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
            << ")  (" << process->getCreationTime() << ")  on Core: " << process->getAssignedCore()
            << "  " << process->getCurrentInstructionIndex() << "/" << process->getInstructionCount() << "\n";
    }

    std::cout << "\nFinished processes (" << finishedProcs.size() << "):\n";
    for (const auto& process : finishedProcs) {
        std::cout << "  " << process->getName()
            << " (ID: " << process->getId() << ")   Finished!\n";
    }
    std::cout << "===================\n";
}

void Scheduler::generateReport(const std::string& filename) {
    std::lock_guard<std::mutex> lock(queueMutex);

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }

    // Clean up finished processes list
    runningProcesses.erase(
        std::remove_if(runningProcesses.begin(), runningProcesses.end(),
            [](const auto& p) { return p->isFinished(); }),
        runningProcesses.end());

    outFile << "=== Process Report ===\n";
    outFile << "CPU Cores: " << numCores << " (";
    for (bool available : coreAvailable) {
        outFile << (available ? "free" : "busy") << " ";
    }
    outFile << ")\n\n";

    outFile << "Running processes (" << runningProcesses.size() << "):\n";
    for (const auto& process : runningProcesses) {
        outFile << "  " << process->getName()
            << " (ID: " << process->getId() << ")  "
            << "(" << process->getCreationTime() << ")  "
            << "on Core: " << process->getAssignedCore()
            << "  " << process->getCurrentInstructionIndex()
            << "/" << process->getInstructionCount()
            << "\n";
    }

    outFile << "\nFinished processes (" << finishedProcesses.size() << "):\n";
    for (const auto& process : finishedProcesses) {
        outFile << "  " << process->getName()
            << " (ID: " << process->getId() << ")\n";
    }

    outFile << "=======================\n";

    outFile.close();
}