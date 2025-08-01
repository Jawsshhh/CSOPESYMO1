#include "DemandPaging.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>

DemandPagingAllocator::DemandPagingAllocator(size_t maxMemory, size_t frameSize)
    : totalFrames(maxMemory / frameSize), frameSize(frameSize) {
    frameTable.resize(totalFrames);
    std::ofstream clearLog("csopesy-backing-store.txt", std::ios::trunc);
    clearLog.close();
}

bool DemandPagingAllocator::accessPage(int page) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = globalPageTable.find(page);
    if (it != globalPageTable.end() && it->second.valid) {
        it->second.lastUsed = time(nullptr);
        return true;
    }
    return false;
}

bool DemandPagingAllocator::pageFault(int page) {
    int frameIdx = findFreeFrame();
    if (frameIdx == -1) frameIdx = selectVictim();
    if (frameIdx == -1) return false;

    evict(frameIdx);
    bool success = loadPage(page, frameIdx);
    loadFromBackingStore(page);
    return success;
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
        auto& entry = globalPageTable[frame.page];
        entry.valid = false;
        entry.frameIndex = -1;
        entry.dirty = false;
        writeToBackingStore(frame.page);
    }
    frame.occupied = false;
    frame.page = -1;
}

bool DemandPagingAllocator::loadPage(int page, int frameIdx) {
    auto& entry = globalPageTable[page];

    entry.valid = true;
    entry.frameIndex = frameIdx;
    entry.lastUsed = time(nullptr);
    entry.dirty = false;

    entry.data = readPageDataFromBackingStore(page);
    if (entry.data.empty()) {
        entry.data = "DefaultData_PAGE" + std::to_string(page);
    }

    frameTable[frameIdx] = { page, true };
    return true;
}

void DemandPagingAllocator::writeToBackingStore(int page) {
    const auto& entry = globalPageTable[page];
    std::ofstream store("csopesy-backing-store.txt", std::ios::app);
    if (store.is_open()) {
        store << "[PAGE:" << page << "]\n";
        store << entry.data << "\n\n";
        store.close();
    }
}

void DemandPagingAllocator::loadFromBackingStore(int page) {
    std::ifstream store("csopesy-backing-store.txt");
    std::string line;
    bool found = false;

    while (std::getline(store, line)) {
        if (line == "[PAGE:" + std::to_string(page) + "]") {
            found = true;
            break;
        }
    }
    store.close();

    std::ofstream out("paging-log.txt", std::ios::app);
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);
    out << "[LOAD]  PAGE:" << page << " @ "
        << std::put_time(&localTime, "%H:%M:%S") << (found ? "" : " [MISS]") << "\n";
    out.close();
}

std::string DemandPagingAllocator::readPageDataFromBackingStore(int page) {
    std::ifstream file("csopesy-backing-store.txt");
    if (!file.is_open()) return "";

    std::string line, content;
    bool found = false;

    while (std::getline(file, line)) {
        if (line == "[PAGE:" + std::to_string(page) + "]") {
            found = true;
            while (std::getline(file, line) && !line.empty()) {
                content += line + "\n";
            }
            break;
        }
    }

    return found ? content : "";
}

void DemandPagingAllocator::registerProcessPages(int pid, const std::vector<int>& pages) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    processPageMap[pid] = pages;
    for (int page : pages) {
        globalPageTable[page];  // ensure entry exists
    }
}

void DemandPagingAllocator::generateSnapshot(const std::string& filename, int quantumCycle) {
    std::ofstream out(filename);
    out << "[Snapshot @ Cycle " << quantumCycle << "]\n";
    for (int i = 0; i < frameTable.size(); ++i) {
        const auto& frame = frameTable[i];
        out << "Frame " << i << ": ";
        if (frame.occupied) {
            out << "Page " << frame.page << "\n";
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
