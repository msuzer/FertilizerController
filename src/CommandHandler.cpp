#include <Arduino.h>
#include "CommandHandler.h"
#include "UserInterface.h"
#include "TinyGPSPlus.h"
#include "BLETextServer.h"
#include "DS18B20Sensor.h"
#include "PIController.h"

static constexpr const char* CMD_SET_BLE_DEVICE_NAME        = "setBLEDevName";
static constexpr const char* CMD_GET_DEVICE_INFO            = "getDeviceInfo";
static constexpr const char* CMD_GET_SPEED_INFO             = "getSpeedInfo";
static constexpr const char* CMD_GET_TASK_INFO              = "getTaskInfo";

static constexpr const char* CMD_START_NEW_TASK             = "startNewTask";
static constexpr const char* CMD_PAUSE_TASK                 = "pauseTask";
static constexpr const char* CMD_RESUME_TASK                = "resumeTask";
static constexpr const char* CMD_END_TASK                   = "endTask";
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

extern BLETextServer bleServer;
extern TinyGPSPlus gps;
extern UserInterface ui;
extern PIController pi1;
extern PIController pi2;
BLECommandParser parser;

// when adding new commands, consider increasing 'MAX_COMMANDS' in BLECommandParser
void registerUserCommandHandlers(void) {
    parser.registerCommand(CMD_SET_BLE_DEVICE_NAME, handlerSetBLEDeviceName);
    parser.registerCommand(CMD_GET_DEVICE_INFO, handlerGetDeviceInfo);
    parser.registerCommand(CMD_GET_SPEED_INFO, handlerGetSpeedInfo);
    parser.registerCommand(CMD_GET_TASK_INFO, handlerGetTaskInfo);

    parser.registerCommand(CMD_START_NEW_TASK, handlerStartNewTask);
    parser.registerCommand(CMD_PAUSE_TASK, handlerPauseTask);
    parser.registerCommand(CMD_RESUME_TASK, handlerResumeTask);
    parser.registerCommand(CMD_END_TASK, handlerEndTask);
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
    parser.registerCommand(CMD_REPORT_PID_PARAMS, handlerReportPIDParams);
    parser.registerCommand(CMD_REPORT_USER_PARAMS, handlerReportUserParams);

    parser.sortCommands();
}

void handlerSetBLEDeviceName(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::STRING) {
        printf("New BLE Name = %s\n", instr.postParamStr);
        bleServer.setDeviceName(instr.postParamStr);
    }
}

void handlerGetDeviceInfo(const ParsedInstruction& instr) {
    char jsonBuf[512];
    const char* bleName = bleServer.getDeviceName();
    const char* devUUID = ui.getEspID().c_str();
    const char* sensorUUID = ui.getBoardID().c_str();
    const char* mac = ui.getBleMAC().c_str();
    const char* fwVersion =  FIRMWARE_VERSION;
    const char* devVersion = DEVICE_VERSION;

    snprintf(jsonBuf, sizeof(jsonBuf),
        "{\n"
        "  \"devInfo\": {\n"
        "    \"bleName\": \"%s\",\n"
        "    \"devUUID\": \"%s\",\n"
        "    \"dsUUID\": \"%s\",\n"
        "    \"bleMAC\": \"%s\",\n"
        "    \"fwVer\": \"%s\",\n"
        "    \"devVer\": \"%s\"\n"
        "  }\n"
        "}",
        bleName, devUUID, sensorUUID, mac, fwVersion, devVersion
    );

    Serial.println(jsonBuf);
    bleServer.notify(jsonBuf);
}

void handlerGetSpeedInfo(const ParsedInstruction& instr) {
    char jsonBuf[512];
    const char* speedSrc = ui.getSpeedSource().c_str();
    float minSpeed = ui.getMinWorkingSpeed();
    float simSpeed = ui.getSimSpeed();
    float gpsSpeed = ui.getGPSSpeed();
    int sats = ui.getSatelliteCount();
    Location_t loc = ui.getGPSLocation();

    snprintf(jsonBuf, sizeof(jsonBuf),
        "{\n"
        "  \"gpsInfo\": {\n"
        "    \"spdSrc\": \"%s\",\n"
        "    \"minSpd\": %.2f,\n"
        "    \"simSpd\": %.2f,\n"
        "    \"gpsSpd\": %.2f,\n"
        "    \"lat\": %.6f,\n"
        "    \"lng\": %.6f,\n"
        "    \"sats\": %d\n"
        "  }\n"
        "}",
        speedSrc, minSpeed, simSpeed, gpsSpeed, loc.lat, loc.lng, sats
    );

    Serial.println(jsonBuf);
    bleServer.notify(jsonBuf);
}

void handlerGetTaskInfo(const ParsedInstruction& instr) {
    char jsonBuf[512];
    float flowDaaSet = ui.getTargetFlowRatePerDaa();
    float flowMinSet = ui.getTargetFlowRatePerMin();
    float flowDaaReal = ui.getRealFlowRatePerDaa();
    float flowMinReal = ui.getRealFlowRatePerMin();
    int tankLevel = ui.getTankLevel();
    float areaDone = ui.getAreaCompleted();          // daa
    int duration = ui.getTaskDuration();            // seconds
    float consumed = ui.getLiquidConsumed();         // liters

    snprintf(jsonBuf, sizeof(jsonBuf),
        "{\n"
        "  \"taskinfo\": {\n"
        "    \"flowDaaSet\": %.2f,\n"
        "    \"flowMinSet\": %.2f,\n"
        "    \"flowDaaReal\": %.2f,\n"
        "    \"flowMinReal\": %.2f,\n"
        "    \"tankLevel\": %d,\n"
        "    \"areaDone\": %.2f,\n"
        "    \"duration\": %d,\n"
        "    \"consumed\": %.2f\n"
        "  }\n"
        "}",
        flowDaaSet, flowMinSet, flowDaaReal, flowMinReal,
        tankLevel, areaDone, duration, consumed
    );

    Serial.println(jsonBuf);
    bleServer.notify(jsonBuf);
}

