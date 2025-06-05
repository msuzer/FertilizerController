#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "BLECommandParser.h"
#include "AppServices.h"

void CommandHandler_setServices(AppServices* s);

void handlerSetBLEDeviceName(const ParsedInstruction& instr);
void handlerGetDeviceInfo(const ParsedInstruction& instr);
void handlerGetSpeedInfo(const ParsedInstruction& instr);
void handlerGetTaskInfo(const ParsedInstruction& instr);

void handlerStartNewTask(const ParsedInstruction& instr);
void handlerPauseTask(const ParsedInstruction& instr);
void handlerResumeTask(const ParsedInstruction& instr);
void handlerEndTask(const ParsedInstruction& instr);
void handlerSetInWorkZone(const ParsedInstruction& instr);

void handlerSetTargetFlowRatePerDaa(const ParsedInstruction& instr);
void handlerSetTargetFlowRatePerMin(const ParsedInstruction& instr);
void handlerSetTankLevel(const ParsedInstruction& instr);
void handlerSetMeasuredWeight(const ParsedInstruction& instr);

void handlerSetSpeedSource(const ParsedInstruction& instr);
void handlerSetMinWorkingSpeed(const ParsedInstruction& instr);
void handlerSetSimSpeed(const ParsedInstruction& instr);

void handlerSetAutoRefreshPeriod(const ParsedInstruction& instr);
void handlerSetHeartBeatPeriod(const ParsedInstruction& instr);
void handlerGetErrorInfo(const ParsedInstruction& instr);
void handlerSetPIDKp(const ParsedInstruction& instr);
void handlerSetPIDKi(const ParsedInstruction& instr);

void handlerReportPIParams(const ParsedInstruction& instr);
void handlerReportUserParams(const ParsedInstruction& instr);

void registerUserCommandHandlers(void);

#endif // COMMAND_HANDLER_H