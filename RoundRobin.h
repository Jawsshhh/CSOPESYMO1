#pragma once
#include "Scheduler.h"
#include <queue>
#include <thread>
#include <algorithm>



class RRScheduler : public Scheduler {
public:
    RRScheduler(int numCores, int quantum, int delays_per_exec,
        size_t maxMemory, size_t frameSize);
    ~RRScheduler() override;

    void addProcess(std::shared_ptr<Process> process);

    bool tryAllocateProcessPages(std::shared_ptr<Process> process);

private:
    size_t frameSize;
    void schedulerLoop() override;
    void workerLoop(int coreId) override;
    int delays_per_exec;
    unsigned quantum;
    std::queue<std::shared_ptr<Process>> readyQueue;
    std::mutex queueMutex;
    std::atomic<int> nextCoreId = 0;
    std::mutex coreTurnMutex;
    std::atomic<int> quantumCycle = 0;
    std::mutex snapshotMutex;
    int globalPageCounter = 0;

    std::queue<std::shared_ptr<Process>> pendingProcesses;  // ADD THIS LINE

};