#include <algorithm>
#include <unordered_set>
#include "ProcessHandler.h"

void ProcessHandler::insertProcess(const std::shared_ptr<Process>& process) {
    std::lock_guard<std::mutex> lock(processMutex);
    running.push_back(process);
}

bool ProcessHandler::deleteProcess(int processId) {
    std::lock_guard<std::mutex> lock(processMutex);
    auto it = std::remove_if(running.begin(), running.end(),
        [processId](const auto& p) { return p->getId() == processId; });

    if (it != running.end()) {
        running.erase(it, running.end());
        return true;
    }
    return false;
}

void ProcessHandler::updateProcessState(int processId, bool isFinished) {
    std::lock_guard<std::mutex> lock(processMutex);
    for (auto& process : running) {
        if (process->getId() == processId) {
            process->setFinished(isFinished);
            break;
        }
    }
}

std::shared_ptr<Process> ProcessHandler::getProcess(int processId) {
    std::lock_guard<std::mutex> lock(processMutex);
    for (const auto& process : running) {
        if (process->getId() == processId) return process;
    }
    for (const auto& process : finished) {
        if (process->getId() == processId) return process;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Process>> ProcessHandler::getAllProcesses() {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<std::shared_ptr<Process>> all = running;
    all.insert(all.end(), finished.begin(), finished.end());
    return all;
}

std::vector<std::shared_ptr<Process>> ProcessHandler::getRunningProcesses() {
    std::lock_guard<std::mutex> lock(processMutex);

    std::unordered_set<int> seenIds;
    std::vector<std::shared_ptr<Process>> uniqueRunning;

    for (const auto& p : running) {
        if (seenIds.insert(p->getId()).second) {
            uniqueRunning.push_back(p);
        }
    }

    return uniqueRunning;
}


std::vector<std::shared_ptr<Process>> ProcessHandler::getFinishedProcesses() {
    std::lock_guard<std::mutex> lock(processMutex);

    std::unordered_set<int> seenIds;
    std::vector<std::shared_ptr<Process>> uniqueFinished;

    for (const auto& p : finished) {
        if (seenIds.insert(p->getId()).second) {
            uniqueFinished.push_back(p);
        }
    }

    return uniqueFinished;
}


std::vector<std::shared_ptr<Process>> ProcessHandler::getProcessesByCore(int coreId) {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<std::shared_ptr<Process>> result;
    for (const auto& process : running) {
        if (process->getAssignedCore() == coreId) {
            result.push_back(process);
        }
    }
    return result;
}

bool ProcessHandler::hasUnfinishedProcessOnCore(int coreId) {
    std::lock_guard<std::mutex> lock(processMutex);
    for (const auto& process : running) {
        if (process->getAssignedCore() == coreId && !process->isFinished()) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<Process> ProcessHandler::getFirstUnfinishedProcessOnCore(int coreId) {
    std::lock_guard<std::mutex> lock(processMutex);
    for (const auto& process : running) {
        if (process->getAssignedCore() == coreId && !process->isFinished()) {
            return process;
        }
    }
    return nullptr;
}

void ProcessHandler::markProcessFinished(int processId) {
    std::lock_guard<std::mutex> lock(processMutex);

    auto it = running.begin();
    while (it != running.end()) {
        if ((*it)->getId() == processId) {
            (*it)->setFinished(true);
            finished.push_back(*it);
            it = running.erase(it);  // Remove and continue
        }
        else {
            ++it;
        }
    }
}
