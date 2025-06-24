#pragma once
#include <queue>
#include <memory>
#include "Scheduler.h"


class FCFSScheduler : public Scheduler {
public:
    FCFSScheduler(int numCores);
    ~FCFSScheduler() override;

    void addProcess(std::shared_ptr<Process> process) override;

private:
    void schedulerLoop() override;
    void workerLoop(int coreId) override;
    std::queue<std::shared_ptr<Process>> processQueue;
};