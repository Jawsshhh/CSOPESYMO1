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



struct Config {
    int num_cpu = 0;
    std::string scheduler = "none";
    int quantum_cycles = 0;
    int batch_process_freq = 0;
    int min_ins = 0;
    int max_ins = 0;
    int delays_per_exec = 0;
    bool initialized = false;
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

    void addNewScreen(const std::string& name) {
        screenSessions[name] = make_shared<Screen>(name, 1, 100);
        currentConsole = screenSessions[name];
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
    shared_ptr<ConsoleGeneral> currentConsole;
    shared_ptr<ConsoleGeneral> previousConsole;

};

string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
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
                    }
                }
                config.initialized = true;

                cout << "System initialized with the following configuration:\n"
                    << "Number of CPUs: " << config.num_cpu << "\n"
                    << "Scheduler: " << config.scheduler << "\n"
                    << "Quantum cycles: " << config.quantum_cycles << "\n"
                    << "Batch process frequency: " << config.batch_process_freq << " CPU cycles\n"
                    << "Minimum instructions per process: " << config.min_ins << "\n"
                    << "Maximum instructions per process: " << config.max_ins << "\n"
                    << "Delay per execution: " << config.delays_per_exec << " CPU cycles\n";

                
                if (config.scheduler == "fcfs") {
                   scheduler = std::unique_ptr<Scheduler>(new FCFSScheduler(config.num_cpu));
                   scheduler->start();
                }
                else if (config.scheduler == "rr") {
                   cout << "RR Created";
                   scheduler = std::make_unique<RRScheduler>(config.num_cpu, config.quantum_cycles);
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
                // if the screen already exists
                cout << "Screen already exists. Please type another name.\n";
            }
            else {
                // create a new screen
                consoleManager.addNewScreen(name);
                consoleManager.initializeScreen();

                string subCommand;
                while (getline(cin, subCommand)) {
                    if (subCommand == "exit") {
                        consoleManager.destroyScreen();
                        consoleManager.switchConsole("MAIN");
                        consoleManager.initializeScreen();
                        break;
                    }
                    else if (subCommand == "print") {
                        for (int i = 0; i < 10; ++i) {
                           // scheduler.addProcess(std::make_shared<Process>(name, i));
                        }
                    }
                    else if (subCommand == "screen -ls") {
                      //  scheduler.listProcesses();
                    }
                    else {
                        cout << "Unknown screen command. Type 'exit' to return.\n";
                    }
                    cout << "Enter a command: ";
                }
            }

        }
        else if (inputCommand.rfind("screen -r ", 0) == 0) {
            string name = inputCommand.substr(10);
            if (consoleManager.findScreenSessions(name)) {
                consoleManager.switchConsole(name);
                consoleManager.initializeScreen();

                string subCommand;
                while (getline(cin, subCommand)) {
                    if (subCommand == "exit") {
                        consoleManager.destroyScreen();
                        consoleManager.switchConsole("MAIN");
                        consoleManager.initializeScreen();
                        break;
                    }
                    
                    else if (subCommand == "screen -ls") {
                        if (scheduler) {
                            scheduler->listProcesses();
                        }
                        else {
                            cout << "Error: Scheduler not initialized\n";
                        }
                    }
                    else {
                        cout << "Unknown screen command. Type 'exit' to return.\n";
                    }
                    cout << "Enter a command: ";
                }
            }
            else {
                cout << "No session named '" << name << "' found.\n";
            }
        }
            
        else if (inputCommand == "scheduler-test") {
            if (!config.initialized) {  // Check both initialization and scheduler
                cout << "Error: System not initialized. Use 'initialize' first.\n";
                continue;
            }
            else if (!scheduler) {
                cout << "Error: Scheduler not created. Use 'initialize' first.\n";
                continue;
            }

            cout << "Generating dummy processes...\n";

            for (int i = 0; i < 10; i++) {
                string processName = "dummy_" + to_string(i);
                auto process = make_shared<Process>(processName, i);

                int numInstructions = 100;
                for (int j = 0; j < numInstructions; j++) {
                    int instructionType = rand() % 4;
                    switch (instructionType) {
                    case 0:  // PRINT
                        process->addInstruction("PRINT \"Hello from " + processName + "\"");
                        break;
                    case 1:  // DECLARE
                        process->addInstruction("DECLARE var" + to_string(j) + " " + to_string(rand() % 100));
                        break;
                    case 2:  // ADD
                        process->addInstruction("ADD result var" + to_string(j) + " " + to_string(rand() % 50));
                        break;
                    case 3:  // SLEEP
                        process->addInstruction("SLEEP " + to_string(1 + rand() % 5));
                        break;
                    default:
                        process->addInstruction("PRINT \"Default instruction\"");
                        break;
                    }
                }

                try {
                    scheduler->addProcess(process);
                    cout << "Added process: " << processName
                        << " with " << numInstructions << " instructions\n";
                }
                catch (const exception& e) {
                    cerr << "Failed to add process " << processName
                        << ": " << e.what() << "\n";
                }
            }
        }
        else if (inputCommand == "scheduler-stop") {
            cout << "scheduler-stop command recognized. Doing something.\n";
        }
        else if (inputCommand == "report-util") {
            cout << "report-util command recognized. Doing something.\n";
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
    return 0;

}