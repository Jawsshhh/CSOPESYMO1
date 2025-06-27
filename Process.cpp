#include "Process.h"
#include "Instruction.h"
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


void Process::executeNextInstruction() {
    if (delayCount > 0) {
        delayCount--;
        return;
    }

    if (currentInstruction < static_cast<int>(instructionList.size())) {
        auto instr = instructionList[currentInstruction++];

        // Execute first to ensure any state changes happen
        instr->execute();

        // Log after execution with full details
        logInstruction(
            instructionTypeToString(instr->getInstructionType()),
            instr->getDetails()
        );

        delayCount = maxExecDelay;
    }
}

void Process::logInstruction(const std::string& type, const std::string& details) {
    // Only log PRINT instructions
    if (type == "PRINT") {
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
}

void Process::addInstruction(std::shared_ptr<Instruction> instruction) {
    instructionList.push_back(instruction);
}

void Process::setFinished(bool finished)
{
     std::lock_guard<std::mutex> lock(stateMutex);
     isFinishedFlag = finished;
}

bool Process::isFinished() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return isFinishedFlag || currentInstruction >= instructionList.size();
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

std::string Process::getStatus() const {
    return isFinished() ? "Finished!" : "Running";
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
