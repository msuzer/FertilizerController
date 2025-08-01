// ============================================
// File: CommandHandler.cpp
// Purpose: Handles BLE command parsing and execution
// Part of: BLE Layer / Communication
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include <Arduino.h>
#include "CommandHandler.h"
#include "BLECommandParser.h"
#include "BLETextServer.h"
#include "core/SystemContext.h"
#include "gps/GPSProvider.h"
#include "control/PIController.h"
#include "core/SystemPreferences.h"
#include "ble/UserInfoFormatter.h"
#include "core/LogUtils.h"

#define MAX_BLE_PACKET_SIZE 244  // Example BLE max size in bytes (adjust as needed)

static constexpr const char* CMD_SET_LOG_LEVEL              = "setLogLevel";

static constexpr const char* CMD_SET_BLE_DEVICE_NAME        = "setBLEDevName";
static constexpr const char* CMD_GET_DEVICE_INFO            = "getDeviceInfo";
static constexpr const char* CMD_GET_SPEED_INFO             = "getSpeedInfo";
static constexpr const char* CMD_GET_TASK_INFO              = "getTaskInfo";
static constexpr const char* CMD_GET_VERSION_INFO           = "getVersionInfo";

static constexpr const char* CMD_SET_TASK_STATE             = "setTaskState";
static constexpr const char* CMD_SET_IN_WORK_ZONE           = "setInWorkZone";

static constexpr const char* CMD_SET_TARGET_FLOW_RATE_DAA   = "setTargetFlowRatePerDaa";
static constexpr const char* CMD_SET_TARGET_FLOW_RATE_MIN   = "setTargetFlowRatePerMin";
static constexpr const char* CMD_SET_TANK_LEVEL             = "setTankLevel";
static constexpr const char* CMD_SET_MEASURED_WEIGHT        = "setMeasuredWeight";

static constexpr const char* CMD_SET_SPEED_SOURCE           = "setSpeedSource";
static constexpr const char* CMD_SET_MIN_WORKING_SPEED      = "setMinWorkingSpeed";
static constexpr const char* CMD_SET_SIM_SPEED              = "setSimSpeed";

static constexpr const char* CMD_SET_AUTO_REFRESH_PERIOD    = "setAutoRefresh";
static constexpr const char* CMD_SET_HEARTBEAT_PERIOD       = "setHeartBeat";
static constexpr const char* CMD_GET_ERROR_INFO             = "reportError";
static constexpr const char* CMD_SET_PI_KP                  = "setPIDKp";
static constexpr const char* CMD_SET_PI_KI                  = "setPIDKi";

static constexpr const char* CMD_REPORT_PID_PARAMS          = "reportPIDParams";
static constexpr const char* CMD_REPORT_USER_PARAMS         = "reportUserParams";

SystemContext* CommandHandler::context = nullptr;

