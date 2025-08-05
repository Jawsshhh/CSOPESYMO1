#include "DemandPaging.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <queue>
#include <algorithm>
#include <limits>

DemandPagingAllocator::DemandPagingAllocator(size_t maxMemory, size_t frameSize,
    const std::string& backingStore)
    : totalFrames(maxMemory / frameSize),
    frameSize(frameSize),
    nextPageId(0),
    maxVirtualPages(static_cast<int>((maxMemory / frameSize) * 1.5)),
    backingStoreFile(backingStore) {

    frameTable.resize(totalFrames);

    // Clear both log files at startup
    std::ofstream clearBackingStore(backingStoreFile, std::ios::trunc);
    clearBackingStore.close();

    std::ofstream clearLog("paging-log.txt", std::ios::trunc);
    clearLog.close();

    // Initialize reusable page IDs
    for (int i = 0; i < maxVirtualPages; ++i) {
        reusablePageIds.push(i);
    }
}

bool DemandPagingAllocator::accessPage(int page) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = globalPageTable.find(page);
    if (it != globalPageTable.end() && it->second.valid) {
        it->second.lastUsed = time(nullptr);
        logPageOperation(page, "ACCESS");
        return true;
    }
    logPageOperation(page, "ACCESS_MISS");
    return false;
}

bool DemandPagingAllocator::pageFault(int page) {
    std::unique_lock<std::mutex> lock(memoryMutex);

    // Find a frame to use
    int frameIdx = findFreeFrame();
    if (frameIdx == -1) {
        frameIdx = selectVictim();
    }
    if (frameIdx == -1) {
        logPageOperation(page, "FAULT_FAILED", false);
        return false;
    }

    // Evict current page if frame is occupied
    evict(frameIdx);

    // Load the requested page
    bool success = loadPage(page, frameIdx);

 

    // Log the page fault operation
    logPageOperation(page, success ? "FAULT_SUCCESS" : "FAULT_FAILED", success);

    return success;
}

int DemandPagingAllocator::findFreeFrame() {
    for (int i = 0; i < frameTable.size(); ++i) {
        if (!frameTable[i].occupied) return i;
    }
    return -1;
}

int DemandPagingAllocator::selectVictim() {
    int oldest = -1;
    time_t oldestTime = std::numeric_limits<time_t>::max();

    for (int i = 0; i < frameTable.size(); ++i) {
        if (frameTable[i].occupied) {
            int page = frameTable[i].page;
            if (globalPageTable[page].lastUsed < oldestTime) {
                oldest = i;
                oldestTime = globalPageTable[page].lastUsed;
            }
        }
    }

    return oldest;
}

void DemandPagingAllocator::evict(int frameIdx) {
    auto& frame = frameTable[frameIdx];
    if (!frame.occupied) {
        return; // Nothing to evict
    }

    int page = frame.page;
    auto& entry = globalPageTable[page];

    // Mark page as invalid in memory
    entry.valid = false;
    entry.frameIndex = -1;

    // Write to backing store if page is dirty
    if (entry.dirty) {
        bool writeSuccess = writePageToBackingStore(page, entry.data, frameIdx);
        logPageOperation(page, writeSuccess ? "EVICT_WRITE" : "EVICT_WRITE_FAILED", writeSuccess);

        if (writeSuccess) {
            entry.dirty = false; // Mark as clean after successful write
            pagesOut++;
        }
    }
    else {
        logPageOperation(page, "EVICT_CLEAN");
        pagesOut++;
    }

    // Clear the frame
    frame.occupied = false;
    frame.page = -1;
}


bool DemandPagingAllocator::loadPage(int page, int frameIdx) {
    auto& entry = globalPageTable[page];

    // Try to read from backing store first
    std::string pageData = readPageFromBackingStore(page);

    if (pageData.empty()) {
        // Page not in backing store, create default data
        pageData = "DefaultData_PAGE" + std::to_string(page);
        logPageOperation(page, "LOAD_NEW");
    }
    else {
        logPageOperation(page, "LOAD_FROM_STORE");
    }

    // Update page table entry
    entry.valid = true;
    entry.frameIndex = frameIdx;
    entry.lastUsed = time(nullptr);
    entry.dirty = false;  // Just loaded, so not dirty yet
    entry.data = pageData;

    // Update frame table
    frameTable[frameIdx] = { page, true };
	pagesIn++;
    return true;
}

