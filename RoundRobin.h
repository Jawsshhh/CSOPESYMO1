#pragma once
#include "Scheduler.h"
#include <queue>

class RRScheduler : public Scheduler {
public:
    RRScheduler(int numCores, int quantum);
    ~RRScheduler() override;

    void addProcess(std::shared_ptr<Process> process) override;

private:
    void schedulerLoop() override;
    void workerLoop(int coreId) override;

    unsigned quantum;
    std::queue<std::shared_ptr<Process>> readyQueue;
    std::mutex queueMutex;
};