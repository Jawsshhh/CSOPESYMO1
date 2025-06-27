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
    int getCurrentInstructionIndex() const;
    size_t getInstructionCount() const;

    void executeNextInstruction();
    void addInstruction(Instruction instruction);
    void setFinished(bool finished);
    bool isFinished() const;
    void setMaxExecutionDelay(int delay);

    SymbolTable& getSymbolTable();


private:
    SymbolTable symbolTable;
    std::vector<std::shared_ptr<Instruction>> instructionList;
    static std::mutex fileMutex;
    void logInstruction(const std::string& type, const std::string& details);
    mutable std::mutex stateMutex;
    std::string name;
    int id;
    std::string creationTime;
    std::ofstream logFile;
    int assignedCore = -1;
    int currentInstruction = 0;
    std::atomic<bool> isFinishedFlag{ false };

    int delayCount = 0;
    int maxExecDelay = 0;
};