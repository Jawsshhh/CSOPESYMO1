#ifndef PROCESS_HANDLER_H
#define PROCESS_HANDLER_H

#include <vector>
#include <memory>
#include <mutex>
#include "Process.h"

class ProcessHandler {
private:
    std::vector<std::shared_ptr<Process>> running;   
    std::vector<std::shared_ptr<Process>> finished;  
    mutable std::mutex processMutex;

public:
    void insertProcess(const std::shared_ptr<Process>& process);
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

    std::vector<std::shared_ptr<Process>> getCurrentlyActiveProcessesPerCore(int numCores);

};

#endif
