#include "DemandPaging.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>

DemandPagingAllocator::DemandPagingAllocator(size_t maxMemory, size_t frameSize)
    : totalFrames(maxMemory / frameSize), frameSize(frameSize) {
    frameTable.resize(totalFrames);

    // Clear log file at start of session
    std::ofstream clearLog("csopesy-backing-store.txt", std::ios::trunc);
    clearLog.close();
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
    for (int i = 0; i < frameTable.size(); ++i)
        if (frameTable[i].occupied) return i;
    return -1;
}

void DemandPagingAllocator::evict(int frameIdx) {
    auto& frame = frameTable[frameIdx];
    if (frame.occupied) {
        auto& pt = pageTables[frame.pid];
        if (frame.page < pt.size()) {
            pt[frame.page].valid = false;
            pt[frame.page].frameIndex = -1;
            pt[frame.page].dirty = false;

            writeToBackingStore(frame.pid, frame.page);  // Includes actual page data
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

    // Try to load actual page data
    pt[page].data = readPageDataFromBackingStore(pid, page);
    if (pt[page].data.empty()) {
        pt[page].data = "DefaultData_PID" + std::to_string(pid) + "_PAGE" + std::to_string(page);
    }

    frameTable[frameIdx] = { pid, page, true };
}


void DemandPagingAllocator::writeToBackingStore(int pid, int page) {
    const auto& pt = pageTables[pid];
    if (page >= pt.size()) return;

    const std::string& data = pt[page].data;

    std::ofstream store("csopesy-backing-store.txt", std::ios::app);
    if (store.is_open()) {
        store << "[PID:" << pid << " PAGE:" << page << "]\n";
        store << data << "\n\n";  // Use double newline as separator
        store.close();
    }
}


void DemandPagingAllocator::loadFromBackingStore(int pid, int page) {
    std::ifstream store("csopesy-backing-store.txt");
    std::string line;
    bool found = false;

    while (std::getline(store, line)) {
        if (line == "[PID:" + std::to_string(pid) + " PAGE:" + std::to_string(page) + "]") {
            found = true;
            break;
        }
    }
    store.close();

    // Optional: Log to a separate file instead, or keep this for visibility
    std::ofstream out("paging-log.txt", std::ios::app);  // Better not to corrupt csopesy-backing-store.txt
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);
    out << "[LOAD]  PID:" << pid << " PAGE:" << page << " @ "
        << std::put_time(&localTime, "%H:%M:%S") << (found ? "\n" : " [MISS]\n");
    out.close();
}




std::string DemandPagingAllocator::readPageDataFromBackingStore(int pid, int page) {
    std::ifstream file("csopesy-backing-store.txt");
    if (!file.is_open()) return "";

    std::string line, content;
    bool found = false;

    while (std::getline(file, line)) {
        if (line == "[PID:" + std::to_string(pid) + " PAGE:" + std::to_string(page) + "]") {
            found = true;
            while (std::getline(file, line) && !line.empty()) {
                content += line + "\n";
            }
            break;
        }
    }

    return found ? content : "";
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

size_t DemandPagingAllocator::getUsedMemory() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    size_t used = 0;
    for (const auto& frame : frameTable) {
        if (frame.occupied) used++;
    }
    return used * frameSize;
}

size_t DemandPagingAllocator::getFreeMemory() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    size_t free = 0;
    for (const auto& frame : frameTable) {
        if (!frame.occupied) free++;
    }
    return free * frameSize;
}

std::unordered_map<int, std::vector<PageTableEntry>>& DemandPagingAllocator::getPageTables() {
    return pageTables;
}
