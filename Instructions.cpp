#include "Instructions.h"
#include "Process.h"

Instruction::Instruction(int pid, InstructionType instructionType)
{

}

Instruction::InstructionType Instruction::getInstructionType()
{
	return InstructionType();
}

void Instruction::execute()
{
}

PrintInstruction::PrintInstruction(int pid, std::string& toPrint) : Instruction(pid, Instruction::InstructionType::PRINT)
{
	this->toPrint = toPrint;
}

void PrintInstruction::execute()
{
	Instruction::execute();

	std::stringstream msg;
	//need to include procesname somehow
	msg << this->toPrint << std::endl;
}

DeclareInstruction::DeclareInstruction(int pid, std::string& varName, uint16_t value) : Instruction(pid, Instruction::InstructionType::DECLARE)
{
	this->varName = varName;
	this->value = value;
}

void DeclareInstruction::execute()
{
	Instruction::execute();
	performDeclaration();

}

void DeclareInstruction::performDeclaration()
{
	//check if variable is in the symbol table
	//if it is in the symbol table, throw error
	//if not, push into symbol table with INT data type

}


AddInstruction::AddInstruction(int pid, const std::string& var1, const std::string& var2, const std::string& var3) : Instruction(pid, Instruction::InstructionType::ADD)
{
	this->var1 = var1;
	this->var2 = var2;
	this->var3 = var3;
}

void AddInstruction::execute()
{
	Instruction::execute();
}

uint16_t AddInstruction::getValue(const std::string& var)
{
	// if the variable is a number
	if (checkNumber(var)) {
		//convert variable to uint16_t
		return static_cast<uint16_t>(std::stoi(var));
	}
	else {
		//check symbol table if it exists
		//check
	}
	// 1. check if what has been put is a variable (string) or a value (number), if it is a var proceed to 2, else
	// 2. check symbol table for the var if it exists
	// 2a. if it doesn't declare it as 0 -> use DECLARE function
	// 3. if it does exist, check the data type and confirm that it is an integer
	// 4. add

	// 3a. if it is a value, check first if it is an integer and get the uint16_t value of that integer

	return 0;
}

bool AddInstruction::checkNumber(const std::string& var)
{
	return !var.empty() && std::all_of(var.begin(), var.end(), std::isdigit);
}

void AddInstruction::add(const std::string& var1, const std::string& var2, const std::string& var3)
{
	uint16_t var2 = getValue(var2);
	uint16_t var3 = getValue(var3);

	//reassign it to the variable

}

SubtractInstruction::SubtractInstruction(int pid, const std::string& var1, const std::string& var2, const std::string& var3) : Instruction(pid, Instruction::InstructionType::SUBTRACT)
{
	this->var1 = var1;
	this->var2 = var2;
	this->var3 = var3;
}

void SubtractInstruction::execute()
{
	Instruction::execute();
}

SleepInstruction::SleepInstruction(int pid, uint8_t x) : Instruction(pid, Instruction::InstructionType::SLEEP)
{
}

void SleepInstruction::execute()
{
	Instruction::execute();
}

ForInstruction::ForInstruction(int pid, std::vector<Instruction> instructionList, int repeats) : Instruction(pid, Instruction::InstructionType::FOR)
{
}

void ForInstruction::execute()
{
	Instruction::execute();
}





