#include "Instruction.h"
#include "Process.h"
#include "ProcessHandler.h"

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

PrintInstruction::PrintInstruction(Process* process, std::string& toPrint) : Instruction(process, Instruction::InstructionType::PRINT)
{
	this->toPrint = toPrint;
}

void PrintInstruction::execute()
{
	Instruction::execute();

	std::stringstream msg;
	msg << this->toPrint << process->getName() << std::endl;
}

DeclareInstruction::DeclareInstruction(Process* process, const std::string& varName, uint16_t value) : Instruction(process, Instruction::InstructionType::DECLARE)
{
	this->varName = varName;
	this->value = value;
}

void DeclareInstruction::execute()
{
	Instruction::execute();
	performDeclaration();

	//DEBUG STATEMENT:

}

bool DeclareInstruction::performDeclaration()
{
	if (!process->getSymbolTable().checkVarExists(varName)) {
		//if the variable is not in the symbol table (meaning it exists)
		process->getSymbolTable().insertVariable(varName, SymbolTable::DataType::INTEGER, value);
		return true;
	}
	else {
		// if the variable does exist in the symbol table, return error
		return false;
	}

}

AddInstruction::AddInstruction(Process* process, const std::string& var1, const std::string& var2, const std::string& var3) : Instruction(process, Instruction::InstructionType::ADD)
{
	this->var1 = var1;
	this->var2 = var2;
	this->var3 = var3;
}

void AddInstruction::execute()
{
	Instruction::execute();
	AddInstruction::add();

	//DEBUG STATEMENT
}

uint16_t AddInstruction::getValue(const std::string& var)
{
	// if var is a number
	if (checkNumber(var)) {
		//convert variable to uint16_t
		return static_cast<uint16_t>(std::stoi(var));
	}
	else {
		//if the var is a variable
		//check symbol table if it exists and if it is an int
		if (process->getSymbolTable().checkVarExists(var) && process->getSymbolTable().retrieveDataType(var) == SymbolTable::DataType::INTEGER) {
			return static_cast<uint16_t>(std::stoi(process->getSymbolTable().retrieveValue(var)));
		}

	}

	DeclareInstruction::DeclareInstruction(process, var, 0);
	return static_cast<uint16_t>(std::stoi(process->getSymbolTable().retrieveValue(var)));
}

bool AddInstruction::checkNumber(const std::string& var)
{
	return !var.empty() && std::all_of(var.begin(), var.end(), std::isdigit);
}

void AddInstruction::add()
{
	uint16_t add2 = getValue(var2);
	uint16_t add3 = getValue(var3);

	uint16_t sum = add2 + add3;

	process->getSymbolTable().updateVariable(var1, std::to_string(sum));

}

SubtractInstruction::SubtractInstruction(Process* process, const std::string& var1, const std::string& var2, const std::string& var3) : Instruction(process, Instruction::InstructionType::SUBTRACT)
{
	this->var1 = var1;
	this->var2 = var2;
	this->var3 = var3;
}

void SubtractInstruction::execute()
{
	Instruction::execute();
	SubtractInstruction::subtract();
}

uint16_t SubtractInstruction::getValue(const std::string& var)
{
	// if var is a number
	if (checkNumber(var)) {
		//convert variable to uint16_t
		return static_cast<uint16_t>(std::stoi(var));
	}
	else {
		//if the var is a variable
		//check symbol table if it exists and if it is an int
		if (process->getSymbolTable().checkVarExists(var) && process->getSymbolTable().retrieveDataType(var) == SymbolTable::DataType::INTEGER) {
			return static_cast<uint16_t>(std::stoi(process->getSymbolTable().retrieveValue(var)));
		}

	}

	DeclareInstruction::DeclareInstruction(process, var, 0);
	return static_cast<uint16_t>(std::stoi(process->getSymbolTable().retrieveValue(var)));
}

bool SubtractInstruction::checkNumber(const std::string& var)
{
	return !var.empty() && std::all_of(var.begin(), var.end(), std::isdigit);
}

void SubtractInstruction::subtract()
{
	uint16_t add2 = getValue(var2);
	uint16_t add3 = getValue(var3);

	uint16_t difference = add2 - add3;

	process->getSymbolTable().updateVariable(var1, std::to_string(difference));
}

SleepInstruction::SleepInstruction(Process* process, uint8_t x) : Instruction(process, Instruction::InstructionType::SLEEP)
{

}


void SleepInstruction::execute()
{
	Instruction::execute();
}

ForInstruction::ForInstruction(Process* process, std::vector<Instruction> instructionList, int repeats) : Instruction(process, Instruction::InstructionType::FOR)
{
}

void ForInstruction::execute()
{
	Instruction::execute();
}





