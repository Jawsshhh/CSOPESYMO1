#include "MemoryManager.h"
#include <fstream>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <iostream>

MemoryManager::MemoryManager(size_t maxMemory, size_t frameSize)
    : maxMemory(maxMemory), frameSize(frameSize) {
    memoryBlocks.push_back({ 0, maxMemory, false, -1 });
}

bool MemoryManager::allocateMemory(int processId, size_t memoryNeeded) {
    std::lock_guard<std::mutex> lock(memoryMutex);

    // Prevent allocating again if already allocated
    for (const auto& block : memoryBlocks) {
        if (block.allocated && block.processId == processId) {
            return false;
        }
    }

    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {
        if (!it->allocated && it->size >= memoryNeeded) {
            size_t originalStart = it->start;
            size_t originalSize = it->size;

            // Replace the current block with an allocated block
            MemoryBlock allocatedBlock = {
                originalStart,
                memoryNeeded,
                true,
                processId
            };

            // Create the remaining free block (if any)
            MemoryBlock remainingBlock = {
                originalStart + memoryNeeded,
                originalSize - memoryNeeded,
                false,
                -1
            };

            // Replace the current block
            it = memoryBlocks.erase(it);
            it = memoryBlocks.insert(it, allocatedBlock);

            // Insert the remaining free block if space is left
            if (remainingBlock.size > 0) {
                memoryBlocks.insert(std::next(it), remainingBlock);
            }

            return true;
        }
    }

    return false; // No free space found
}


void MemoryManager::deallocateMemory(int processId) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {
        if (it->allocated && it->processId == processId) {
            it->allocated = false;
            it->processId = -1;

            // Merge with previous block if it's free
            if (it != memoryBlocks.begin()) {
                auto prev = std::prev(it);
                if (!prev->allocated) {
                    prev->size += it->size;
                    it = memoryBlocks.erase(it);
                    it = prev; // Reset iterator to merged block
                }
            }

            // Merge with next block if it's free
            if (it + 1 != memoryBlocks.end() && !(it + 1)->allocated) {
                it->size += (it + 1)->size;
                memoryBlocks.erase(it + 1);
            }

            break;
        }
    }
}

size_t MemoryManager::calculateFragmentation() const {
    size_t fragmentation = 0;

    for (const auto& block : memoryBlocks) {
        if (!block.allocated) {
            fragmentation += block.size;
        }
    }

    return fragmentation;
}

void MemoryManager::generateMemorySnapshot(const std::string& filename, int quantumCycle) const {
    

    std::lock_guard<std::mutex> lock(memoryMutex);

   

    std::ofstream outFile(filename);

    if (!outFile.is_open()) {
        return;
    }

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    std::tm timeStruct;
    localtime_s(&timeStruct, &now_time);
    outFile << "Timestamp: " << std::put_time(&timeStruct, "%m/%d/%Y %I:%M:%S %p") << "\n";


    // Process count
    int processCount = 0;
    for (const auto& block : memoryBlocks) {
        if (block.allocated) ++processCount;
    }
    outFile << "Number of processes in memory: " << processCount << "\n";

    // Fragmentation
    size_t fragmentation = calculateFragmentation();
    outFile << "Total external fragmentation in KB: " << (fragmentation / 1024) << "\n";

    outFile << "---end--- = " << maxMemory << "\n";

    // Print in reverse (high to low)
    for (auto it = memoryBlocks.rbegin(); it != memoryBlocks.rend(); ++it) {
        outFile << it->start + it->size << "\n";
        if (it->allocated) {
            outFile << "P" << it->processId << "\n";
        }
        else {
            outFile << "---\n";
        }
    }

    outFile << "---start--- = 0\n";
    outFile.close();
}
