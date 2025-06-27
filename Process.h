#pragma once
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <map>
#include <string>
#include <vector>
#include "Instruction.h"
#include "SymbolTable.h"

class Process {
public:
    Process(const std::string& name, int id);
    ~Process();

    std::string getName() const;
    std::string getCreationTime() const;
    int getId() const;
    void setAssignedCore(int core);
    int getAssignedCore() const;
    size_t getCurrentInstruction() const { return currentInstruction; }
    size_t getInstructionCount() const { return instructionList.size(); }
    void executeNextInstruction();
    void addInstruction(const std::string& instruction);
    void setFinished(bool finished) {
        std::lock_guard<std::mutex> lock(stateMutex);
        isFinishedFlag = finished;
    }
    bool isFinished() const {
        std::lock_guard<std::mutex> lock(stateMutex);
        return isFinishedFlag || currentInstruction >= instructionList.size();
    }
    void setMaxExecutionDelay(int delay);

    SymbolTable& getSymbolTable();


private:
    SymbolTable symbolTable;
    std::vector<Instruction> instructionList;
    static std::mutex fileMutex;
    void logInstruction(const std::string& type, const std::string& details);
    mutable std::mutex stateMutex;
    std::string name;
    int id;
    std::string creationTime;
    std::ofstream logFile;
    int assignedCore = -1;
    size_t currentInstruction = 0;
    std::atomic<bool> isFinishedFlag{ false };

    int delayCount = 0;
    int maxExecDelay = 0;
};