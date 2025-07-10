#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <memory>

class Process;

class Instruction {
public:
    enum class InstructionType {
        PRINT,
        DECLARE,
        ADD,
        SUBTRACT,
        SLEEP,
        FOR
    };

    Instruction(Process* process, InstructionType instructionType);
    virtual ~Instruction() = default;

    InstructionType getInstructionType();
    virtual void execute();
    virtual std::string getDetails() const = 0;


protected:
    Process* process;
    InstructionType instructionType;
    std::vector<std::string> args;               // arguments: var names or values
    std::vector<Instruction> body;               // for FOR loops
    int repeatCount = 0;                         // for FOR loops
};

class PrintInstruction : public Instruction {
public:
    PrintInstruction(Process* process, const std::string& toPrint);  
    void execute() override;
    std::string getDetails() const override;

private:
    std::string toPrint;
};

class DeclareInstruction : public Instruction {
public:
    DeclareInstruction(Process* process, const std::string& varName, uint16_t value);
    void execute() override;
    std::string getDetails() const override;  // Add this line
    bool performDeclaration();

private:
    std::string varName;
    uint16_t value;  // Changed from std::string to uint16_t to match constructor
};

class AddInstruction : public Instruction {
public:
    AddInstruction(Process* process, const std::string& var1,
        const std::string& var2, const std::string& var3);
    void execute() override;
    std::string getDetails() const override;
    void add();

private:
    std::string var1;
    std::string var2;
    std::string var3;

    uint16_t getValue(const std::string& var);
    bool checkNumber(const std::string& s);
};

class SubtractInstruction : public Instruction {
public:
    SubtractInstruction(Process* process, const std::string& var1, const std::string& var2, const std::string& var3);
    void execute() override;
    uint16_t getValue(const std::string& var);
    bool checkNumber(const std::string& var);
    std::string getDetails() const override;
    void subtract();
private:
    std::string var1;
    std::string var2;
    std::string var3;
};

class SleepInstruction : public Instruction, public std::enable_shared_from_this<SleepInstruction> {
public:
    SleepInstruction(Process* process, uint8_t x);
    void execute() override;
    std::string getDetails() const override;
    std::vector<int> tickLog;    
    bool isSleeping = false;

private:
    uint8_t x;
};

class ForInstruction : public Instruction {
public:
    ForInstruction(Process* process, std::vector<Instruction> instructionList, int repeats);
    void execute() override;
private:
    std::vector<Instruction> instructionList;
};