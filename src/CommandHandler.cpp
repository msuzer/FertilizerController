#include <Arduino.h>
#include "CommandHandler.h"
#include "TinyGPSPlus.h"

const char* strGetDeviceUUID = "getDeviceUUID";

BLECommandParser parser;
extern TinyGPSPlus gps;

// when adding new commands, consider increasing 'MAX_COMMANDS' in BLECommandParser
void registerUserCommandHandlers(void) {
    parser.registerCommand(strGetDeviceUUID, handlerGetDeviceUUID);
  
    parser.sortCommands();
}

void handlerGetDeviceUUID(const ParsedInstruction& instr) {
    char buffer[64];

    uint64_t chipid = ESP.getEfuseMac();  // Returns 64-bit number (top 16 bits are often constant)
    printf("Chip ID: %04X%08X\n", (uint16_t)(chipid>>32), (uint32_t)chipid);
    sprintf(buffer, "%s=%08X", strGetDeviceUUID, (uint32_t)(chipid & 0xFFFFFFFF));

    // indicateString(buffer);
}
