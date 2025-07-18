#include <iostream>
#include <string>
#include <cstdlib>
#include <memory>
using namespace std;

#include <map>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <climits>

#include "Scheduler.h"
#include "Process.h"
#include "FCFS.h"
#include "RoundRobin.h"
#include "CPUTick.h"

CPUTick cpuTick;

// In main.cpp
struct Config {
    int num_cpu = 0;
    std::string scheduler = "none";
    int quantum_cycles = 0;
    int batch_process_freq = 0;
    int min_ins = 0;
    int max_ins = 0;
    int delays_per_exec = 0;
    size_t max_overall_mem = 16384;  
    size_t mem_per_frame = 16;       
    size_t mem_per_proc = 4096;      
    bool initialized = false;
    std::atomic<bool> populate_running{ false };
    std::mutex populate_mutex;
    std::condition_variable populate_cv;
};
class ConsoleGeneral {
public:
    virtual void draw() { cout << "Print Terminal"; };
    void setName(const std::string& name) { this->name = name; };
    void setCurrentLine(const int currentLine) { this->currentLine = currentLine; };
    void setTotalLines(const int totalLines) { this->totalLines = totalLines; };
    void setTimestamp(const std::string& timestamp) { this->timestamp = timestamp; };
    string getName() const { return name; };
    int getCurrentLine() const { return currentLine; };
    int getTotalLines() const { return totalLines; };
    string getTimestamp() const { return timestamp; };

private:
    std::string name;
    int currentLine = 0;
    int totalLines = 0;
    std::string timestamp;
};

class MainConsole : public ConsoleGeneral {
public:
    void draw() override {
        cout << R"(  ____ ____   ___  ____  _____ ______   __
 / ___/ ___| / _ \|  _ \| ____/ ___\ \ / /
| |   \___ \| | | | |_) |  _| \___ \\ V / 
| |___ ___) | |_| |  __/| |___ ___) || |  
 \____|____/ \___/|_|   |_____|____/ |_|  )";

        cout << "\033[" << 32 << "m";
        cout << "\nHello, Welcome to CSOPESY commandLine!";
        cout << "\033[" << 33 << "m";
        cout << "\nType 'exit' to quit, 'clear' to clear the screen\n";
        cout << "\033[0m";
    };
};

class Screen : public ConsoleGeneral {
public:
    Screen() {
        setName("Unamed");
        setCurrentLine(1);
        setTotalLines(100);
        time_t now = time(nullptr);
        tm local;
        localtime_s(&local, &now);
        std::stringstream ss;
        ss << std::put_time(&local, "%m/%d/%Y, %I:%M:%S %p");
        setTimestamp(ss.str());
    };

    Screen(const std::string& name, int currentLine = 1, int totalLines = 100) {
        setName(name);
        setCurrentLine(currentLine);
        setTotalLines(totalLines);
        time_t now = time(nullptr);
        tm local;
        localtime_s(&local, &now);
        std::stringstream ss;
        ss << std::put_time(&local, "%m/%d/%Y, %I:%M:%S %p");
        setTimestamp(ss.str());
    };

    void draw() override {
        system("CLS");
        cout << "\n==== Screen Session ====\n";
        cout << "Process Name: " << getName() << "\n";
        cout << "Line: " << getCurrentLine() << "/" << getTotalLines() << "\n";
        cout << "Created On: " << getTimestamp() << "\n";
        cout << "========================\n";
        cout << "Type 'exit' to return to the main menu.\n\n";
        cout << "Enter a command: ";
    };
};

class ConsoleManager {
public:
    ConsoleManager() {
        screenSessions["MAIN"] = make_shared<MainConsole>();
        currentConsole = screenSessions["MAIN"];
    };

    void initializeScreen() {
        currentConsole->draw();
    };

    void destroyScreen() {
        system("CLS");
    };

    void addNewScreen(const std::string& name, std::shared_ptr<Process> process = nullptr) {
        screenSessions[name] = make_shared<Screen>(name, 1, 100);
        if (process) {
            screenProcesses[name] = process;
        }
        currentConsole = screenSessions[name];
    }
    std::shared_ptr<Process> getScreenProcess(const std::string& name) {
        if (screenProcesses.find(name) != screenProcesses.end()) {
            return screenProcesses[name];
        }
        return nullptr;
    }
    std::map<std::string, std::shared_ptr<Process>>& getAllScreenProcesses() {
        return screenProcesses;
    }
    void switchConsole(const std::string& name) {
        previousConsole = currentConsole;
        currentConsole = screenSessions[name];
    };

    void returnToPreviousConsole() {
        auto temporaryConsole = currentConsole;
        currentConsole = previousConsole;
        previousConsole = temporaryConsole;
    };

    bool findScreenSessions(const std::string& name) {
        if (screenSessions.find(name) != screenSessions.end()) {
            return true;
        }
        return false;
    }

