#pragma once
#include <queue>
#include <memory>
#include "Scheduler.h"


class FCFSScheduler : public Scheduler {
public:
    FCFSScheduler(int numCores, int delays_per_exec);
    ~FCFSScheduler() override;

    void addProcess(const std::shared_ptr<Process> process);

private:
    void schedulerLoop() override;
    int delays_per_exec;
    void workerLoop(int coreId) override;
    std::queue<std::shared_ptr<Process>> processQueue;
};