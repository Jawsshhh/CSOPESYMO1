#pragma once
#include "Process.h"
#include "ProcessHandler.h"
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <algorithm>

class Scheduler {
public:
    Scheduler(int numCores);
    virtual ~Scheduler();

    virtual void start();
    virtual void stop();
    virtual void addProcess(std::shared_ptr<Process> process) = 0;
    virtual void listProcesses();
    virtual void generateReport(const std::string& filename);

protected:
    int numCores;
    std::vector<std::thread> workerThreads;
    std::thread schedulerThread;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> running;
    std::vector<std::shared_ptr<Process>> runningProcesses;
    std::vector<std::shared_ptr<Process>> finishedProcesses;
    std::vector<bool> coreAvailable;
    ProcessHandler processHandler;
    std::vector<std::string> lastPrintedProcessLines;

    virtual void schedulerLoop() = 0;
    virtual void workerLoop(int coreId) = 0;
    int findAvailableCore() {
        for (int i = 0; i < numCores; ++i) {
            if (coreAvailable[i]) {
                return i;
            }
        }
        return -1; // No available core
    };
};