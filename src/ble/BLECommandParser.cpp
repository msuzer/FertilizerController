#include "BLECommandParser.h"

void BLECommandParser::registerCommand(const std::string& name, CommandFunction handler) {
    commands.push_back({name, handler});
}

void BLECommandParser::sortCommands() {
    std::sort(commands.begin(), commands.end(), [](const CommandEntry& a, const CommandEntry& b) {
        return a.name < b.name;
    });
}

void BLECommandParser::dispatchInstruction(const std::string& input) {
    ParsedInstruction instr;
    
    printf("[Dispatch] Received: %s\n", input.c_str());

    if (!parseInstruction(input, instr)) {
        printf("[Dispatch] Invalid instruction: %s\n", input.c_str());
        return;
    }

    auto it = std::lower_bound(commands.begin(), commands.end(), instr.command,
        [](const CommandEntry& entry, const std::string& name) {
            return entry.name < name;
        });

    if (it != commands.end() && it->name == instr.command) {
        // printf("[Dispatch] Dispatching command: %s\n", it->name.c_str());
        it->handler(instr);
    } else {
        printf("[Dispatch] No handler for command: %s\n", instr.command);
    }
}

bool BLECommandParser::parseInstruction(const std::string& input, ParsedInstruction& out) {
    char cmd[32], val[32];
    int i1, i2;
    float f2;

    if (input.empty()) return false;

    if (input.find('=') != std::string::npos) { // check if the instruction has a '='
        if (input.find('.') != std::string::npos || input.find(',') != std::string::npos) {  // check if the instruction has a '.' or ','
            if (sscanf(input.c_str(), "%31[^0-9]%d=%f", cmd, &i1, &f2) == 3) {
                strcpy(out.command, cmd);
                out.preParamInt = i1;
                out.preParamType = ParamType::INT;
                out.postParam.f = f2;
                out.postParamType = ParamType::FLOAT;
                printf("Parsed: %s%d=%.2f\n", out.command, out.preParamInt, out.postParam.f);
                return true;
            }

            if (sscanf(input.c_str(), "%31[^=]=%f", cmd, &f2) == 2) {
                strcpy(out.command, cmd);
                out.preParamType = ParamType::NONE;
                out.postParam.f = f2;
                out.postParamType = ParamType::FLOAT;
                printf("Parsed: %s=%.2f\n", out.command, out.postParam.f);
                return true;
            }    
        } else {
            if (sscanf(input.c_str(), "%31[^0-9]%d=%d", cmd, &i1, &i2) == 3) {
                strcpy(out.command, cmd);
                out.preParamInt = i1;
                out.preParamType = ParamType::INT;
                out.postParam.i = i2;
                out.postParamType = ParamType::INT;
                printf("Parsed: %s%d=%d\n", out.command, out.preParamInt, out.postParam.i);
                return true;
            }

            if (sscanf(input.c_str(), "%31[^=]=%d", cmd, &i2) == 2) {
                strcpy(out.command, cmd);
                out.preParamType = ParamType::NONE;
                out.postParam.i = i2;
                out.postParamType = ParamType::INT;
                printf("Parsed: %s=%d\n", out.command, out.postParam.i);
                return true;
            }

            if (sscanf(input.c_str(), "%31[^=]=%31[^\n]", cmd, val) == 2) {
                strcpy(out.command, cmd);
                strncpy(out.postParamStr, val, sizeof(out.postParamStr));
                out.postParamStr[sizeof(out.postParamStr) - 1] = '\0';
                out.postParamType = ParamType::STRING;
                printf("Parsed: %s=%s (as string)\n", out.command, out.postParamStr);
                return true;
            }
        }
    }

    if (input.length() < sizeof(out.command)) {
        strcpy(out.command, input.c_str());
        out.preParamType = ParamType::NONE;
        out.postParamType = ParamType::NONE;
        printf("Parsed: %s\n", out.command);
        return true;
    }

    return false;
}
