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
    if (checkVarExists(varName)) {
        return symbolTable.at(varName).value;
    }
    return "";
}

SymbolTable::DataType SymbolTable::retrieveDataType(const std::string& varName)
{
    if (checkVarExists(varName)) {
        return symbolTable.at(varName).dataType;
    }
}

bool SymbolTable::insertVariable(const std::string& varName, DataType dataType, const std::string& value)
{
    //if the variable doesn't exist, insert new variable
    if (!checkVarExists(varName)) {
        ST entry;
        entry.dataType = dataType;
        entry.value = value;
        symbolTable.insert({ varName, entry });
        return true;
    }
    //if the variable does exist, do nothing
    return false;
    
}

bool SymbolTable::removeVariable(const std::string& varName)
{
    if (checkVarExists(varName)) {
        symbolTable.erase(varName);
        return true;
    }
    return false;
}

bool SymbolTable::updateVariable(const std::string& varName, const std::string& newValue)
{
    if (checkVarExists(varName)) {
        ST entry;
        entry.dataType = symbolTable[varName].dataType;
        entry.value = newValue;
        symbolTable[varName] = entry;
        return true;
    }
    return false;
}

const std::unordered_map<std::string, SymbolTable::ST>& SymbolTable::getSymbolTable() const {
    return symbolTable;
}