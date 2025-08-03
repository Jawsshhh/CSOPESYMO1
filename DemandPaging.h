#ifndef DEMAND_PAGING_H
#define DEMAND_PAGING_H

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>

class DemandPagingAllocator {
public:
    DemandPagingAllocator(size_t maxMemory, size_t frameSize, const std::string& backingStore = "csopesy-backing-store.txt");

    bool accessPage(int page);
    bool pageFault(int page);

    void registerProcessPages(int pid, const std::vector<int>& pages);
    void releaseProcessPages(int pid);
    void initializePageData(int page, const std::string& data);
    void generateSnapshot(const std::string& filename, int quantumCycle);

    size_t getUsedMemory() const;
    size_t getFreeMemory() const;
    size_t getFrameSize() const;
    int getNextGlobalPageId();

private:
    struct Frame {
        int page = -1;
        bool occupied = false;
    };

    struct PageTableEntry {
        bool valid = false;
        bool dirty = false;
        int frameIndex = -1;
        time_t lastUsed = 0;
        std::string data;
    };

    size_t totalFrames;
    size_t frameSize;
    int nextPageId;
    int maxVirtualPages;
    std::string backingStoreFile;

    std::vector<Frame> frameTable;
    std::unordered_map<int, PageTableEntry> globalPageTable;
    std::unordered_map<int, std::vector<int>> processPageMap;
    std::queue<int> reusablePageIds;

    mutable std::mutex memoryMutex;
    mutable std::mutex backingStoreMutex;

    int findFreeFrame();
    int selectVictim();
    void evict(int frameIdx);
    bool loadPage(int page, int frameIdx);
    bool writePageToBackingStore(int page, const std::string& data);
    std::string readPageFromBackingStore(int page);
    bool pageExistsInBackingStore(int page);
    void logPageOperation(int page, const std::string& operation, bool success = true);
    void releasePage(int pageId);
};

#endif // DEMAND_PAGING_H
