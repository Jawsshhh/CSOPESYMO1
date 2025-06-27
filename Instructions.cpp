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


AddInstruction::AddInstruction(int pid, int var1, int var2, int var3) : Instruction(pid, Instruction::InstructionType::ADD)
{
}

void AddInstruction::execute()
{
	
	Instruction::execute();
	this->var1 = this->var2 + this->var3;
}

SubtractInstruction::SubtractInstruction(int pid, int var1, int var2, int var3) : Instruction(pid, Instruction::InstructionType::SUBTRACT)
{
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





