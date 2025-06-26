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
    logFile.flush();
}
Process::~Process() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (logFile.is_open()) {
        logFile.flush();  
        logFile.close();
    }
}


int Process::getId() const {
    return id;
}

void Process::setAssignedCore(int core)
{
    assignedCore = core;
}

int Process::getAssignedCore() const
{
    return assignedCore;
}

bool Process::isFinished() const
{
    return currentInstruction >= instructions.size();
}

void Process::setMaxExecutionDelay(int delay)
{
    maxExecDelay = std::max(0, delay);
}

std::string Process::getName() const {
    return name;
}

std::string Process::getCreationTime() const {
    return creationTime;
}


void Process::executeNextInstruction() {
    if (delayCount > 0) {
        delayCount--;
        return;
    }
    
    if (currentInstruction < instructions.size()) {
        const auto& instr = instructions[currentInstruction++];

        if (instr.rfind("PRINT", 0) == 0) {
            logInstruction("PRINT", instr.substr(instr.find('"') + 1, instr.find_last_of('"') - instr.find('"') - 1));
        }
        else if (instr.rfind("DECLARE", 0) == 0) {
            logInstruction("DECLARE", instr.substr(8));
        }
        else if (instr.rfind("ADD", 0) == 0) {
            logInstruction("ADD", instr.substr(4));
        }
        else if (instr.rfind("SLEEP", 0) == 0) {
            logInstruction("SLEEP", instr.substr(6));
        }
        delayCount = maxExecDelay;
    }
}

void Process::logInstruction(const std::string& type, const std::string& details) {
    time_t now = time(nullptr);
    tm local;
    localtime_s(&local, &now);
    std::stringstream ss;
    ss << std::put_time(&local, "%m/%d/%Y %I:%M:%S%p");

    std::lock_guard<std::mutex> lock(fileMutex);
    logFile << "(" << ss.str() << ") Core:" << assignedCore 
            << " \"" << type << " " << details << "\"\n";
    logFile.flush();
}

void Process::addInstruction(const std::string& instruction) {
    instructions.push_back(instruction);
}