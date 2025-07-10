
#pragma once
#include <vector>
#include <string>
#include <mutex>

class MemoryManager {
public:
    MemoryManager(size_t maxMemory, size_t frameSize, size_t procMemory);

    bool allocateMemory(int processId);
    void deallocateMemory(int processId);
    void generateMemorySnapshot(const std::string& filename, int quantumCycle) const;
    bool isInMemory(int pid) const;


private:
    struct MemoryBlock {
        size_t start;
        size_t size;
        bool allocated;
        int processId;
    };

    size_t maxMemory;
    size_t frameSize;
    size_t procMemory;
    std::vector<MemoryBlock> memoryBlocks;
    mutable std::mutex memoryMutex;

    size_t calculateFragmentation() const;
};