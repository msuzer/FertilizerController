// ============================================
// File: CommandHandler.h
// Purpose: Handles BLE command parsing and execution
// Part of: BLE Layer / Communication
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once

#include "BLECommandParser.h"
#include <Arduino.h>

class SystemContext; // Forward declaration

class CommandHandler {
public:
    static CommandHandler& getInstance();
    CommandHandler() = default;
    CommandHandler(const CommandHandler&) = delete;
    CommandHandler& operator=(const CommandHandler&) = delete;

    inline void setContext(SystemContext* ctx) { context = ctx; }

    void registerHandlers();
    static void sendBLEPacketChecked(const String& packet);

    // Handlers
    static void handlerSetBLEDeviceName(const ParsedInstruction& instr);
    static void handlerGetDeviceInfo(const ParsedInstruction& instr);
    static void handlerGetSpeedInfo(const ParsedInstruction& instr);
    static void handlerGetTaskInfo(const ParsedInstruction& instr);
    static void handlerGetVersionInfo(const ParsedInstruction& instr);

    static void handlerStartNewTask(const ParsedInstruction& instr);
    static void handlerPauseTask(const ParsedInstruction& instr);
    static void handlerResumeTask(const ParsedInstruction& instr);
    static void handlerEndTask(const ParsedInstruction& instr);
    static void handlerSetInWorkZone(const ParsedInstruction& instr);

    static void handlerSetTargetFlowRatePerDaa(const ParsedInstruction& instr);
    static void handlerSetTargetFlowRatePerMin(const ParsedInstruction& instr);
    static void handlerSetTankLevel(const ParsedInstruction& instr);
    static void handlerSetMeasuredWeight(const ParsedInstruction& instr);

    static void handlerSetSpeedSource(const ParsedInstruction& instr);
    static void handlerSetMinWorkingSpeed(const ParsedInstruction& instr);
    static void handlerSetSimSpeed(const ParsedInstruction& instr);

    static void handlerSetAutoRefreshPeriod(const ParsedInstruction& instr);
    static void handlerSetHeartBeatPeriod(const ParsedInstruction& instr);
    static void handlerGetErrorInfo(const ParsedInstruction& instr);
    static void handlerSetPIDKp(const ParsedInstruction& instr);
    static void handlerSetPIDKi(const ParsedInstruction& instr);

    static void handlerReportPIParams(const ParsedInstruction& instr);
    static void handlerReportUserParams(const ParsedInstruction& instr);

private:
    static SystemContext* context;
};
