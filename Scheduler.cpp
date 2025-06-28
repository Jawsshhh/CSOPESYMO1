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
    std::lock_guard<std::mutex> lock(queueMutex);
    lastPrintedProcessLines.clear();

    auto runningProcs = processHandler.getRunningProcesses();
    auto finishedProcs = processHandler.getFinishedProcesses();

    int coresUsed = 1;
    for (bool available : coreAvailable) {
        if (!available) coresUsed++;
    }
    int coresAvailable = numCores - coresUsed;
    int cpuUtilization = static_cast<int>((static_cast<float>(coresUsed) / numCores) * 100);

    std::cout << "\n=== Process List ===\n";
    lastPrintedProcessLines.push_back("=== Process List ===");

    std::string cpuUtil = "CPU utilization: " + std::to_string(cpuUtilization) + "%";
    std::string usedLine = "Cores used: " + std::to_string(coresUsed);
    std::string availLine = "Cores available: " + std::to_string(coresAvailable);

    std::cout << cpuUtil << "\n" << usedLine << "\n" << availLine << "\n";
    lastPrintedProcessLines.push_back(cpuUtil);
    lastPrintedProcessLines.push_back(usedLine);
    lastPrintedProcessLines.push_back(availLine);

    std::string runningHeader = "\nRunning processes (" + std::to_string(runningProcs.size()) + "):";
    std::cout << runningHeader << "\n";
    lastPrintedProcessLines.push_back(runningHeader);

    for (const auto& process : runningProcs) {
        std::string processLog = "  " + process->getName() +
            " (ID: " + std::to_string(process->getId()) + ")  (" + process->getCreationTime() +
            ")  on Core: " + std::to_string(process->getAssignedCore()) +
            "  " + std::to_string(process->getCurrentInstructionIndex()) + "/" +
            std::to_string(process->getInstructionCount());

        std::cout << processLog << "\n";
        lastPrintedProcessLines.push_back(processLog);
    }

    std::string finishedHeader = "\nFinished processes (" + std::to_string(finishedProcs.size()) + "):";
    std::cout << finishedHeader << "\n";
    lastPrintedProcessLines.push_back(finishedHeader);

    for (const auto& process : finishedProcs) {
        std::string line = "  " + process->getName() +
            " (ID: " + std::to_string(process->getId()) + ")   Finished!";
        std::cout << line << "\n";
        lastPrintedProcessLines.push_back(line);
    }
    lastPrintedProcessLines.push_back("===================\n");
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