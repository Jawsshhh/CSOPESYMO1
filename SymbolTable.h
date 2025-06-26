#pragma once
#include <unordered_map>
#include <string>
#include <vector>

enum class DataType {
    INTEGER = 0,
    STRING = 1,
    FLOAT = 2,
    CHAR = 3,
};

class ST {
    enum DataType;
    std::string value;
};

class SymbolTable {
public:
    std::unordered_map<std::string, ST> symbolTable;

    bool checkVarExists(std::string varName);
    std::string retriveValue(std::string varName);
};