// when adding new commands, consider increasing 'MAX_COMMANDS' in BLECommandParser
void CommandHandler::registerHandlers(void) {
    BLECommandParser& parser = context->getBLECommandParser();
    parser.registerCommand(CMD_SET_LOG_LEVEL, handlerSetLogLevel);

    parser.registerCommand(CMD_SET_BLE_DEVICE_NAME, handlerSetBLEDeviceName);
    parser.registerCommand(CMD_GET_DEVICE_INFO, handlerGetDeviceInfo);
    parser.registerCommand(CMD_GET_SPEED_INFO, handlerGetSpeedInfo);
    parser.registerCommand(CMD_GET_TASK_INFO, handlerGetTaskInfo);
    parser.registerCommand(CMD_GET_VERSION_INFO, handlerGetVersionInfo);

    parser.registerCommand(CMD_SET_TASK_STATE, handlerSetTaskState);
    parser.registerCommand(CMD_SET_IN_WORK_ZONE, handlerSetInWorkZone);
    parser.registerCommand(CMD_SET_TARGET_FLOW_RATE_DAA, handlerSetTargetFlowRatePerDaa);
    parser.registerCommand(CMD_SET_TARGET_FLOW_RATE_MIN, handlerSetTargetFlowRatePerMin);
    parser.registerCommand(CMD_SET_MEASURED_WEIGHT, handlerSetMeasuredWeight);
    parser.registerCommand(CMD_SET_SPEED_SOURCE, handlerSetSpeedSource);
    parser.registerCommand(CMD_SET_MIN_WORKING_SPEED, handlerSetMinWorkingSpeed);
    parser.registerCommand(CMD_SET_SIM_SPEED, handlerSetSimSpeed);
    parser.registerCommand(CMD_SET_TANK_LEVEL, handlerSetTankLevel);
    parser.registerCommand(CMD_SET_AUTO_REFRESH_PERIOD, handlerSetAutoRefreshPeriod);
    parser.registerCommand(CMD_SET_HEARTBEAT_PERIOD, handlerSetHeartBeatPeriod);
    parser.registerCommand(CMD_GET_ERROR_INFO, handlerGetErrorInfo);
    parser.registerCommand(CMD_SET_PI_KP, handlerSetPIDKp);
    parser.registerCommand(CMD_SET_PI_KI, handlerSetPIDKi);
    parser.registerCommand(CMD_REPORT_PID_PARAMS, handlerReportPIParams);
    parser.registerCommand(CMD_REPORT_USER_PARAMS, handlerReportUserParams);

    parser.sortCommands();
}

void CommandHandler::sendBLEPacketChecked(const String& packet) {
    if (packet.length() > MAX_BLE_PACKET_SIZE) {
        LogUtils::warn("[BLE] packet too long! Length=%d, Max=%d. Not sending.\n",
                      packet.length(), MAX_BLE_PACKET_SIZE);
        return;
    }

    Serial.println(packet);
    context->getBLETextServer().notify(packet.c_str());
}

void CommandHandler::handlerSetLogLevel(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        int level = instr.postParam.i;
        LogUtils::setLogLevel(static_cast<LogLevel>(level));
        SystemPreferences::save(PrefKey::KEY_LOG_LEVEL, level);
        String response = String(level) + " (" + LogUtils::logLevelToString(static_cast<LogLevel>(level)) + ")";
        context->getBLETextServer().notifyString(CMD_SET_LOG_LEVEL, response.c_str());
    }
}

void CommandHandler::handlerSetBLEDeviceName(const ParsedInstruction &instr) {
    if (instr.postParamType == ParamType::STRING) {
        context->getBLETextServer().setDeviceName(instr.postParamStr);
    }
}

void CommandHandler::handlerGetVersionInfo(const ParsedInstruction &instr) {
    String packet = UserInfoFormatter::makeVersionInfoPacket();
    sendBLEPacketChecked(packet);
}

void CommandHandler::handlerGetDeviceInfo(const ParsedInstruction& instr) {
    UserInfoFormatter::DeviceInfoData devData = {
        context->getBLETextServer().getDeviceName(),
        context->getEspID().c_str(),
        context->getBoardID().c_str(),
        context->getBleMAC().c_str()
    };

    String packet = UserInfoFormatter::makeDeviceInfoPacket(devData);
    sendBLEPacketChecked(packet);
}

void CommandHandler::handlerGetSpeedInfo(const ParsedInstruction& instr) {
    const GPSProvider& gpsProvider = context->getGPSProvider();
    Location_t loc = gpsProvider.getLocation();
    const SystemParams& params = context->getParams();

    UserInfoFormatter::GPSInfoData gpsData = {
        params.speedSource.c_str(),
        params.minWorkingSpeed,
        params.simSpeed,
        gpsProvider.getSpeed(),
        (float) loc.lat,
        (float) loc.lng,
        gpsProvider.getSatelliteCount()
    };

    String packet = UserInfoFormatter::makeGPSInfoPacket(gpsData);
    sendBLEPacketChecked(packet);
}

