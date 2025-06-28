#pragma once
#include "Scheduler.h"
#include <queue>

class RRScheduler : public Scheduler {
public:
    RRScheduler(int numCores, int quantum, int delays_per_exec);
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