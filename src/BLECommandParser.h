#ifndef BLE_COMMAND_PARSER_H
#define BLE_COMMAND_PARSER_H

#include <string>
#include <vector>
#include <functional>
#include <cstring>
#include <algorithm>
#include <cstdio>

#define MAX_COMMANDS 40
#define MAX_COMMAND_STRLEN	32

enum class ParamType {
    NONE,
    INT,
    FLOAT
};

struct ParsedInstruction {
    char command[MAX_COMMAND_STRLEN];
    int preParamInt = 0;
    ParamType preParamType = ParamType::NONE;
    union {
        int i;
        float f;
    } postParam;
    ParamType postParamType = ParamType::NONE;
};

//using CommandHandler = std::function<void(const ParsedInstruction&)>; // C++11 Lambda
using CommandHandler = void (*)(const ParsedInstruction&); // C++ Style Function Pointer
// typedef void (*CommandHandler)(const ParsedInstruction&); // C Style Function Pointer

struct CommandEntry {
    std::string name;
    CommandHandler handler;
};

class BLECommandParser {
public:
    BLECommandParser(){commands.reserve(MAX_COMMANDS);}
    void registerCommand(const std::string& name, CommandHandler handler);
    void sortCommands();
    void dispatchInstruction(const std::string& input);
private:
    std::vector<CommandEntry> commands;
    bool parseInstruction(const std::string& input, ParsedInstruction& out);
};

#endif // BLE_COMMAND_PARSER_H
