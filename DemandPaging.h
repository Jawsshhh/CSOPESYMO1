#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <queue>
struct PageTableEntry {
    bool valid = false;
    int frameIndex = -1;
    bool dirty = false;
    time_t lastUsed = 0;
    std::string data;
};

struct Frame {
    int page = -1;
    bool occupied = false;
};

class DemandPagingAllocator {
public:
    DemandPagingAllocator(size_t maxMemory, size_t frameSize);

    bool accessPage(int page);
    bool pageFault(int page);

    void registerProcessPages(int pid, const std::vector<int>& pages);
    void generateSnapshot(const std::string& filename, int quantumCycle);
    void initializePageData(int page, const std::string& data);


    size_t getUsedMemory() const;
    size_t getFreeMemory() const;

    int getNextGlobalPageId();

    size_t getFrameSize() const;


private:

    size_t frameSize;

    int nextPageId;
    std::queue<int> reusablePageIds;
    const int maxVirtualPages;

    int findFreeFrame();
    int selectVictim();
    void evict(int frameIdx);
    bool loadPage(int page, int frameIdx);

    void writeToBackingStore(int page);
    void loadFromBackingStore(int page);
    std::string readPageDataFromBackingStore(int page);

    int totalFrames;

    std::vector<Frame> frameTable;
    std::unordered_map<int, PageTableEntry> globalPageTable;

    std::unordered_map<int, std::vector<int>> processPageMap;
    mutable std::mutex memoryMutex;
};
