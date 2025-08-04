#include "Instruction.h"
#include "Process.h"
#include "ProcessHandler.h"
#include "CPUTick.h"
#include <iostream>

Instruction::Instruction(Process* process, InstructionType instructionType)
{
	this->process = process;
	this->instructionType = instructionType;
}

Instruction::InstructionType Instruction::getInstructionType()
{
	return instructionType;
}

void Instruction::execute() {
}

/*
* PRINT INSTRUCTION:
*/

PrintInstruction::PrintInstruction(Process* process, const std::string& toPrint)
	: Instruction(process, Instruction::InstructionType::PRINT),
	toPrint(toPrint)
{
}

void PrintInstruction::execute() {
	std::stringstream msg;

	if (process->getSymbolTable().checkVarExists(toPrint)) {
		msg << "Value from " << toPrint << ": "
			<< process->getSymbolTable().retrieveValue(toPrint) << std::endl;
	}
	else {
		msg << toPrint << std::endl;
	}

}

std::string PrintInstruction::getDetails() const {
	return "Message: " + toPrint;
}

/*
* DECLARE INSTRUCTION: declares a uint16 with a variable name "var", and a default "value"
*/

DeclareInstruction::DeclareInstruction(Process* process, const std::string& varName, uint16_t value)
	: Instruction(process, Instruction::InstructionType::DECLARE),
	varName(varName),
	value(value)  // Direct initialization of uint16_t
{
}

void DeclareInstruction::execute()
{
	performDeclaration();
}

bool DeclareInstruction::performDeclaration() {
	if (!process->getSymbolTable().checkVarExists(varName)) {
		process->getSymbolTable().insertVariable(
			varName,
			SymbolTable::DataType::INTEGER,
			std::to_string(value)  // Convert uint16_t to string for storage
		);
		return true;
	}
	return false;
}

std::string DeclareInstruction::getDetails() const {
	return "Declared variable: " + varName + " with value: " + process->getSymbolTable().retrieveValue(varName);
}

/*
* ADD INSTRUCTION: performs an addition operation var 1 = var2/value + var3/value
* var1, var2, var3 are variables. Variables are automatically declared with a value of 0 if they have not been declared beforehand. Can also add a uint16 value.
*/

AddInstruction::AddInstruction(Process* process, const std::string& var1,
	const std::string& var2, const std::string& var3)
	: Instruction(process, InstructionType::ADD),
	var1(var1), var2(var2), var3(var3) {
}

void AddInstruction::execute()
{
	add();
}

uint16_t AddInstruction::getValue(const std::string& var)
{
	//check if the var is a number
	if (checkNumber(var)) {
		return static_cast<uint16_t>(std::stoi(var));
	}

	//if its not a number, and it doesn't exist, declare it as 
	if (!process->getSymbolTable().checkVarExists(var)) {
		DeclareInstruction decl(process, var, 0);
		decl.execute();
	}

	return static_cast<uint16_t>(std::stoi(process->getSymbolTable().retrieveValue(var)));
}

std::string AddInstruction::getDetails() const {
	return "ADD " + process->getSymbolTable().retrieveValue(var1) + " = " + var2 + " + " + var3;
}

bool AddInstruction::checkNumber(const std::string& var)
{
	return !var.empty() && std::all_of(var.begin(), var.end(), std::isdigit);
}

void AddInstruction::add()
{
	if (!process->getSymbolTable().checkVarExists(var1)) {
		DeclareInstruction decl(process, var1, 0);
		decl.execute();
	}

	uint16_t val2 = getValue(var2);
	uint16_t val3 = getValue(var3);

	uint16_t result = val2 + val3;

	process->getSymbolTable().updateVariable(var1, std::to_string(result));

}

/*
* SUBTRACT INSTRUCTION:
*/

SubtractInstruction::SubtractInstruction(Process* process, const std::string& var1,
	const std::string& var2, const std::string& var3)
	: Instruction(process, InstructionType::SUBTRACT),
	var1(var1), var2(var2), var3(var3) {
}

void SubtractInstruction::execute()
{
	subtract();
}

uint16_t SubtractInstruction::getValue(const std::string& var)
{
	//check if the var is a number
	if (checkNumber(var)) {
		return static_cast<uint16_t>(std::stoi(var));
	}

	//if its not a number, and it doesn't exist, declare it as 
	if (!process->getSymbolTable().checkVarExists(var)) {
		DeclareInstruction decl(process, var, 0);
		decl.execute();
	}

	return static_cast<uint16_t>(std::stoi(process->getSymbolTable().retrieveValue(var)));
}

bool SubtractInstruction::checkNumber(const std::string& var)
{
	return !var.empty() && std::all_of(var.begin(), var.end(), std::isdigit);
}

void SubtractInstruction::subtract()
{
	if (!process->getSymbolTable().checkVarExists(var1)) {
		DeclareInstruction decl(process, var1, 0);
		decl.execute();
	}

	uint16_t val2 = getValue(var2);
	uint16_t val3 = getValue(var3);

	uint16_t result = val2 - val3;

	process->getSymbolTable().updateVariable(var1, std::to_string(result));
}

