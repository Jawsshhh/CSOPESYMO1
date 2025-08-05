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
        FOR,
        READ,
        WRITE,
    };

    Instruction(Process* process, InstructionType instructionType);
    virtual ~Instruction() = default;

    InstructionType getInstructionType();
    virtual void execute();
    virtual std::string getDetails() const;


protected:
    Process* process;
    InstructionType instructionType;
    std::vector<std::string> args;              
    std::vector<Instruction> body;              
    int repeatCount = 0;                        
};

class ReadInstruction : public Instruction {
public:
    ReadInstruction(Process* process, const std::string& varName, const std::string& memoryAddress);
    void execute() override;
    std::string getDetails() const override;
    bool read();

private:
    std::string varName;
    std::string memoryAddress;

    uint16_t readFromMemoryAddress(size_t address);
    bool isValidMemoryAddress(size_t address);
};

class WriteInstruction : public Instruction {
public:
    WriteInstruction(Process* process, const std::string& memoryAddress, const std::string& value);
    void execute() override;
    std::string getDetails() const override;
    bool write();
    uint16_t getValue(const std::string& val);
    
    bool checkNumber(const std::string& val);

private:
    std::string value;
    std::string memoryAddress;

    void writeToMemoryAddress(size_t address, uint16_t value);
    bool isValidMemoryAddress(size_t address);
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
    std::string getDetails() const override;  
    bool performDeclaration();

private:
    std::string varName;
    uint16_t value;  
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

class SleepInstruction : public Instruction {
public:
    SleepInstruction(Process* process, std::string sleepCycles);
    void execute() override;
    void sleep();
    std::string getDetails() const override;

private:
    std::string sleepCycles;
};

class ForInstruction : public Instruction {
public:
    ForInstruction(Process* process, const std::vector<std::shared_ptr<Instruction>>& instructionList, int repeatCount);
    void execute() override;
    bool executeNextInLoop();
    bool isLoopComplete() const;
    void resetLoop();
    std::string getDetails() const override;

private:
    std::vector<std::shared_ptr<Instruction>> instructionList;
    int repeatCount;
    int currentIteration;
    bool isExecuting;
    int currentInstructionIndex;
};