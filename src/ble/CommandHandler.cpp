#include <Arduino.h>
#include "CommandHandler.h"
#include "BLECommandParser.h"
#include "BLETextServer.h"
#include "core/SystemContext.h"
#include "gps/GPSProvider.h"
#include "control/PIController.h"
#include "core/SystemPreferences.h"

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

static AppServices* services = nullptr;
static SystemContext* context = nullptr;

void CommandHandler_setServices(AppServices *s) {
    services = s;
    context = s->systemContext;
}

// when adding new commands, consider increasing 'MAX_COMMANDS' in BLECommandParser
void registerUserCommandHandlers(void) {
    BLECommandParser *parser = services->parser;
    parser->registerCommand(CMD_SET_BLE_DEVICE_NAME, handlerSetBLEDeviceName);
    parser->registerCommand(CMD_GET_DEVICE_INFO, handlerGetDeviceInfo);
    parser->registerCommand(CMD_GET_SPEED_INFO, handlerGetSpeedInfo);
    parser->registerCommand(CMD_GET_TASK_INFO, handlerGetTaskInfo);

    parser->registerCommand(CMD_START_NEW_TASK, handlerStartNewTask);
    parser->registerCommand(CMD_PAUSE_TASK, handlerPauseTask);
    parser->registerCommand(CMD_RESUME_TASK, handlerResumeTask);
    parser->registerCommand(CMD_END_TASK, handlerEndTask);
    parser->registerCommand(CMD_SET_IN_WORK_ZONE, handlerSetInWorkZone);
    parser->registerCommand(CMD_SET_TARGET_FLOW_RATE_DAA, handlerSetTargetFlowRatePerDaa);
    parser->registerCommand(CMD_SET_TARGET_FLOW_RATE_MIN, handlerSetTargetFlowRatePerMin);
    parser->registerCommand(CMD_SET_MEASURED_WEIGHT, handlerSetMeasuredWeight);
    parser->registerCommand(CMD_SET_SPEED_SOURCE, handlerSetSpeedSource);
    parser->registerCommand(CMD_SET_MIN_WORKING_SPEED, handlerSetMinWorkingSpeed);
    parser->registerCommand(CMD_SET_SIM_SPEED, handlerSetSimSpeed);
    parser->registerCommand(CMD_SET_TANK_LEVEL, handlerSetTankLevel);
    parser->registerCommand(CMD_SET_AUTO_REFRESH_PERIOD, handlerSetAutoRefreshPeriod);
    parser->registerCommand(CMD_SET_HEARTBEAT_PERIOD, handlerSetHeartBeatPeriod);
    parser->registerCommand(CMD_GET_ERROR_INFO, handlerGetErrorInfo);
    parser->registerCommand(CMD_SET_PI_KP, handlerSetPIDKp);
    parser->registerCommand(CMD_SET_PI_KI, handlerSetPIDKi);
    parser->registerCommand(CMD_REPORT_PID_PARAMS, handlerReportPIParams);
    parser->registerCommand(CMD_REPORT_USER_PARAMS, handlerReportUserParams);

    parser->sortCommands();
}

void handlerSetBLEDeviceName(const ParsedInstruction &instr) {
    if (instr.postParamType == ParamType::STRING) {
        printf("New BLE Name = %s\n", instr.postParamStr);
        services->bleServer->setDeviceName(instr.postParamStr);
    }
}

void handlerGetDeviceInfo(const ParsedInstruction& instr) {
    char jsonBuf[512];
    const char* bleName = services->bleServer->getDeviceName();
    const char* devUUID = services->systemContext->getEspID().c_str();
    const char* sensorUUID = services->systemContext->getBoardID().c_str();
    const char* mac = services->systemContext->getBleMAC().c_str();
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
    services->bleServer->notify(jsonBuf);
}

