
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

    size_t getMemoryNeeded() const;
    std::string getName() const;
    std::string getCreationTime() const;
    int getId() const;
    void setAssignedCore(int core);
    int getAssignedCore() const;
    int getCurrentInstructionIndex() const;
    size_t getInstructionCount() const;
    int getMemorySize() const;

    void executeNextInstruction();
    void addInstruction(std::shared_ptr<Instruction> instruction);
    void setFinished(bool finished);
    bool isFinished() const;
    void setMaxExecutionDelay(int delay);
    static std::string instructionTypeToString(Instruction::InstructionType type);
    bool isSleeping() const;
    void setSleeping(bool state, uint8_t ticks = 0);
    int getRemainingSleepTicks() const;
    std::vector<std::string> getLogs() const;
    std::string getStatus() const;
    void logInstruction(const std::string& type, const std::string& details);

    void setCurrentSleepInstruction(std::shared_ptr<SleepInstruction> instr);
    std::shared_ptr<SleepInstruction> getCurrentSleepInstruction();

    SymbolTable& getSymbolTable();

    void assignPages(const std::vector<int>& pages);
    const std::vector<int>& getAssignedPages() const;

    std::chrono::system_clock::time_point getStartTime() const;

private:
    SymbolTable symbolTable;
    std::vector<std::shared_ptr<Instruction>> instructionList;
    std::vector<int> assignedPages;

    static std::mutex fileMutex;
    mutable std::mutex stateMutex;

    std::string name;
    int id;
    std::string creationTime;
    std::ofstream logFile;

    int assignedCore = -1;
    int currentInstruction = 0;
    std::atomic<bool> isFinishedFlag{ false };
    std::atomic<bool> sleeping{ false };
    std::atomic<int> remainingSleepTicks{ 0 };
    std::shared_ptr<SleepInstruction> currentSleepInstruction;

    size_t memoryRequired;
    int delayCount = 0;
    int maxExecDelay = 0;
    int memorySize = 0;  // Optional if needed
};
