#pragma once
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "Process.h"

class FCFSScheduler {
public:
    FCFSScheduler();
    ~FCFSScheduler();

    void start();
    void stop();
    void addProcess(std::shared_ptr<Process> process);
    void listProcesses();

private:
    void schedulerLoop();
    void workerLoop(int coreId);

    std::queue<std::shared_ptr<Process>> processQueue;
    std::vector<std::thread> workerThreads;
    std::thread schedulerThread;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> running;
    std::vector<std::shared_ptr<Process>> runningProcesses;
    std::vector<std::shared_ptr<Process>> finishedProcesses;
    std::vector<bool> coreAvailable;

};