std::string SubtractInstruction::getDetails() const {
	return "SUB " + process->getSymbolTable().retrieveValue(var1) + " = " + var2 + " - " + var3;
}

/*
* SLEEP INSTRUCTION
*/

SleepInstruction::SleepInstruction(Process* process, std::string sleepCycles)
	: Instruction(process, Instruction::InstructionType::SLEEP), sleepCycles(sleepCycles) 
{
}

void SleepInstruction::execute()
{
	sleep();
}

void SleepInstruction::sleep()
{
	uint8_t sC = static_cast<uint8_t>(std::stoi(sleepCycles));
	process->setIsSleeping(true, sC);
}

std::string SleepInstruction::getDetails() const {
	return "[INITIALIZE] SLEEP for " + std::to_string(process->getRemainingSleepCycles()) + " cycles";
}

/*
* FOR INSTRUCTION
*/

ForInstruction::ForInstruction(Process* process, std::vector<Instruction> instructionList, int repeats) 
	: Instruction(process, Instruction::InstructionType::FOR), repeats(repeats)
{
}

void ForInstruction::execute()
{
	if (repeats <= 3) {

	}
}

/*
* READ INSTRUCTION
*/
ReadInstruction::ReadInstruction(Process* process, const std::string& varName, const std::string& memoryAddress)
	: Instruction(process, Instruction::InstructionType::READ), varName(varName), memoryAddress(memoryAddress)
{
}

void ReadInstruction::execute() {
	read();
}

bool ReadInstruction::read() {
	try {
		
		size_t hexAddress = std::stoull(memoryAddress, nullptr, 16);

		if (!isValidMemoryAddress(hexAddress)) {
			
			process->setMemoryAccessViolation(true, memoryAddress);
			return false;
		}

		uint16_t value = readFromMemoryAddress(hexAddress);

		if (!process->getSymbolTable().checkVarExists(varName)) {
			DeclareInstruction decl(process, varName, 0);
			decl.execute();
		}

		process->getSymbolTable().updateVariable(varName, std::to_string(value));

		return true;
	}
	catch (const std::exception& e) {
		process->setMemoryAccessViolation(true, memoryAddress);
		return false;
	}
}

uint16_t ReadInstruction::readFromMemoryAddress(size_t address) {
	auto& memoryMap = process->getMemoryMap();

	if (memoryMap.find(address) != memoryMap.end()) {
		return memoryMap[address];
	}

	return 0; 
}

bool ReadInstruction::isValidMemoryAddress(size_t address) {
	size_t processMemorySize = process->getMemoryNeeded();
	size_t baseAddress = process->getBaseMemoryAddress();

	return (address >= baseAddress && address < (baseAddress + processMemorySize));
}

std::string ReadInstruction::getDetails() const {
	return "READ " + varName + " from address " + memoryAddress;
}
/*
* WRITE INSTRUCTION
*/
WriteInstruction::WriteInstruction(Process* process, const std::string& memoryAddress, const std::string& value)
	: Instruction(process, Instruction::InstructionType::WRITE),
	memoryAddress(memoryAddress), value(value)
{
}

void WriteInstruction::execute() {
	write();
}

bool WriteInstruction::write() {
	try {
		size_t hexAddress = std::stoull(memoryAddress, nullptr, 16);

		if (!isValidMemoryAddress(hexAddress)) {
			process->setMemoryAccessViolation(true, memoryAddress);
			return false;
		}

		uint16_t writeValue = getValue(value);

		writeToMemoryAddress(hexAddress, writeValue);

		return true;
	}
	catch (const std::exception& e) {
		process->setMemoryAccessViolation(true, memoryAddress);
		return false;
	}
}

uint16_t WriteInstruction::getValue(const std::string& val) {
	if (checkNumber(val)) {
		return static_cast<uint16_t>(std::stoi(val));
	}

	if (process->getSymbolTable().checkVarExists(val)) {
		return static_cast<uint16_t>(std::stoi(process->getSymbolTable().retrieveValue(val)));
	}

	DeclareInstruction decl(process, val, 0);
	decl.execute();
	return 0;
}

void WriteInstruction::writeToMemoryAddress(size_t address, uint16_t value) {
	auto& memoryMap = process->getMemoryMap();
	memoryMap[address] = value;
}

bool WriteInstruction::isValidMemoryAddress(size_t address) {
	size_t processMemorySize = process->getMemoryNeeded();
	size_t baseAddress = process->getBaseMemoryAddress();

	return (address >= baseAddress && address < (baseAddress + processMemorySize));
}

bool WriteInstruction::checkNumber(const std::string& val) {
	return !val.empty() && std::all_of(val.begin(), val.end(), std::isdigit);
}

std::string WriteInstruction::getDetails() const {
	return "WRITE " + value + " to address " + memoryAddress;
}