void handlerStartNewTask(const ParsedInstruction& instr) {
    // float ftemp = ui.restoreSingleParam("TankLevel", DEFAULT_VALUE_TANK_INITIAL_LEVEL);
    // ui.setTankLevel(ftemp);

    ui.setTankLevel(DEFAULT_TANK_INITIAL_LEVEL);
    ui.clearTaskDuration();
    ui.clearLiquidConsumed();
    ui.clearAreaCompleted();
    ui.clearDistanceTaken();
    ui.clearAllErrors();

    ui.setTaskState(UserTaskState::Started);
}

void handlerPauseTask(const ParsedInstruction& instr) {
    ui.setTaskState(UserTaskState::Paused);
}

void handlerResumeTask(const ParsedInstruction& instr) {
    ui.setTaskState(UserTaskState::Resuming);
}

void handlerEndTask(const ParsedInstruction& instr) {
    ui.setTaskState(UserTaskState::Stopped);
}

void handlerSetInWorkZone(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        ui.setClientInWorkZone(instr.postParam.i > 0);
    }

    if (ui.isClientInWorkZone()) {
        handlerGetTaskInfo(instr);
    }
}

void handlerSetTargetFlowRatePerDaa(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        ui.setTargetFlowRatePerDaa(instr.postParam.f);
        ui.setTargetFlowRatePerMin(0.0f);
        // ui.saveSingleParam("flowRateDaa", ui.getTargetFlowRatePerMin());
        // ui.saveSingleParam("flowRateMin", ui.getTargetFlowRatePerDaa());
    }

    bleServer.notifyValue(CMD_SET_TARGET_FLOW_RATE_DAA, ui.getTargetFlowRatePerDaa());
}

void handlerSetTargetFlowRatePerMin(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        ui.setTargetFlowRatePerMin(instr.postParam.f);
        ui.setTargetFlowRatePerDaa(0.0f);
        // ui.saveSingleParam("flowRateDaa", ui.getTargetFlowRatePerMin());
        // ui.saveSingleParam("flowRateMin", ui.getTargetFlowRatePerDaa());
    }

    bleServer.notifyValue(CMD_SET_TARGET_FLOW_RATE_MIN, ui.getTargetFlowRatePerMin());
}

void handlerSetMeasuredWeight(const ParsedInstruction& instr) {
    // TODO: implement handlerSetMeasuredWeight
}

void handlerSetSpeedSource(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::STRING) {
        ui.setSpeedSource(instr.postParamStr);
        //ui.saveSingleParam("SpeedSource", ui.getSpeedSource());
    }
    bleServer.notifyString(CMD_SET_SPEED_SOURCE, ui.getSpeedSource());
}

void handlerSetMinWorkingSpeed(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        ui.setMinWorkingSpeed(instr.postParam.f);
        // ui.saveSingleParam("MinSpeed", ui.getMinWorkingSpeed());
    }
    bleServer.notifyValue(CMD_SET_MIN_WORKING_SPEED, ui.getMinWorkingSpeed());
}

void handlerSetSimSpeed(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        ui.setSimSpeed(instr.postParam.f);
        // ui.saveSingleParam("GroundSpeed", ui.getGroundSpeed());
    }
    bleServer.notifyValue(CMD_SET_SIM_SPEED, ui.getSimSpeed());
}

void handlerSetTankLevel(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        ui.setTankLevel(instr.postParam.i);
        // ui.saveSingleParam("TankLevel", ui.getTankLevel());
    }
    bleServer.notifyValue(CMD_SET_TANK_LEVEL, ui.getTankLevel());
}

void handlerSetAutoRefreshPeriod(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        ui.setAutoRefreshPeriod(instr.postParam.i);
        // ui.saveSingleParam("AutoRefresh", ui.getAutoRefreshPeriod());
    }
    bleServer.notifyValue(CMD_SET_AUTO_REFRESH_PERIOD, ui.getAutoRefreshPeriod());
}

void handlerSetHeartBeatPeriod(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        ui.setHeartBeatPeriod(instr.postParam.i);
        //ui.saveSingleParam("HeartBeat", ui.getHeartBeatPeriod());
    }
    bleServer.notifyValue(CMD_SET_HEARTBEAT_PERIOD, ui.getHeartBeatPeriod());
}

void handlerGetErrorInfo(const ParsedInstruction& instr) {
    // TODO: implement handlerGetErrorInfo
}

void handlerSetPIDKp(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        pi1.setPIKp(instr.postParam.f);
        pi2.setPIKp(instr.postParam.f);
        // ui.saveSingleParam("PIKp", pi1.getPIKp());
    }
    bleServer.notifyValue(CMD_SET_PI_KP, pi1.getPIKp());
}

void handlerSetPIDKi(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        pi1.setPIKi(instr.postParam.f);
        pi2.setPIKi(instr.postParam.f);
        // ui.saveSingleParam("PIKi", pi1.getPIKi());
    }
    bleServer.notifyValue(CMD_SET_PI_KI, pi1.getPIKi());
}

void handlerReportPIDParams(const ParsedInstruction& instr) {
    // TODO: implement handlerReportPIDParams
}

void handlerReportUserParams(const ParsedInstruction& instr) {
    // TODO: implement handlerReportUserParams
}
