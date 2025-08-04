#include "Process.h"
#include "Instruction.h"
#include "DemandPaging.h"
#include <iostream>
#include <cmath>


std::mutex Process::fileMutex;

Process::Process(const std::string& name, int id, size_t memoryRequired)
    : name(name), id(id), memoryRequired(memoryRequired) {
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

void Process::assignPages(const std::vector<int>& pages) {
    assignedPages = pages;

    std::lock_guard<std::mutex> lock(fileMutex);
    if (logFile.is_open()) {
        logFile << "[PAGE ASSIGNMENT] Assigned " << pages.size() << " pages to process (Page IDs: ";
        for (size_t i = 0; i < pages.size(); ++i) {
            logFile << pages[i];
            if (i != pages.size() - 1) logFile << ", ";
        }
        logFile << ")\n";
        logFile.flush();
    }
}

const std::vector<int>& Process::getAssignedPages() const {
    return assignedPages;
}

std::chrono::system_clock::time_point Process::getStartTime() const
{
    return std::chrono::system_clock::time_point();
}


bool Process::executeNextInstruction() {
    if (isSleeping) {
        logInstruction("SLEEP", "SLEEP FOR " + std::to_string(remainingSleepCycles) + " CYCLES");
        remainingSleepCycles--;

        if (remainingSleepCycles <= 0) {
            isSleeping = false;
        }

        return false;
    }

    if (currentInstruction < static_cast<int>(instructionList.size())) {

        auto instr = instructionList[currentInstruction++];

        instr->execute();

        logInstruction(
            instructionTypeToString(instr->getInstructionType()),
            instr->getDetails()
        );

        return true;

    }

    if (currentInstruction >= static_cast<int>(instructionList.size())) {
        isFinished = true;
        return false;
    }

    return true;
}

std::string Process::getName() const { return name; }
int Process::getId() const { return id; }
std::string Process::getCreationTime() const { return creationTime; }
int Process::getCurrentInstructionIndex() const { return currentInstruction; }
size_t Process::getInstructionCount() const { return instructionList.size(); }
size_t Process::getMemoryNeeded() const { return memoryRequired; }

void Process::setAssignedCore(int core) { assignedCore = core; }
int Process::getAssignedCore() const { return assignedCore; }

void Process::setMaxExecutionDelay(int delay) { maxExecDelay = std::max(0, delay); }
SymbolTable& Process::getSymbolTable() { return symbolTable; }

void Process::logInstruction(const std::string& type, const std::string& details) {
    time_t now = time(nullptr);
    tm local;
    localtime_s(&local, &now);
    std::stringstream ss;
    ss << std::put_time(&local, "%m/%d/%Y %I:%M:%S%p");

    std::lock_guard<std::mutex> lock(fileMutex);
    logFile << "(" << ss.str() << ") Core:" << assignedCore
        << " \"" << details << "\"\n";
    logFile.flush();

}

void Process::addInstruction(std::shared_ptr<Instruction> instruction) {
    instructionList.push_back(instruction);
}

std::vector<std::string> Process::getLogs() const {
    std::vector<std::string> logs;
    std::string logFileName = "process_" + std::to_string(id) + ".txt";

    std::lock_guard<std::mutex> lock(fileMutex);
    std::ifstream logFile(logFileName);
    if (!logFile.is_open()) return logs;

    std::string line;
    while (std::getline(logFile, line)) {
        logs.push_back(line);
    }
    return logs;
}

std::string Process::instructionTypeToString(Instruction::InstructionType type) {
    switch (type) {
    case Instruction::InstructionType::PRINT:    return "PRINT";
    case Instruction::InstructionType::DECLARE:  return "DECLARE";
    case Instruction::InstructionType::ADD:      return "ADD";
    case Instruction::InstructionType::SUBTRACT: return "SUBTRACT";
    case Instruction::InstructionType::SLEEP:    return "SLEEP";
    case Instruction::InstructionType::FOR:      return "FOR";
    default: return "UNKNOWN";
    }
}

void Process::setIsSleeping(bool isSleeping, uint8_t sleepCycles)
{
    this->isSleeping = isSleeping;
    this->remainingSleepCycles = sleepCycles;
}

void Process::setIsFinished(bool isFinished)
{

    std::lock_guard<std::mutex> lock(stateMutex);
    this->isFinished = isFinished;
}





int Process::getIsSleeping() const {
    return isSleeping;
}

int Process::getIsFinished() const {
    return isFinished;
}

int Process::getRemainingSleepCycles() const
{
    return remainingSleepCycles;
}

int Process::getMemorySize() const {
    return memorySize;
}
