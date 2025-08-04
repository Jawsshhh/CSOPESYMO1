#pragma once
#include <queue>
#include <memory>
#include "Scheduler.h"


class FCFSScheduler : public Scheduler {
public:
    FCFSScheduler(int numCores, size_t maxMemory, size_t frameSize, int delays_per_exec);
    ~FCFSScheduler() override;

    void addProcess(const std::shared_ptr<Process> process);

private:
    void schedulerLoop() override;
    void workerLoop(int coreId) override;

    bool handleMemoryAccess(std::shared_ptr<Process> process);
    bool allocateMemoryForProcess(std::shared_ptr<Process> process);
    void deallocateMemoryForProcess(std::shared_ptr<Process> process);

    int delays_per_exec;
    std::queue<std::shared_ptr<Process>> processQueue;
    std::queue<std::shared_ptr<Process>> waitingQueue;
    std::mutex waitingQueueMutex;
};