#pragma once
#include <string>
#include <vector>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR
};

struct Instruction {
    InstructionType type;
    std::vector<std::string> args;               // arguments: var names or values
    std::vector<Instruction> body;               // for FOR loops
    int repeatCount = 0;                         // for FOR loops
};

class Instruction {
public:
    Instruction(int pid, InstructionType instructionType);
    InstructionType getInstructionType();
    virtual void execute();

protected:
    int pid;
    InstructionType instructionType;
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
    DeclareInstruction(int pid, std::string& varName, std::string& value);
    void execute() override;
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
protected:
};

class ForInstruction : public Instruction {
public:
protected:
};