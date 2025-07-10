#pragma once
#include "Scheduler.h"
#include <queue>
#include <thread>

class RRScheduler : public Scheduler {
public:
    RRScheduler(int numCores, int quantum, int delays_per_exec,
        size_t maxMemory, size_t frameSize, size_t procMemory);
    ~RRScheduler() override;

    void addProcess(std::shared_ptr<Process> process);

private:
    void schedulerLoop() override;
    void workerLoop(int coreId) override;
    int delays_per_exec;
    unsigned quantum;
    std::queue<std::shared_ptr<Process>> readyQueue;
    std::mutex queueMutex;
};