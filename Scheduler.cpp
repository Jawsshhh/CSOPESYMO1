#include "Scheduler.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <unordered_set>

Scheduler::Scheduler(int numCores, size_t maxMemory, size_t frameSize)
    : numCores(numCores), running(false), memoryManager(maxMemory, frameSize) {
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

     auto runningProcs = processHandler.getCurrentlyActiveProcessesPerCore(numCores);
    //auto runningProcs = processHandler.getRunningProcesses();
    auto finishedProcs = processHandler.getFinishedProcesses();

    /*int coresUsed = 0;
   for (bool available : coreAvailable) {
       if (!available) coresUsed++;
   }*/
    std::unordered_set<int> usedCores;
    for (const auto& p : runningProcs) {
        usedCores.insert(p->getAssignedCore());
    }
    int coresUsed = static_cast<int>(usedCores.size());

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

    std::unordered_set<int> printedProcessIds;

    for (const auto& process : runningProcs) {
        int pid = process->getId();
        if (printedProcessIds.count(pid)) continue;  // skip duplicate

        printedProcessIds.insert(pid);

        std::string processLog = "  " + process->getName() +
            " (ID: " + std::to_string(pid) + ")  (" + process->getCreationTime() +
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

bool MemoryManager::isInMemory(int pid) const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    for (const auto& block : memoryBlocks) {
        if (block.allocated && block.processId == pid) return true;
    }
    return false;
}


void Scheduler::generateReport(const std::string& filename) {
    std::lock_guard<std::mutex> lock(queueMutex);

    std::ofstream outputFile(filename);
    /*if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }*/

    for (const auto& line : lastPrintedProcessLines) {
        outputFile << line << "\n";
    }

    //outputFile.flush();
    outputFile.close();
    std::cout << "Report generated to " << filename << "!\n";
}
