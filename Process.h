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
//#include "Instruction.h"
//#include "SymbolTable.h"

class Process {
public:
    Process(const std::string& name, int id);
    ~Process();

    std::string getName() const;
    std::string getCreationTime() const;
    int getId() const;
    void setAssignedCore(int core);
    int getAssignedCore() const;

    void executeNextInstruction();
    void addInstruction(const std::string& instruction);
    bool isFinished() const;
    void setMaxExecutionDelay(int delay);


private:
    static std::mutex fileMutex;
    void logInstruction(const std::string& type, const std::string& details);

    std::string name;
    int id;
    std::string creationTime;
    std::ofstream logFile;
    int assignedCore = -1;
    std::vector<std::string> instructions;
    size_t currentInstruction = 0;

    int delayCount = 0;        
    int maxExecDelay = 0;
};