void handlerGetSpeedInfo(const ParsedInstruction& instr) {
    char jsonBuf[512];
    const char* speedSrc = services->systemContext->getSpeedSource().c_str();
    float minSpeed = services->systemContext->getMinWorkingSpeed();
    float simSpeed = services->systemContext->getSimSpeed();
    float gpsSpeed = services->gpsProvider->getSpeed();
    int sats = services->gpsProvider->getSatelliteCount();
    Location_t loc = services->gpsProvider->getLocation();

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
    services->bleServer->notify(jsonBuf);
}

void handlerGetTaskInfo(const ParsedInstruction& instr) {
    char jsonBuf[512];
    float flowDaaSet = services->systemContext->getLeftChannel().getTargetFlowRatePerDaa();
    float flowMinSet = services->systemContext->getLeftChannel().getTargetFlowRatePerMin();
    float flowDaaReal = services->systemContext->getLeftChannel().getRealFlowRatePerDaa();
    float flowMinReal = services->systemContext->getLeftChannel().getRealFlowRatePerMin();
    int tankLevel = services->systemContext->getTankLevel();
    float areaDone = context->getLeftChannel().getAreaCompleted();          // daa
    int duration = context->getLeftChannel().getTaskDuration();            // seconds
    float consumed = context->getLeftChannel().getLiquidConsumed();         // liters

    snprintf(jsonBuf, sizeof(jsonBuf),
        "{\n"
        "  \"taskInfo\": {\n"
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
    services->bleServer->notify(jsonBuf);
}

void handlerReportPIParams(const ParsedInstruction& instr) {
    char jsonBuf[512];
    float piKp = services->pi1->getPIKp();
    float piKi = services->pi1->getPIKi();

    snprintf(jsonBuf, sizeof(jsonBuf),
        "{\n"
        "  \"piInfo\": {\n"
        "    \"piKp\": %.2f,\n"
        "    \"piKi\": %.2f,\n"
        "  }\n"
        "}",
        piKp, piKi
    );

    Serial.println(jsonBuf);
    services->bleServer->notify(jsonBuf);
}

void handlerStartNewTask(const ParsedInstruction& instr) {
    float ftemp = services->prefs->getFloat(PrefKey::KEY_TANK_LEVEL, DEFAULT_TANK_INITIAL_LEVEL);
    services->systemContext->setTankLevel(ftemp);

    context->getLeftChannel().clearTaskDuration();
    context->getLeftChannel().clearLiquidConsumed();
    context->getLeftChannel().clearAreaCompleted();
    context->getLeftChannel().clearDistanceTaken();
    services->systemContext->getLeftChannel().clearAllErrors();

    services->systemContext->getLeftChannel().setTaskState(UserTaskState::Started);
}

void handlerPauseTask(const ParsedInstruction& instr) {
    services->systemContext->getLeftChannel().setTaskState(UserTaskState::Paused);
}

void handlerResumeTask(const ParsedInstruction& instr) {
    services->systemContext->getLeftChannel().setTaskState(UserTaskState::Resuming);
}

void handlerEndTask(const ParsedInstruction& instr) {
    services->systemContext->getLeftChannel().setTaskState(UserTaskState::Stopped);
}

void handlerSetInWorkZone(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        context->setClientInWorkZone(instr.postParam.i > 0);
    }

    if (context->isClientInWorkZone()) {
        handlerGetTaskInfo(instr);
    }
}

void handlerSetTargetFlowRatePerDaa(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        services->systemContext->getLeftChannel().setTargetFlowRatePerDaa(instr.postParam.f);
        services->systemContext->getLeftChannel().setTargetFlowRatePerMin(0.0f);
        services->prefs->save(PrefKey::KEY_LEFT_RATE_DAA, services->systemContext->getLeftChannel().getTargetFlowRatePerMin());
        services->prefs->save(PrefKey::KEY_LEFT_RATE_MIN, services->systemContext->getLeftChannel().getTargetFlowRatePerDaa());
    }

    services->bleServer->notifyValue(CMD_SET_TARGET_FLOW_RATE_DAA, services->systemContext->getLeftChannel().getTargetFlowRatePerDaa());
}