bool DemandPagingAllocator::writePageToBackingStore(int page, const std::string& data, int frameIdx) {
    std::lock_guard<std::mutex> lock(backingStoreMutex);

    try {
        // Read existing content
        std::ifstream inFile(backingStoreFile);
        std::vector<std::string> lines;
        std::string line;
        bool pageFound = false;

        // Read all lines and check if page already exists
        while (std::getline(inFile, line)) {
            lines.push_back(line);
        }
        inFile.close();

        // Look for existing page entry
        std::string pageHeader = "[PAGE:" + std::to_string(page) + "]";
        auto it = std::find(lines.begin(), lines.end(), pageHeader);

        if (it != lines.end()) {
            // Page exists, find the data section and update it
            size_t headerPos = std::distance(lines.begin(), it);

            // Look for the data line (should be after the header)
            for (size_t i = headerPos + 1; i < lines.size(); ++i) {
                if (lines[i].find("DATA:") == 0) {
                    // Update existing data line with frame information
                    lines[i] = "DATA:" + data;
                    pageFound = true;
                    break;
                }
                // If we hit another page header or empty line, insert data here
                if (lines[i].find("[PAGE:") == 0 || lines[i].empty()) {
                    lines.insert(lines.begin() + i, "DATA:" + data);
                    lines.insert(lines.begin() + i + 1, "EVICTED_FROM_FRAME:" + std::to_string(frameIdx));
                    pageFound = true;
                    break;
                }
            }

            // If we didn't find a data line, append it after the header
            if (!pageFound) {
                lines.insert(lines.begin() + headerPos + 1, "DATA:" + data);
                lines.insert(lines.begin() + headerPos + 2, "EVICTED_FROM_FRAME:" + std::to_string(frameIdx));
                pageFound = true;
            }
            else {
                // Update or add frame information
                bool frameInfoFound = false;
                for (size_t i = headerPos + 1; i < lines.size(); ++i) {
                    if (lines[i].find("EVICTED_FROM_FRAME:") == 0) {
                        lines[i] = "EVICTED_FROM_FRAME:" + std::to_string(frameIdx);
                        frameInfoFound = true;
                        break;
                    }
                    if (lines[i].find("[PAGE:") == 0) {
                        // Hit next page, insert frame info before it
                        lines.insert(lines.begin() + i, "EVICTED_FROM_FRAME:" + std::to_string(frameIdx));
                        frameInfoFound = true;
                        break;
                    }
                }
                if (!frameInfoFound) {
                    // Add frame info at the end
                    lines.push_back("EVICTED_FROM_FRAME:" + std::to_string(frameIdx));
                }
            }
        }

        // Write back to file
        std::ofstream outFile(backingStoreFile, std::ios::trunc);
        if (!outFile.is_open()) {
            return false;
        }

        // Write existing content
        for (const auto& existingLine : lines) {
            outFile << existingLine << "\n";
        }

        // If page wasn't found, append new entry
        if (!pageFound) {
            outFile << pageHeader << "\n";
            outFile << "DATA:" << data << "\n";
            outFile << "EVICTED_FROM_FRAME:" << frameIdx << "\n";
            outFile << "\n"; // Empty line separator
        }

        outFile.close();
        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "Error writing to backing store: " << e.what() << std::endl;
        return false;
    }
}
std::string DemandPagingAllocator::readPageFromBackingStore(int page) {
    std::lock_guard<std::mutex> lock(backingStoreMutex);

    try {
        std::ifstream file(backingStoreFile);
        if (!file.is_open()) {
            return "";
        }

        std::string line;
        std::string pageHeader = "[PAGE:" + std::to_string(page) + "]";

        while (std::getline(file, line)) {
            if (line == pageHeader) {
                // Found the page header, look for the data line
                while (std::getline(file, line)) {
                    if (line.find("DATA:") == 0) {
                        file.close();
                        return line.substr(5); // Remove "DATA:" prefix
                    }
                    // Stop if we hit another page header or empty line
                    if (line.find("[PAGE:") == 0 || line.empty()) {
                        break;
                    }
                }
                break;
            }
        }

        file.close();
        return ""; // Page not found

    }
    catch (const std::exception& e) {
        std::cerr << "Error reading from backing store: " << e.what() << std::endl;
        return "";
    }
}
bool DemandPagingAllocator::pageExistsInBackingStore(int page) {
    std::lock_guard<std::mutex> lock(backingStoreMutex);

    std::ifstream file(backingStoreFile);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string pageHeader = "[PAGE:" + std::to_string(page) + "]";

    while (std::getline(file, line)) {
        if (line == pageHeader) {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

void DemandPagingAllocator::logPageOperation(int page, const std::string& operation, bool success) {
    std::ofstream logFile("paging-log.txt", std::ios::app);
    if (!logFile.is_open()) {
        return;
    }

    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);

    logFile << "[" << operation << "] PAGE:" << page << " @ "
        << std::put_time(&localTime, "%H:%M:%S");

    if (!success) {
        logFile << " [FAILED]";
    }

    logFile << "\n";
    logFile.close();
}

void DemandPagingAllocator::registerProcessPages(int pid, const std::vector<int>& pages) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    processPageMap[pid] = pages;
    for (int page : pages) {
        globalPageTable[page];  // ensure entry exists
    }
}

void DemandPagingAllocator::generateSnapshot(const std::string& filename, int quantumCycle) {
    std::lock_guard<std::mutex> lock(memoryMutex);
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

void DemandPagingAllocator::initializePageData(int page, const std::string& data) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto& entry = globalPageTable[page];
    entry.data = data;
    entry.dirty = true;

    // DON'T write to backing store immediately during initialization
    // Only write when the page is actually evicted
    logPageOperation(page, "INIT_MEMORY_ONLY");
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

size_t DemandPagingAllocator::getFrameSize() const {
    return frameSize;
}

int DemandPagingAllocator::getNextGlobalPageId() {
    std::lock_guard<std::mutex> lock(memoryMutex);

    if (!reusablePageIds.empty()) {
        int id = reusablePageIds.front();
        reusablePageIds.pop();
        return id;
    }

    if (nextPageId >= maxVirtualPages) {
        throw std::runtime_error("Maximum virtual pages exceeded");
    }

    return nextPageId++;
}

void DemandPagingAllocator::releaseProcessPages(int pid) {
    std::lock_guard<std::mutex> lock(memoryMutex);

    auto it = processPageMap.find(pid);
    if (it == processPageMap.end()) {
        return; // Process not found
    }

    // Release each page assigned to this process
    for (int pageId : it->second) {
        releasePageInternal(pageId);  // Use internal method to avoid double locking
    }

    // Remove process from map
    processPageMap.erase(it);

    logPageOperation(-1, "PROCESS_" + std::to_string(pid) + "_RELEASED");
}

void DemandPagingAllocator::releasePageInternal(int pageId) {
    // Remove from global page table
    auto pageIt = globalPageTable.find(pageId);
    if (pageIt != globalPageTable.end()) {
        auto& entry = pageIt->second;

        // If page is in memory, free the frame
        if (entry.valid && entry.frameIndex != -1) {
            int frameIdx = entry.frameIndex;
            frameTable[frameIdx].occupied = false;
            frameTable[frameIdx].page = -1;
            logPageOperation(pageId, "FRAME_FREED");

            // If page is dirty, write to backing store before releasing
            if (entry.dirty) {
                bool writeSuccess = writePageToBackingStore(pageId, entry.data, frameIdx);
                logPageOperation(pageId, writeSuccess ? "FINAL_WRITE" : "FINAL_WRITE_FAILED");
                if (writeSuccess) {
                    pagesOut++;
                }
            }
        }
        else if (entry.dirty) {
            // Page not in memory but dirty, write without frame info (use -1)
            bool writeSuccess = writePageToBackingStore(pageId, entry.data, -1);
            logPageOperation(pageId, writeSuccess ? "FINAL_WRITE" : "FINAL_WRITE_FAILED");
            if (writeSuccess) {
                pagesOut++;
            }
        }

        // Remove from page table
        globalPageTable.erase(pageIt);
    }

    // Return page ID to reusable pool
    reusablePageIds.push(pageId);
    logPageOperation(pageId, "PAGE_RELEASED");
}

// FIXED: Use internal method to avoid race conditions
void DemandPagingAllocator::releasePage(int pageId) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    releasePageInternal(pageId);
}

long long DemandPagingAllocator::getPagesIn() const {
    return pagesIn.load();
}

long long DemandPagingAllocator::getPagesOut() const {
    return pagesOut.load();
}

