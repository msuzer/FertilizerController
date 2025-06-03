#include <Arduino.h>
#include "CommandHandler.h"
#include "TinyGPSPlus.h"
#include "BLETextServer.h"

const char* strGetDeviceUUID = "getDeviceUUID";

// === Command Keywords (for BLE/Serial interface) ===
static constexpr const char* CMD_SET_TARGET_RATE_DAA      = "setTargetFlowRatePerDaa";
static constexpr const char* CMD_SET_TARGET_RATE_MIN      = "setTargetFlowRatePerMin";
static constexpr const char* CMD_SET_FLOW_COEFF           = "setFlowCoeff";
static constexpr const char* CMD_SET_FLOW_MIN             = "setFlowMinValue";
static constexpr const char* CMD_SET_FLOW_MAX             = "setFlowMaxValue";
static constexpr const char* CMD_SET_BOOM_WIDTH           = "setBoomWidth";
static constexpr const char* CMD_SET_MIN_WORKING_SPEED    = "setMinWorkingSpeed";
static constexpr const char* CMD_SET_AUTO_REFRESH_PERIOD  = "setAutoRefreshPeriod";
static constexpr const char* CMD_SET_HEARTBEAT_PERIOD     = "setHeartBeatPeriod";
static constexpr const char* CMD_SET_SPEED_SOURCE         = "setSpeedSource";
static constexpr const char* CMD_SET_LEFT_OFFSET          = "setLeftActuatorOffset";
static constexpr const char* CMD_SET_RIGHT_OFFSET         = "setRightActuatorOffset";

extern BLETextServer bleServer;
extern TinyGPSPlus gps;
BLECommandParser parser;

// when adding new commands, consider increasing 'MAX_COMMANDS' in BLECommandParser
void registerUserCommandHandlers(void) {
    parser.registerCommand(strGetDeviceUUID, handlerGetDeviceUUID);
    parser.sortCommands();
}

void handlerGetDeviceUUID(const ParsedInstruction& instr) {
    uint64_t chipid = ESP.getEfuseMac();  // Returns 64-bit number (top 16 bits are often constant)
    printf("Chip ID: %04X%08X\n", (uint16_t)(chipid>>32), (uint32_t)chipid);
    bleServer.notifyFormatted("%s=%08X", strGetDeviceUUID, (uint32_t)(chipid & 0xFFFFFFFF));
}
