#include "Process.h"
#include <iostream>

std::mutex Process::fileMutex;

Process::Process(const std::string& name, int id) : name(name), id(id) {
    time_t now = time(nullptr);
    tm local;
    localtime_s(&local, &now);
    std::stringstream ss;
    ss << std::put_time(&local, "%m/%d/%Y %I:%M:%S%p");
    creationTime = ss.str();

    std::lock_guard<std::mutex> lock(fileMutex);
    logFile.open("process_" + std::to_string(id) + ".txt", std::ios::out | std::ios::trunc);
    logFile << "Process name: " << name << "\nLogs:\n";
}

Process::~Process() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (logFile.is_open()) {
        logFile.flush();  
        logFile.close();
    }
}

void Process::executePrintCommand(int core, const std::string& screenName) {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (printCount >= totalPrints) return;

    time_t now = time(nullptr);
    tm local;
    localtime_s(&local, &now);
    std::stringstream ss;
    ss << std::put_time(&local, "%m/%d/%Y %I:%M:%S%p");

    logFile << "(" << ss.str() << ") Core:" << core
        << " \"Hello world from " << getName() << "\"\n";
    logFile.flush();  // Explicit flush after each write
    printCount++;
}
int Process::getId() const {
    return id;
}
std::string Process::getName() const {
    return name;
}

std::string Process::getCreationTime() const {
    return creationTime;
}

void Process::writeHeader() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (printCount == 0 && logFile.is_open()) {
        logFile << "Process name: " << name << "\nLogs:\n";
    }
}

void Process::executeNextInstruction() {
    if (currentInstruction < instructions.size()) {
        const auto& instr = instructions[currentInstruction++];

        std::lock_guard<std::mutex> lock(fileMutex);
        logFile << "Executing: " << instr << "\n";

        if (instr.rfind("PRINT", 0) == 0) {
            logFile << "Hello world from " << getName() << "!\n";
        }
    }
}

void Process::addInstruction(const std::string& instruction) {
    instructions.push_back(instruction);
}