void handlerSetTargetFlowRatePerMin(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        services->systemContext->getLeftChannel().setTargetFlowRatePerMin(instr.postParam.f);
        services->systemContext->getLeftChannel().setTargetFlowRatePerDaa(0.0f);
        services->prefs->save(PrefKey::KEY_LEFT_RATE_DAA, services->systemContext->getLeftChannel().getTargetFlowRatePerMin());
        services->prefs->save(PrefKey::KEY_LEFT_RATE_MIN, services->systemContext->getLeftChannel().getTargetFlowRatePerDaa());
    }

    services->bleServer->notifyValue(CMD_SET_TARGET_FLOW_RATE_MIN, services->systemContext->getLeftChannel().getTargetFlowRatePerMin());
}

void handlerSetMeasuredWeight(const ParsedInstruction& instr) {
    // TODO: implement handlerSetMeasuredWeight
}

void handlerSetSpeedSource(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::STRING) {
        services->systemContext->setSpeedSource(instr.postParamStr);
        services->prefs->save(PrefKey::KEY_SPEED_SRC, services->systemContext->getSpeedSource());
    }
    services->bleServer->notifyString(CMD_SET_SPEED_SOURCE, services->systemContext->getSpeedSource());
}

void handlerSetMinWorkingSpeed(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        services->systemContext->setMinWorkingSpeed(instr.postParam.f);
        services->prefs->save(PrefKey::KEY_MIN_SPEED, services->systemContext->getMinWorkingSpeed());
    }
    services->bleServer->notifyValue(CMD_SET_MIN_WORKING_SPEED, services->systemContext->getMinWorkingSpeed());
}

void handlerSetSimSpeed(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        services->systemContext->setSimSpeed(instr.postParam.f);
        services->prefs->save(PrefKey::KEY_SIM_SPEED, services->systemContext->getSimSpeed());
    }
    services->bleServer->notifyValue(CMD_SET_SIM_SPEED, services->systemContext->getSimSpeed());
}

void handlerSetTankLevel(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        services->systemContext->setTankLevel(instr.postParam.i);
        services->prefs->save(PrefKey::KEY_TANK_LEVEL, services->systemContext->getTankLevel());
    }
    services->bleServer->notifyValue(CMD_SET_TANK_LEVEL, services->systemContext->getTankLevel());
}

void handlerSetAutoRefreshPeriod(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        services->systemContext->setAutoRefreshPeriod(instr.postParam.i);
        services->prefs->save(PrefKey::KEY_REFRESH, services->systemContext->getAutoRefreshPeriod());
    }
    services->bleServer->notifyValue(CMD_SET_AUTO_REFRESH_PERIOD, services->systemContext->getAutoRefreshPeriod());
}

void handlerSetHeartBeatPeriod(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::INT) {
        services->systemContext->setHeartBeatPeriod(instr.postParam.i);
        services->prefs->save(PrefKey::KEY_HEARTBEAT, services->systemContext->getHeartBeatPeriod());
    }
    services->bleServer->notifyValue(CMD_SET_HEARTBEAT_PERIOD, services->systemContext->getHeartBeatPeriod());
}

void handlerGetErrorInfo(const ParsedInstruction& instr) {
    // TODO: implement handlerGetErrorInfo
}

void handlerSetPIDKp(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        services->pi1->setPIKp(instr.postParam.f);
        services->pi2->setPIKp(instr.postParam.f);
        services->prefs->save(PrefKey::KEY_PI_KP, services->pi1->getPIKp());
    }
    services->bleServer->notifyValue(CMD_SET_PI_KP, services->pi1->getPIKp());
}

void handlerSetPIDKi(const ParsedInstruction& instr) {
    if (instr.postParamType == ParamType::FLOAT) {
        services->pi1->setPIKi(instr.postParam.f);
        services->pi2->setPIKi(instr.postParam.f);
        services->prefs->save(PrefKey::KEY_PI_KI, services->pi1->getPIKi());
    }
    services->bleServer->notifyValue(CMD_SET_PI_KI, services->pi1->getPIKi());
}

void handlerReportUserParams(const ParsedInstruction& instr) {
    // TODO: implement handlerReportUserParams
}
