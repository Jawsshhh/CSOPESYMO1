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


void Scheduler::displayProcessSmi() {
    std::lock_guard<std::mutex> lock(queueMutex);

    auto runningProcs = processHandler.getRunningProcesses();

    // CPU Util
    std::unordered_set<int> usedCores;
    for (const auto& p : runningProcs) {
        usedCores.insert(p->getAssignedCore());
    }
    int coresUsed = static_cast<int>(usedCores.size());
    int cpuUtilization = static_cast<int>((static_cast<float>(coresUsed) / numCores) * 100);


    // Memory Util
    int usedMem = memoryManager.getUsedMemory();
    int totalMem = memoryManager.getUsedMemory() + memoryManager.getFreeMemory();       
    int memUtilPercent = static_cast<int>((static_cast<float>(usedMem) / totalMem) * 100);

    // ==== Print ====
    std::cout << "\n========= Process-smi ==========\n";
    std::cout << "CPU Util: " << cpuUtilization << "%\n";
    std::cout << "Memory Usage: " << usedMem << " MiB / " << static_cast<int>(totalMem) << " MiB\n";
    std::cout << "Memory Util: " << memUtilPercent << "%\n";
    std::cout << "============================================\n";
    std::cout << "Running processes and memory usage:\n";
    std::cout << "--------------------------------------------\n";
    for (const auto& process : runningProcs) {
        if (process) {
            std::cout << process->getName() << "  " << process->getMemoryNeeded() << " MiB\n";
        }
    }
    std::cout << "============================================\n";
}

void Scheduler::displayVMStat() {
    // Memory statistics
    size_t totalMemory = memoryManager.getUsedMemory() + memoryManager.getFreeMemory();
    size_t usedMemory = memoryManager.getUsedMemory();
    size_t freeMemory = memoryManager.getFreeMemory();

    // CPU statistics
    long long totalTicks = totalCpuTicks.load();
    long long idleTicks = idleCpuTicks.load();
    long long activeTicks = activeCpuTicks.load();

    // Paging statistics
    long long pagedIn = memoryManager.getPagesIn();
    long long pagedOut = memoryManager.getPagesOut();

    std::cout << "\n============ vmstat ============\n";
    std::cout << "Total Memory: " << totalMemory << " B\n";
    std::cout << "Used Memory: " << usedMemory << " B\n";
    std::cout << "Free Memory: " << freeMemory << " B\n";
    std::cout << "Idle CPU ticks: " << idleTicks << "\n";
    std::cout << "Active CPU ticks: " << activeTicks << "\n";
    std::cout << "Total CPU ticks: " << totalTicks << "\n";
    std::cout << "Num paged in: " << pagedIn << "\n";
    std::cout << "Num paged out: " << pagedOut << "\n";
    std::cout << "==================================\n";
}