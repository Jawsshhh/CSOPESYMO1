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

   /* std::lock_guard<std::mutex> lock(fileMutex);
    logFile.open("process_" + std::to_string(id) + ".txt", std::ios::out | std::ios::trunc);
    logFile << "Process name: " << name << "\nLogs:\n";
    logFile.flush();*/

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

void Process::logInstruction(const std::string& type, const std::string& details) {
   
    /*time_t now = time(nullptr);
    tm local;
    localtime_s(&local, &now);
    std::stringstream ss;
    ss << std::put_time(&local, "%m/%d/%Y %I:%M:%S%p");

    std::lock_guard<std::mutex> lock(fileMutex);
    logFile << "(" << ss.str() << ") Core:" << assignedCore
        << " \"" << details << "\"\n";
    logFile.flush();*/

    time_t now = time(nullptr);
    tm local;
    localtime_s(&local, &now);
    std::stringstream ss;
    ss << std::put_time(&local, "%m/%d/%Y %I:%M:%S%p");

    std::stringstream logEntry;
    logEntry << "(" << ss.str() << ") Core:" << assignedCore
        << " \"" << details << "\"";

    logs.push_back(logEntry.str());
    
}

void Process::addInstruction(std::shared_ptr<Instruction> instruction) {
    instructionList.push_back(instruction);
}

std::vector<std::string> Process::getLogs() const {
    /*std::vector<std::string> logs;
    std::string logFileName = "process_" + std::to_string(id) + ".txt";

    std::lock_guard<std::mutex> lock(fileMutex);
    std::ifstream logFile(logFileName);
    if (!logFile.is_open()) return logs;

    std::string line;
    while (std::getline(logFile, line)) {
        logs.push_back(line);
    }*/
    return logs;
}

void Process::setMemoryAccessViolation(bool violation, const std::string& address) {
    hasMemoryViolation = violation;
    violationAddress = address;

    if (violation) {
        time_t now = time(nullptr);
        tm local;
        localtime_s(&local, &now);
        std::stringstream ss;
        ss << std::put_time(&local, "%H:%M:%S");
        violationTime = ss.str();

        setIsFinished(true);

        logs.push_back("Process " + name + "shut down due to memory access violation error that occurred at " +
            violationTime + ". " + address + " invalid.");
    }
}

std::string Process::getMemoryViolationDetails() const {
    if (hasMemoryViolation) {
        return "Process " + name + " shut down due to memory access violation error that occurred at "
            + violationTime + ". " + violationAddress + " invalid.";
    }
    return "";
}

bool Process::canAddVariable() const {
    return symbolTable.getSymbolTable().size() < SYMBOL_TABLE_MAX_SIZE;
}

void Process::setCurrentForLoop(std::shared_ptr<ForInstruction> forLoop) {
    currentForLoop = forLoop;
    isInForLoop = true;
}

void Process::clearForLoop() {
    currentForLoop.reset();
    isInForLoop = false;
}

std::string Process::timestamp() const {
    time_t now = time(nullptr);
    tm local;
    localtime_s(&local, &now);
    std::stringstream ss;
    ss << std::put_time(&local, "%H:%M:%S");
    return ss.str();
}

bool Process::shouldWakeUp() const
{
    return isSleeping && remainingSleepCycles == 0;
}

bool Process::executeNextInstruction() {
    if (hasMemoryViolation) {
        logInstruction(
            "MEMORY VIOLATION",
            getMemoryViolationDetails()
        );
        return false;
    }

    if (isSleeping) {
        --remainingSleepCycles;
        if (remainingSleepCycles > 0) {
            return true;  
        }

        isSleeping = false;
        return true;
    }

    if (isInForLoop && currentForLoop) {
        currentForLoop->execute();

        if (currentForLoop->isLoopComplete()) {
            clearForLoop();
            currentInstruction++; 
        }

        return currentInstruction < static_cast<int>(instructionList.size()) || isInForLoop;
    }


    if (currentInstruction < static_cast<int>(instructionList.size())) {
        auto instr = instructionList[currentInstruction++];

        if (instr->getInstructionType() == Instruction::InstructionType::FOR) {
            auto forInstr = std::dynamic_pointer_cast<ForInstruction>(instr);
            if (forInstr) {
                setCurrentForLoop(forInstr);
                forInstr->execute();

                logInstruction("FOR", forInstr->getDetails());

                return true;
            }
        }

        currentInstruction++;

        if (instr->getInstructionType() == Instruction::InstructionType::DECLARE) {
            if (!canAddVariable()) {
                logInstruction("DECLARE", "IGNORED - Symbol table full (32 variables max)");
                return currentInstruction < static_cast<int>(instructionList.size());
            }
        }

        instr->execute();

        if (hasMemoryViolation) {
            return false;
        }

        logInstruction(
            instructionTypeToString(instr->getInstructionType()),
            instr->getDetails()
        );

        return currentInstruction < static_cast<int>(instructionList.size());
    }

    if (currentInstruction >= static_cast<int>(instructionList.size())) {
        isFinished = true;
        return false;
    }

    return true;
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

const std::vector<int>& Process::getAssignedPages() const { return assignedPages; }
std::chrono::system_clock::time_point Process::getStartTime() const { return std::chrono::system_clock::time_point(); }
void Process::setIsSleeping(bool isSleeping, uint8_t sleepCycles)
{ this->isSleeping = isSleeping; this->remainingSleepCycles = sleepCycles; }
void Process::setIsFinished(bool isFinished)
{ std::lock_guard<std::mutex> lock(stateMutex); this->isFinished = isFinished; }
int Process::getIsSleeping() const { return isSleeping; }
int Process::getIsFinished() const { return isFinished; }
int Process::getRemainingSleepCycles() const { return remainingSleepCycles; }
int Process::getMemorySize() const { return memorySize; }
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
std::unordered_map<size_t, uint16_t>& Process::getMemoryMap() { return memoryMap; }
bool Process::hasMemoryAccessViolation() const { return hasMemoryViolation; }
size_t Process::getBaseMemoryAddress() const { return baseMemoryAddress; }
void Process::setBaseMemoryAddress(size_t address) { baseMemoryAddress = address; }
bool Process::getIsInForLoop() const { return isInForLoop; }
std::shared_ptr<ForInstruction> Process::getCurrentForLoop() const { return currentForLoop; }
