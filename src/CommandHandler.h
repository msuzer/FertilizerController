#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "BLECommandParser.h"

void handlerGetDeviceUUID(const ParsedInstruction& instr);
void registerUserCommandHandlers(void);

#endif // COMMAND_HANDLER_H