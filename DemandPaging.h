#pragma once

#include <unordered_map>
#include <vector>
#include <fstream>
#include <ctime>
#include <mutex>
#include <string>
#include <sstream>
#include <iomanip>

struct PageTableEntry {
    bool valid = false;
    int frameIndex = -1;
    bool dirty = false;
    time_t lastUsed = 0;
};

struct FrameInfo {
    int pid = -1;
    int page = -1;
    bool occupied = false;
};

class DemandPagingAllocator {
public:
    DemandPagingAllocator(size_t maxMemory, size_t frameSize);

    bool accessPage(int pid, int page);
    void pageFault(int pid, int page);
    void generateSnapshot(const std::string& filename, int quantumCycle);

    void writeToBackingStore(int pid, int page);
    void loadFromBackingStore(int pid, int page);

    std::unordered_map<int, std::vector<PageTableEntry>>& getPageTables();

private:
    int findFreeFrame();
    int selectVictim();
    void evict(int frameIdx);
    void loadPage(int pid, int page, int frameIdx);

    size_t totalFrames;
    size_t frameSize;
    std::vector<FrameInfo> frameTable;
    std::unordered_map<int, std::vector<PageTableEntry>> pageTables;
    std::mutex memoryMutex;
};
#pragma once
