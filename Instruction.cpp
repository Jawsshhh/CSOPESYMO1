#include "Instruction.h"
#include "Process.h"
#include "ProcessHandler.h"
#include <iostream>

Instruction::Instruction(Process* process, InstructionType instructionType)
{
	this->process = process;
	this->instructionType = instructionType;
}

Instruction::InstructionType Instruction::getInstructionType()
{
	return InstructionType();
}

void Instruction::execute()
{

}



PrintInstruction::PrintInstruction(Process* process, const std::string& toPrint)
	: Instruction(process, Instruction::InstructionType::PRINT),
	toPrint(toPrint)
{
}

void PrintInstruction::execute() {
	Instruction::execute();
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
	Instruction::execute();
	performDeclaration();

	//DEBUG STATEMENT:

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
	Instruction::execute();
	add();

	//DEBUG STATEMENT
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
	Instruction::execute();
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

SleepInstruction::SleepInstruction(Process* process, uint8_t x) : Instruction(process, Instruction::InstructionType::SLEEP), 
	x(x) {	
}


void SleepInstruction::execute()
{
	Instruction::execute();
}

std::string SleepInstruction::getDetails() const {
	return "SLEEP for ";
}

/*
* FOR INSTRUCTION
*/
ForInstruction::ForInstruction(Process* process, std::vector<Instruction> instructionList, int repeats) : Instruction(process, Instruction::InstructionType::FOR)
{
}

void ForInstruction::execute()
{
	Instruction::execute();
}





