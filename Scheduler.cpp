#include "Scheduler.h"
#include "Instruction.h"
#include <iostream>
#include <algorithm>
#include <fstream>

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

    int coresUsed = 0;
    for (bool available : coreAvailable) {
        if (!available) coresUsed++;
    }
    int coresAvailable = numCores - coresUsed;

    int cpuUtilization = static_cast<int>((static_cast<float>(coresUsed) / numCores) * 100);

    std::cout << "\n=== Process List ===\n";
    std::cout << "CPU utilization: " << cpuUtilization << "\n";
    std::cout << "Cores used: " << coresUsed << "\n";
    std::cout << "Cores available: " << coresAvailable << "\n";    
    
    
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

    auto runningProcs = processHandler.getRunningProcesses();
    auto finishedProcs = processHandler.getFinishedProcesses();

    int coresUsed = 0;
    for (bool available : coreAvailable) {
        if (!available) coresUsed++;
    }
    int coresAvailable = numCores - coresUsed;

    int cpuUtilization = static_cast<int>((static_cast<float>(coresUsed) / numCores) * 100);

    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }

    outputFile << "=== Process List ===\n";
    outputFile << "CPU utilization: " << cpuUtilization << "\n";
    outputFile << "Cores used: " << coresUsed << "\n";
    outputFile << "Cores available: " << coresAvailable << "\n";

    outputFile << "\nRunning processes (" << runningProcs.size() << "):\n";
    for (const auto& process : runningProcs) {
        outputFile << "  " << process->getName()
            << " (ID: " << process->getId()
            << ")  (" << process->getCreationTime() << ")  on Core: " << process->getAssignedCore()
            << "  " << process->getCurrentInstructionIndex() << "/" << process->getInstructionCount() << "\n";
    }

    outputFile << "\nFinished processes (" << finishedProcs.size() << "):\n";
    for (const auto& process : finishedProcs) {
        outputFile << "  " << process->getName()
            << " (ID: " << process->getId() << ")   Finished!\n";
    }
    outputFile << "===================\n";
    outputFile.close();
}