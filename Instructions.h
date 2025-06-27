#pragma once
#include <string>
#include <vector>
#include <sstream>

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
    AddInstruction(int pid, int var1, int var2, int var3);
    void execute() override;
private:
    int var1;
    int var2;
    int var3;
};

class SubtractInstruction : public Instruction {
public:
    SubtractInstruction(int pid, int var1, int var2, int var3);
    void execute() override;
private:
    int var1;
    int var2;
    int var3;
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