void CommandHandler::handlerGetTaskInfo(const ParsedInstruction& instr) {
    DispenserChannel& left = context->getLeftChannel();
    DispenserChannel& right = context->getRightChannel();
    ApplicationMetrics& leftMetrics = left.getTaskController().getMetrics();
    ApplicationMetrics& rightMetrics = right.getTaskController().getMetrics();
        
    UserInfoFormatter::TaskChannelInfoData leftData = {
        left.getTargetFlowRatePerDaa(), left.getTargetFlowRatePerMin(),
        left.getRealFlowRatePerDaa(), left.getRealFlowRatePerMin(),
        (int) ApplicationMetrics::getTankLevel(),
        leftMetrics.getArea(), leftMetrics.getDuration(), leftMetrics.getConsumption()
    };

    UserInfoFormatter::TaskChannelInfoData rightData = {
        right.getTargetFlowRatePerDaa(), right.getTargetFlowRatePerMin(),
        right.getRealFlowRatePerDaa(), right.getRealFlowRatePerMin(),
        (int) ApplicationMetrics::getTankLevel(),
        rightMetrics.getArea(), rightMetrics.getDuration(), rightMetrics.getConsumption()
    };

    String packet = UserInfoFormatter::makeTaskInfoPacket(leftData, rightData);
    sendBLEPacketChecked(packet);
}

void CommandHandler::handlerReportPIParams(const ParsedInstruction& instr) {
    UserInfoFormatter::PIInfoData piData = {
        context->getLeftChannel().getPIController().getPIKp(),
        context->getLeftChannel().getPIController().getPIKi()
    };

    String packet = UserInfoFormatter::makePIPacket(piData);
    sendBLEPacketChecked(packet);
}

void CommandHandler::handlerSetTaskState(const ParsedInstruction& instr) {
    if (instr.preParamType == ParamType::INT) {
        const int channelIndex = instr.preParamInt;
        if (instr.postParamType == ParamType::INT) {
            const UserTaskState newState = static_cast<UserTaskState>(instr.postParam.i);
            if (channelIndex == 0) {
                context->getLeftChannel().getTaskController().setTaskState(newState);
            } else if (channelIndex == 1) {
                context->getRightChannel().getTaskController().setTaskState(newState);
            }
        } else {
            // TODO report current task state
        }
    }
}

void CommandHandler::handlerSetInWorkZone(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        DispenserChannel::setClientInWorkZone(instr.postParam.i > 0);
    }

    if (DispenserChannel::isClientInWorkZone()) {
        handlerGetTaskInfo(instr);
    }
}

void CommandHandler::handlerSetTargetFlowRatePerDaa(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        context->getLeftChannel().setTargetFlowRatePerDaa(instr.postParam.f);
        context->getLeftChannel().setTargetFlowRatePerMin(0.0f);
        SystemPreferences::save(PrefKey::KEY_LEFT_RATE_DAA, context->getLeftChannel().getTargetFlowRatePerMin());
        SystemPreferences::save(PrefKey::KEY_LEFT_RATE_MIN, context->getLeftChannel().getTargetFlowRatePerDaa());
    }

    context->getBLETextServer().notifyValue(CMD_SET_TARGET_FLOW_RATE_DAA, context->getLeftChannel().getTargetFlowRatePerDaa());
}

void CommandHandler::handlerSetTargetFlowRatePerMin(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        context->getLeftChannel().setTargetFlowRatePerMin(instr.postParam.f);
        context->getLeftChannel().setTargetFlowRatePerDaa(0.0f);
        SystemPreferences::save(PrefKey::KEY_LEFT_RATE_DAA, context->getLeftChannel().getTargetFlowRatePerMin());
        SystemPreferences::save(PrefKey::KEY_LEFT_RATE_MIN, context->getLeftChannel().getTargetFlowRatePerDaa());
    }

    context->getBLETextServer().notifyValue(CMD_SET_TARGET_FLOW_RATE_MIN, context->getLeftChannel().getTargetFlowRatePerMin());
}

void CommandHandler::handlerSetMeasuredWeight(const ParsedInstruction& instr) {
    // TODO: implement handlerSetMeasuredWeight
}

