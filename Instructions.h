#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

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

    Instruction(int pid, InstructionType instructionType);
    InstructionType getInstructionType();
    virtual void execute();


protected:
    int pid;
    InstructionType instructionType;
    std::vector<std::string> args;               // arguments: var names or values
    std::vector<Instruction> body;               // for FOR loops
    int repeatCount = 0;                         // for FOR loops
};

class PrintInstruction : public Instruction {
public:
    PrintInstruction(int pid, std::string& toPrint);
    void execute() override;
private:
    std::string toPrint;
};

class DeclareInstruction : public Instruction {
public:
    DeclareInstruction(int pid, std::string& varName, uint16_t value);
    void execute() override;
    void performDeclaration();
private:
    std::string varName;
    std::string value;
};

class AddInstruction : public Instruction {
public:
    AddInstruction(int pid, const std::string& var1, const std::string& var2, const std::string& var3);
    void execute() override;
    uint16_t getValue(const std::string& var);
    bool checkNumber(const std::string& var);
    void add(const std::string& var1, const std::string& var2, const std::string& var3);
private:
    std::string var1 = "";
    std::string var2 = "";
    std::string var3 = "";
};

class SubtractInstruction : public Instruction {
public:
    SubtractInstruction(int pid, const std::string& var1, const std::string& var2, const std::string& var3);
    void execute() override;
    uint16_t getValue();
    bool checkNumber();
    void subtract();
private:
    std::string var1;
    std::string var2;
    std::string var3;
};

class SleepInstruction : public Instruction {
public:
    SleepInstruction(int pid, uint8_t x);
    void execute() override;
private:
    uint8_t x;
};

class ForInstruction : public Instruction {
public:
    ForInstruction(int pid, std::vector<Instruction> instructionList, int repeats);
    void execute() override;
private:
    std::vector<Instruction> instructionList;
};