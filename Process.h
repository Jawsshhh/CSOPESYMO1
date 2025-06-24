#pragma once
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <vector>

class Process {
public:
    Process(const std::string& name, int id);
    ~Process();

    void executePrintCommand(int core, const std::string& screenName);

    std::string getName() const;
    std::string getCreationTime() const;
    int getId() const;
    void setAssignedCore(int core) { assignedCore = core; }
    int getAssignedCore() const { return assignedCore; }
    std::ofstream& getLogFile() { return logFile; }

    void executeNextInstruction();
    void addInstruction(const std::string& instruction);
    bool isFinished() const { return currentInstruction >= instructions.size(); }
    
    void writeHeader();

private:
    static std::mutex fileMutex;
    std::string name;
    int id;
    std::string creationTime;
    std::ofstream logFile;
    int printCount = 0;
    const int totalPrints = 100;
    int assignedCore = -1;
    std::vector<std::string> instructions;
    size_t currentInstruction = 0;
};