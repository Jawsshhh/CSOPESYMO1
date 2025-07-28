#include "DemandPaging.h"

DemandPagingAllocator::DemandPagingAllocator(size_t maxMemory, size_t frameSize)
    : totalFrames(maxMemory / frameSize), frameSize(frameSize) {
    frameTable.resize(totalFrames);
}

bool DemandPagingAllocator::accessPage(int pid, int page) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto& pt = pageTables[pid];
    if (pt.size() <= page) pt.resize(page + 1);

    if (pt[page].valid) {
        pt[page].lastUsed = time(nullptr);
        return true;
    }
    return false;
}

void DemandPagingAllocator::pageFault(int pid, int page) {
    int frameIdx = findFreeFrame();
    if (frameIdx == -1) frameIdx = selectVictim();
    evict(frameIdx);
    loadPage(pid, page, frameIdx);
    loadFromBackingStore(pid, page);
}

int DemandPagingAllocator::findFreeFrame() {
    for (int i = 0; i < frameTable.size(); ++i)
        if (!frameTable[i].occupied) return i;
    return -1;
}

int DemandPagingAllocator::selectVictim() {
    // FIFO for simplicity: choose first occupied frame
    for (int i = 0; i < frameTable.size(); ++i)
        if (frameTable[i].occupied) return i;
    return -1; // Should not happen
}

void DemandPagingAllocator::evict(int frameIdx) {
    auto& frame = frameTable[frameIdx];
    if (frame.occupied) {
        auto& pt = pageTables[frame.pid];
        if (frame.page < pt.size()) {
            pt[frame.page].valid = false;
            pt[frame.page].frameIndex = -1;
            pt[frame.page].dirty = false;
            writeToBackingStore(frame.pid, frame.page);
        }
    }
    frame.occupied = false;
    frame.pid = -1;
    frame.page = -1;
}

void DemandPagingAllocator::loadPage(int pid, int page, int frameIdx) {
    auto& pt = pageTables[pid];
    if (pt.size() <= page) pt.resize(page + 1);

    pt[page].valid = true;
    pt[page].frameIndex = frameIdx;
    pt[page].lastUsed = time(nullptr);
    pt[page].dirty = false;

    frameTable[frameIdx] = { pid, page, true };
}

void DemandPagingAllocator::writeToBackingStore(int pid, int page) {
    std::ofstream store("csopesy-backing-store.txt", std::ios::app);
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);  // safer version
    store << "[EVICT] PID:" << pid << " PAGE:" << page << " @ " << std::put_time(&localTime, "%H:%M:%S") << "\n";
    store.close();
}

void DemandPagingAllocator::loadFromBackingStore(int pid, int page) {
    std::ofstream store("csopesy-backing-store.txt", std::ios::app);
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);  // safer version
    store << "[LOAD]  PID:" << pid << " PAGE:" << page << " @ " << std::put_time(&localTime, "%H:%M:%S") << "\n";
    store.close();
}


void DemandPagingAllocator::generateSnapshot(const std::string& filename, int quantumCycle) {
    std::ofstream out(filename);
    out << "[Snapshot @ Cycle " << quantumCycle << "]\n";
    for (int i = 0; i < frameTable.size(); ++i) {
        const auto& frame = frameTable[i];
        out << "Frame " << i << ": ";
        if (frame.occupied) {
            out << "PID " << frame.pid << " Page " << frame.page << "\n";
        }
        else {
            out << "Free\n";
        }
    }
    out.close();
}

std::unordered_map<int, std::vector<PageTableEntry>>& DemandPagingAllocator::getPageTables() {
    return pageTables;
}