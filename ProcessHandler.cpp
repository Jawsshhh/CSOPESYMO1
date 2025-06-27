#include "ProcessHandler.h"
#include <algorithm>

void ProcessHandler::insertProcess(const std::shared_ptr<Process>& process) {
    std::lock_guard<std::mutex> lock(processMutex);
    processes.push_back(process);
}

bool ProcessHandler::deleteProcess(int processId) {
    std::lock_guard<std::mutex> lock(processMutex);
    auto it = std::remove_if(processes.begin(), processes.end(),
        [processId](const auto& p) { return p->getId() == processId; });

    if (it != processes.end()) {
        processes.erase(it, processes.end());
        return true;
    }
    return false;
}

void ProcessHandler::updateProcessState(int processId, bool isFinished) {
    std::lock_guard<std::mutex> lock(processMutex);
    for (auto& process : processes) {
        if (process->getId() == processId) {
            process->setFinished(isFinished);
            break;
        }
    }
}

std::shared_ptr<Process> ProcessHandler::getProcess(int processId) {
    std::lock_guard<std::mutex> lock(processMutex);
    for (const auto& process : processes) {
        if (process->getId() == processId) {
            return process;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Process>> ProcessHandler::getAllProcesses() {
    std::lock_guard<std::mutex> lock(processMutex);
    return processes;
}

std::vector<std::shared_ptr<Process>> ProcessHandler::getRunningProcesses() {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<std::shared_ptr<Process>> running;
    std::unordered_set<int> seenIds;

    for (const auto& process : processes) {
        if (!process->isFinished() && seenIds.insert(process->getId()).second) {
            running.push_back(process);
        }
    }
    return running;
}

std::vector<std::shared_ptr<Process>> ProcessHandler::getFinishedProcesses() {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<std::shared_ptr<Process>> finished;
    std::unordered_set<int> seenIds;

    for (const auto& process : processes) {
        if (process->isFinished() && seenIds.insert(process->getId()).second) {
            finished.push_back(process);
        }
    }
    return finished;
}

std::vector<std::shared_ptr<Process>> ProcessHandler::getProcessesByCore(int coreId) {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<std::shared_ptr<Process>> coreProcesses;
    for (const auto& process : processes) {
        if (process->getAssignedCore() == coreId) {
            coreProcesses.push_back(process);
        }
    }
    return coreProcesses;
}

bool ProcessHandler::hasUnfinishedProcessOnCore(int coreId) {
    std::lock_guard<std::mutex> lock(processMutex);
    for (const auto& process : processes) {
        if (process->getAssignedCore() == coreId && !process->isFinished()) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<Process> ProcessHandler::getFirstUnfinishedProcessOnCore(int coreId) {
    std::lock_guard<std::mutex> lock(processMutex);
    for (const auto& process : processes) {
        if (process->getAssignedCore() == coreId && !process->isFinished()) {
            return process;
        }
    }
    return nullptr;
}

void ProcessHandler::markProcessFinished(int processId) {
    std::lock_guard<std::mutex> lock(processMutex);
    for (auto& process : processes) {
        if (process->getId() == processId) {
            process->setFinished(true);
            break;
        }
    }
}