    shared_ptr<ConsoleGeneral> getCurrentConsole() {
        return currentConsole;
    }


private:
    map<string, shared_ptr<ConsoleGeneral>> screenSessions;
    map<string, shared_ptr<Process>> screenProcesses;
    shared_ptr<ConsoleGeneral> currentConsole;
    shared_ptr<ConsoleGeneral> previousConsole;

};

string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

void populateProcesses(Config& config, ConsoleManager& consoleManager, unique_ptr<Scheduler>& scheduler) {
    static int processCounter = 0;  // Counter for unique process names

    while (config.populate_running) {
        // Create a new process with a unique name
        string processName = "process_" + to_string(processCounter++);

        // Create the process and screen
        auto process = make_shared<Process>(processName, processCounter, config.mem_per_proc);
        consoleManager.addNewScreen(processName, process);

        // Generate random number of instructions
        int numInstructions = config.min_ins + rand() % (config.max_ins - config.min_ins + 1);

        // Add instructions to the process
        for (int j = 0; j < numInstructions && config.populate_running; j++) {
            int instructionType = rand() % 4;
            switch (instructionType) {
            case 0: {
                auto printInstr = make_shared<PrintInstruction>(
                    process.get(),
                    "Hello from " + processName
                );
                process->addInstruction(printInstr);
                break;
            }
            case 1: {
                std::string varName = "var";
                uint16_t value = 10;
                auto declareInstr = make_shared<DeclareInstruction>(
                    process.get(),
                    varName,
                    value
                );
                process->addInstruction(declareInstr);
                break;
            }
            case 2: {
                std::string destVar = "0";
                std::string src1 = std::to_string(rand() % 50);
                std::string src2 = std::to_string(rand() % 50);

                auto addInstr = make_shared<AddInstruction>(
                    process.get(),
                    destVar,
                    src1,
                    src2
                );
                process->addInstruction(addInstr);
                break;
            }
            case 3: {
                std::string destVar = "var1";
                std::string src1 = std::to_string(rand() % 50);
                std::string src2 = std::to_string(rand() % 50);

                auto subInstr = make_shared<SubtractInstruction>(
                    process.get(),
                    destVar,
                    src1,
                    src2
                );
                process->addInstruction(subInstr);
                break;
            }
            
            }
        }

        if (config.populate_running) {
            try {
                scheduler->addProcess(process);
            }
            catch (const exception&) {

            }
        }

        // Wait for the configured delay or until stopped
        unique_lock<mutex> lock(config.populate_mutex);
        config.populate_cv.wait_for(lock,
            chrono::milliseconds(config.batch_process_freq),
            [&config] { return !config.populate_running; });
    }
}
int main() {
    Config config;
    string inputCommand;
    ConsoleManager consoleManager = ConsoleManager();
    unique_ptr<Scheduler> scheduler;
    consoleManager.initializeScreen();

    while (true) {

        cout << "Enter a command: ";

        getline(cin, inputCommand);
        if (inputCommand == "initialize") {
            string value;
            ifstream configFile("config.txt");
            if (configFile) {
                string line;
                while (getline(configFile, line)) {
                    
                    istringstream iss(line);
                    string key;
                    if (iss >> key) {
                        if (key == "num-cpu") iss >> config.num_cpu;
                        else if (key == "scheduler") iss >> config.scheduler;
                        else if (key == "quantum-cycles") iss >> config.quantum_cycles;
                        else if (key == "batch-process-freq") iss >> config.batch_process_freq;
                        else if (key == "min-ins") iss >> config.min_ins;
                        else if (key == "max-ins") iss >> config.max_ins;
                        else if (key == "delay-per-exec") iss >> config.delays_per_exec;
                        else if (key == "max-overall-mem") iss >> config.max_overall_mem;
                        else if (key == "mem-per-frame") iss >> config.mem_per_frame;
                        else if (key == "mem-per-proc") iss >> config.mem_per_proc;
                    }
                }
                config.initialized = true;
                cpuTick.start(config.batch_process_freq);

                cout << "System initialized with the following configuration:\n"
                    << "Number of CPUs: " << config.num_cpu << "\n"
                    << "Scheduler: " << config.scheduler << "\n"
                    << "Quantum cycles: " << config.quantum_cycles << "\n"
                    << "Batch process frequency: " << config.batch_process_freq << " CPU cycles\n"
                    << "Minimum instructions per process: " << config.min_ins << "\n"
                    << "Maximum instructions per process: " << config.max_ins << "\n"
                    << "Delay per execution: " << config.delays_per_exec << " CPU cycles\n";

                
                if (config.scheduler == "fcfs") {
                    scheduler = std::unique_ptr<Scheduler>(new FCFSScheduler(
                        config.num_cpu,
                        config.max_overall_mem,
                        config.mem_per_frame,
                        config.delays_per_exec));
                    scheduler->start();
                }
                else if (config.scheduler == "rr") {
                    scheduler = std::make_unique<RRScheduler>(
                        config.num_cpu,
                        config.quantum_cycles,
                        config.delays_per_exec,
                        config.max_overall_mem,
                        config.mem_per_frame
                        );
                    scheduler->start();
                }
            }
            else {
                cout << "Error: config.txt not found\n";
            }
        }

        else if (inputCommand.rfind("screen -s ", 0) == 0) {
            string name = inputCommand.substr(10);

            if (consoleManager.findScreenSessions(name)) {
                cout << "Screen already exists. Please type another name.\n";
            }
            else {
                static int processId = 0;
                auto process = make_shared<Process>(name, processId++, config.mem_per_proc);

                consoleManager.addNewScreen(name, process);
                consoleManager.initializeScreen();

                string subCommand;
                while (getline(cin, subCommand)) {
                    cout << "Enter command:> ";
                    if (subCommand == "exit") {
                        consoleManager.destroyScreen();
                        consoleManager.switchConsole("MAIN");
                        consoleManager.initializeScreen();
                        break;
                    }
                    else if (subCommand == "process-smi") {
                        cout << "Process name: " << process->getName() << "\n";
                        cout << "ID: " << process->getId() << "\n";
                        cout << "Messages:\n";  // Changed from "Logs:"

                        auto logs = process->getLogs();
                        for (const auto& log : logs) {
                            cout << log << "\n";  // All logs are PRINT messages now
                        }

                        if (process->isFinished()) {
                            cout << "Finished!\n";
                        }
                        else {
                            cout << "Current instruction: " << process->getCurrentInstructionIndex() + 1
                                << "/" << process->getInstructionCount() << "\n";
                        }
                    }
                    else {
                        cout << "Unknown screen command. Type 'exit' to return.\n";
                    }
                }
            }
        }
        else if (inputCommand.rfind("screen -r ", 0) == 0) {
            string name = inputCommand.substr(10);
            auto process = consoleManager.getScreenProcess(name);

            if (!process /*|| process->isFinished()*/) {
                cout << "Process " << name << " not found.\n";
                continue;
            }

            consoleManager.switchConsole(name);
            consoleManager.initializeScreen();

            string subCommand;
            while (getline(cin, subCommand)) {
                //cout << "Enter command:> ";
                if (subCommand == "exit") {
                    consoleManager.destroyScreen();
                    consoleManager.switchConsole("MAIN");
                    consoleManager.initializeScreen();
                    break;
                }
                else if (subCommand == "process-smi") {
                    cout << "===================\n";
                    cout << "Process name: " << process->getName() << "\n";
                    cout << "ID: " << process->getId() << "\n";
                    cout << "Messages:\n";  // Changed from "Logs:"

                    auto logs = process->getLogs();
                    for (const auto& log : logs) {
                        cout << log << "\n";  // All logs are PRINT messages now
                    }

                    if (process->isFinished()) {
                        cout << "Finished!\n";
                        cout << "===================\n";
                    }
                    else {
                        cout << "Current instruction: " << process->getCurrentInstructionIndex() + 1
                            << "/" << process->getInstructionCount() << "\n";
                    }
                    cout << "Enter command: ";
                }
                else {
                    cout << "Unknown screen command. Type 'exit' to return.\n";
                    
                }
                
            }
        }
            
        else if (inputCommand == "scheduler-start") {
            if (!config.initialized) {
                cout << "Error: System not initialized. Use 'initialize' first.\n";
                continue;
            }
            if (!scheduler) {
                cout << "Error: Scheduler not created. Use 'initialize' first.\n";
                continue;
            }
            if (config.populate_running) {
                cout << "Process population is already running.\n";
                continue;
            }

            config.populate_running = true;
            thread populate_thread(populateProcesses,
                ref(config),
                ref(consoleManager),
                ref(scheduler));
            populate_thread.detach();
            cout << "Started automatic process population (frequency: "
                << config.batch_process_freq << "ms)\n";
        }

        else if (inputCommand == "scheduler-stop") {
            if (!config.populate_running) {
                cout << "Process population is not currently running.\n";
                continue;
            }

            {
                lock_guard<mutex> lock(config.populate_mutex);
                config.populate_running = false;
            }
            config.populate_cv.notify_all();
            cout << "Stopped automatic process population.\n";
            }
        else if (inputCommand == "report-util") {
            if (!scheduler) {
                std::cout << "Error: Scheduler not initialized.\n";
            }
            else {
                scheduler->generateReport("csopesy.txt");
            }
        }

        else if (inputCommand == "screen -ls") {
            if(scheduler){
                scheduler->listProcesses();
            }
         }
        else if (inputCommand == "clear") {
            consoleManager.destroyScreen();
            consoleManager.switchConsole("MAIN");
            consoleManager.initializeScreen();
        }
        else if (inputCommand == "exit") {
            if (scheduler) {
                scheduler->stop();
            }

            cout << "Exiting the program.\n";
            break;
        }
        else {
            cout << "Unknown command. Try again.\n";
        }
    }
    cpuTick.stop();
    return 0;

}