#include "Process.h"
#include "Instruction.h"
#include <iostream>

std::mutex Process::fileMutex;

Process::Process(const std::string& name, int id, size_t memoryRequired) : name(name), id(id), memoryRequired(memoryRequired) {
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

bool Process::executeNextInstruction() {

    // if the process is sleeping, will countdown the sleep cycles until it wakes up
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


void Process::logInstruction(const std::string& type, const std::string& details) {
    // Only log PRINT instructions
    //if (type == "PRINT") {
        time_t now = time(nullptr);
        tm local;
        localtime_s(&local, &now);
        std::stringstream ss;
        ss << std::put_time(&local, "%m/%d/%Y %I:%M:%S%p");

        std::lock_guard<std::mutex> lock(fileMutex);
        logFile << "(" << ss.str() << ") Core:" << assignedCore
            << " \"" << details << "\"\n";
        logFile.flush();
   // }

}

void Process::addInstruction(std::shared_ptr<Instruction> instruction) {
    instructionList.push_back(instruction);
}

std::vector<std::string> Process::getLogs() const {
    std::vector<std::string> logs;
    std::string logFileName = "process_" + std::to_string(id) + ".txt";

    std::lock_guard<std::mutex> lock(fileMutex);

    std::ifstream logFile(logFileName);
    if (!logFile.is_open()) {
        return logs;  
    }

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

size_t Process::getMemoryNeeded() const {
    return memoryRequired;
}

int Process::getId() const {
    return id;
}

void Process::setAssignedCore(int core)
{
    assignedCore = core;
}

void Process::setSleeping(bool isSleeping, uint8_t sleepCycles)
{
    this->isSleeping = isSleeping;
    this->remainingSleepCycles = sleepCycles;
}

void Process::setIsFinished(bool isFinished)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    this->isFinished = isFinished;
}

int Process::getAssignedCore() const
{
    return assignedCore;
}

int Process::getCurrentInstructionIndex() const
{
    return currentInstruction;
}

size_t Process::getInstructionCount() const
{
    return instructionList.size();
}

void Process::setMaxExecutionDelay(int delay)
{
    maxExecDelay = std::max(0, delay);
}

size_t Process::getMemoryNeeded() const 
{ 
    return memoryRequired; 
}

SymbolTable& Process::getSymbolTable()
{
    return symbolTable;
}

std::string Process::getName() const {
    return name;
}

std::string Process::getCreationTime() const {
    return creationTime;
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

const std::vector<int>& Process::getAssignedPages() const
{
    return assignedPages;
}

int Process::getMemorySize() const {
    return memorySize;
}
