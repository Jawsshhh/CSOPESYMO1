#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include "Process.h"
#include <unordered_set>

class ProcessHandler {
public:
    void insertProcess(std::shared_ptr<Process> process);

    bool deleteProcess(int processId);

    void updateProcessState(int processId, bool isFinished);

    std::shared_ptr<Process> getProcess(int processId);

    std::vector<std::shared_ptr<Process>> getAllProcesses();

    std::vector<std::shared_ptr<Process>> getRunningProcesses();

    std::vector<std::shared_ptr<Process>> getFinishedProcesses();

    std::vector<std::shared_ptr<Process>> getProcessesByCore(int coreId);

    bool hasUnfinishedProcessOnCore(int coreId);
    std::shared_ptr<Process> getFirstUnfinishedProcessOnCore(int coreId);
    void markProcessFinished(int processId);
private:
    std::vector<std::shared_ptr<Process>> processes;
    mutable std::mutex processMutex; // mutable allows locking in const methods
};