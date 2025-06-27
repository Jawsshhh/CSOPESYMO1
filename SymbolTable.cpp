#include "SymbolTable.h"

bool SymbolTable::checkVarExists(const std::string& varName)
{
    // if its not present, return 0
    if (symbolTable.find(varName) == symbolTable.end()) {
        return false;
    }
    return true;
}

std::string SymbolTable::retrieveValue(const std::string& varName)
{
    if (checkVarExists(varName) == true) {
        return symbolTable.at(varName).value;
    }
    return "";
}

bool SymbolTable::insertVariable(const std::string& varName, DataType dataType, const std::string& value)
{
    //if the variable doesn't exist, insert new variable
    if (checkVarExists(varName) == true) {
        ST entry;
        entry.dataType = dataType;
        entry.value = value;
        symbolTable[varName] = entry;
        return true;
    }
    //if the variable does exist, do nothing
    return false;
    
}

bool SymbolTable::removeVariable(const std::string& varName)
{
    if (checkVarExists(varName) == true) {
        symbolTable.erase(varName);
        return true;
    }
    return false;
}
