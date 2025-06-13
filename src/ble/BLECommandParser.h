// ============================================
// File: BLECommandParser.h
// Purpose: 
// Part of: BLE Layer / Communication
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once

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
    FLOAT,
    STRING
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
    char postParamStr[MAX_COMMAND_STRLEN] = {0};
};

//using CommandHandler = std::function<void(const ParsedInstruction&)>; // C++11 Lambda
using CommandFunction = void (*)(const ParsedInstruction&); // C++ Style Function Pointer
// typedef void (*CommandHandler)(const ParsedInstruction&); // C Style Function Pointer

struct CommandEntry {
    std::string name;
    CommandFunction handler;
};

class SystemContext; // Forward declaration

class BLECommandParser {
    friend class SystemContext; // Allow SystemContext to access private members
public:
    BLECommandParser(const BLECommandParser&) = delete;
    BLECommandParser& operator=(const BLECommandParser&) = delete;
    BLECommandParser(BLECommandParser&&) = delete;
    BLECommandParser& operator=(BLECommandParser&&) = delete;

    void registerCommand(const std::string& name, CommandFunction handler);
    void sortCommands();
    void dispatchInstruction(const std::string& input);
private:
    BLECommandParser(){commands.reserve(MAX_COMMANDS);}
    std::vector<CommandEntry> commands;
    bool parseInstruction(const std::string& input, ParsedInstruction& out);
};
