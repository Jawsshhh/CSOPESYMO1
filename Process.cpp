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
    
    if (currentInstruction < instructionList.size()) {
        
        Instruction instr = instructionList[currentInstruction++];

        if (instr.getInstructionType() == Instruction::InstructionType::PRINT) {
            logInstruction("PRINT", instr.);
        }
        else if (instr.getInstructionType() == Instruction::InstructionType::DECLARE) {
            logInstruction("DECLARE", instr.substr(8));
        }
        else if (instr.getInstructionType() == Instruction::InstructionType::ADD) {
            logInstruction("ADD", instr.substr(4));
        }
        else if (instr.getInstructionType() == Instruction::InstructionType::SUBTRACT) {
            logInstruction("SUBTRACT", instr.substr(6));
        }
        else if (instr.getInstructionType() == Instruction::InstructionType::SLEEP) {
            logInstruction("SLEEP", instr.substr(6));
        }
        else if (instr.getInstructionType() == Instruction::InstructionType::FOR) {
            logInstruction("FOR", instr.substr(6));
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

void Process::addInstruction(Instruction instruction) {
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
