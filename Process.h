
#pragma once

#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <map>
#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include "Instruction.h"
#include "SymbolTable.h"

class Process {
public:
    Process(const std::string& name, int id, size_t memoryRequired);
    ~Process();

    bool executeNextInstruction();
    void addInstruction(std::shared_ptr<Instruction> instruction);

    std::vector<std::string> getLogs() const;
    void logInstruction(const std::string& type, const std::string& details);

    void assignPages(const std::vector<int>& pages);

    void setMaxExecutionDelay(int delay);
    void setAssignedCore(int core);
    void setSleeping(bool isSleeping, uint8_t sleepCycles);
    void setIsFinished(bool isFinished);

    SymbolTable& getSymbolTable();
    std::string getName() const;
    std::string getCreationTime() const;
    int getId() const;
    int getAssignedCore() const;
    int getCurrentInstructionIndex() const;
    size_t getInstructionCount() const;
    int getMemorySize() const;
    size_t getMemoryNeeded() const;
    int getMemorySize() const;
    int getIsSleeping() const;
    int getIsFinished() const;
    int getRemainingSleepCycles() const;
    const std::vector<int>& getAssignedPages() const;

    static std::string instructionTypeToString(Instruction::InstructionType type);

    void assignPages(const std::vector<int>& pages);
    const std::vector<int>& getAssignedPages() const;

    std::chrono::system_clock::time_point getStartTime() const;

private:
    SymbolTable symbolTable;
    std::vector<std::shared_ptr<Instruction>> instructionList;
    std::vector<int> assignedPages;
    
    std::string name;
    int id;
    std::string creationTime;
    std::ofstream logFile;
    size_t memoryRequired;

    int assignedCore = -1;
    int currentInstruction = 0;
    int memorySize = 0;

    static std::mutex fileMutex;
    mutable std::mutex stateMutex;
    
    bool isFinished = false;
    bool isSleeping = false;
    int remainingSleepCycles = 0;

    int delayCount = 0;
    int maxExecDelay = 0;
};