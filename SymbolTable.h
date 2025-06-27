#pragma once
#include <unordered_map>
#include <string>
#include <vector>

class SymbolTable {
public:
    enum class DataType {
        INTEGER = 0,
        STRING = 1,
        FLOAT = 2,
        CHAR = 3,
    };

    class ST {
    public:
        DataType dataType;
        std::string value;
    };

    bool checkVarExists(const std::string& varName);
    std::string retrieveValue(const std::string& varName);
    bool insertVariable(const std::string& varName, DataType dataType, const std::string& value);
    bool removeVariable(const std::string& varName);
private:
    std::unordered_map<std::string, ST> symbolTable;
};