void CommandHandler::handlerSetSpeedSource(const ParsedInstruction& instr) {
    SystemParams& params = context->getParams();
    if (instr.postParamType == ParamType::STRING) {
        params.speedSource = instr.postParamStr;
        SystemPreferences::save(PrefKey::KEY_SPEED_SRC, params.speedSource);
    }
    context->getBLETextServer().notifyString(CMD_SET_SPEED_SOURCE, params.speedSource);
}

void CommandHandler::handlerSetMinWorkingSpeed(const ParsedInstruction& instr) {
    SystemParams& params = context->getParams();
    if (instr.postParamType == ParamType::FLOAT) {
        params.minWorkingSpeed = instr.postParam.f;
        SystemPreferences::save(PrefKey::KEY_MIN_SPEED, params.minWorkingSpeed);
    }
    context->getBLETextServer().notifyValue(CMD_SET_MIN_WORKING_SPEED, params.minWorkingSpeed);
}

void CommandHandler::handlerSetSimSpeed(const ParsedInstruction& instr) {
    SystemParams& params = context->getParams();
    if (instr.postParamType == ParamType::FLOAT) {
        params.simSpeed = instr.postParam.f;
        SystemPreferences::save(PrefKey::KEY_SIM_SPEED, params.simSpeed);
    }
    context->getBLETextServer().notifyValue(CMD_SET_SIM_SPEED, params.simSpeed);
}

void CommandHandler::handlerSetTankLevel(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        ApplicationMetrics::setTankLevel(instr.postParam.i);
        SystemPreferences::save(PrefKey::KEY_TANK_LEVEL, ApplicationMetrics::getTankLevel());
    }
    context->getBLETextServer().notifyValue(CMD_SET_TANK_LEVEL, ApplicationMetrics::getTankLevel());
}

void CommandHandler::handlerSetAutoRefreshPeriod(const ParsedInstruction& instr) {
    SystemParams& params = context->getParams();
    if (instr.postParamType == ParamType::INT) {
        params.autoRefreshPeriod = instr.postParam.i;
        SystemPreferences::save(PrefKey::KEY_REFRESH, params.autoRefreshPeriod);
    }
    context->getBLETextServer().notifyValue(CMD_SET_AUTO_REFRESH_PERIOD, params.autoRefreshPeriod);
}

void CommandHandler::handlerSetHeartBeatPeriod(const ParsedInstruction& instr) {
    SystemParams& params = context->getParams();
    if (instr.postParamType == ParamType::INT) {
        params.heartBeatPeriod = instr.postParam.i;
        SystemPreferences::save(PrefKey::KEY_HEARTBEAT, params.heartBeatPeriod);
    }
    context->getBLETextServer().notifyValue(CMD_SET_HEARTBEAT_PERIOD, params.heartBeatPeriod);
}

void CommandHandler::handlerGetErrorInfo(const ParsedInstruction& instr) {
    // TODO: implement handlerGetErrorInfo
}

void CommandHandler::handlerSetPIDKp(const ParsedInstruction& instr) {
    PIController& leftPI = context->getLeftChannel().getPIController();
    PIController& rightPI = context->getRightChannel().getPIController();

    if (instr.postParamType == ParamType::FLOAT) {
        leftPI.setPIKp(instr.postParam.f);
        rightPI.setPIKp(instr.postParam.f);
        SystemPreferences::save(PrefKey::KEY_PI_KP, leftPI.getPIKp());
    }
    context->getBLETextServer().notifyValue(CMD_SET_PI_KP, leftPI.getPIKp());
}

void CommandHandler::handlerSetPIDKi(const ParsedInstruction& instr) {
    PIController& leftPI = context->getLeftChannel().getPIController();
    PIController& rightPI = context->getRightChannel().getPIController();

    if (instr.postParamType == ParamType::FLOAT) {
        leftPI.setPIKi(instr.postParam.f);
        rightPI.setPIKi(instr.postParam.f);
        SystemPreferences::save(PrefKey::KEY_PI_KI, leftPI.getPIKi());
    }
    context->getBLETextServer().notifyValue(CMD_SET_PI_KI, leftPI.getPIKi());
}

void CommandHandler::handlerReportUserParams(const ParsedInstruction& instr) {
    // TODO: implement handlerReportUserParams
}
