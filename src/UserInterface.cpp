#include "UserInterface.h"
#include "BLETextServer.h"
#include "TinyGPSPlus.h"
#include "CommandHandler.h"
#include "PIController.h"
#include <ESP32-hal.h>  // For ESP.getEfuseMac()

#define SAVE_VALVE_STATE_PERSISTENTLY

#define VALVE_CHANGE_STATE_TIMEOUT  4 // 4 seconds

const char* strBLEDeviceName = "AgroDose";
const char* prefName = "userdata";

extern const char* strSetValve;
extern const char* strReportWarning;

extern BLETextServer bleServer;
extern BLECommandParser parser;
extern TinyGPSPlus gpsModule;
extern PIController pi1;
extern PIController pi2;

UserInterface::UserInterface() {}

void UserInterface::begin() {
    // ESP ID
    uint64_t chipID = ESP.getEfuseMac();
    espID = String((uint32_t)(chipID >> 32), HEX) + String((uint32_t)chipID, HEX);

    // BLE MAC
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    bleMAC = String(macStr);

    // DS18B20 ID
    boardID = readDS18B20ID();  // You'll provide implementation

    // Load preferences
    loadFromPreferences();
}

// === Status Reporting ===
void UserInterface::reportStatus() {
    bleServer.notifyValue("flowPerMin", flowRatePerMin);
    // Assume bleServer.notifyValue(key, value) is available
    bleServer.notifyValue("flowPerMin", flowRatePerMin);
    bleServer.notifyValue("flowPerDaa", flowRatePerDaa);
    bleServer.notifyValue("speed", groundSpeed);
    bleServer.notifyValue("tankLevel", tankLevel);
    bleServer.notifyValue("liquidConsumed", liquidConsumed);
    bleServer.notifyValue("areaCompleted", areaCompleted);
    bleServer.notifyValue("taskDuration", taskDuration);
    bleServer.notifyValue("distanceTaken", distanceTaken);
    bleServer.notifyValue("leftActuatorPos", leftActuatorPosition);
    bleServer.notifyValue("rightActuatorPos", rightActuatorPosition);
    bleServer.notifyValue("clientInWorkZone", clientInWorkZone);
    bleServer.notifyValue("btDevIndex", btDevIndex);
    bleServer.notifyValue("errorFlags", (int) errorFlags);
    bleServer.notifyString("boardID", boardID);
    bleServer.notifyString("espID", espID);
    bleServer.notifyString("bleMAC", bleMAC);
}

// === Preferences ===
void UserInterface::loadFromPreferences() {
    prefs.begin("UI", true);
    targetFlowRatePerDaa = prefs.getFloat("rateDaa", DEFAULT_TARGET_RATE_KG_DAA);
    targetFlowRatePerMin = prefs.getFloat("rateMin", DEFAULT_TARGET_FLOW_PER_MIN);
    flowCoeff = prefs.getFloat("flowCoeff", DEFAULT_FLOW_COEFF);
    flowMinValue = prefs.getFloat("flowMin", DEFAULT_FLOW_MIN_VALUE);
    flowMaxValue = prefs.getFloat("flowMax", DEFAULT_FLOW_MAX_VALUE);
    boomWidth = prefs.getFloat("boomWidth", DEFAULT_BOOM_WIDTH);
    minWorkingSpeed = prefs.getFloat("minSpeed", DEFAULT_MIN_WORKING_SPEED);
    autoRefreshPeriod = prefs.getInt("refresh", DEFAULT_AUTO_REFRESH_PERIOD);
    heartBeatPeriod = prefs.getInt("heartbeat", DEFAULT_HEARTBEAT_PERIOD);
    speedSource = prefs.getInt("speedSrc", DEFAULT_SPEED_SOURCE);
    leftActuatorOffset = prefs.getFloat("leftOffset", 0.0f);
    rightActuatorOffset = prefs.getFloat("rightOffset", 0.0f);
    prefs.end();
}

void UserInterface::saveToPreferences() {
    // Optional: Save all in one go
    setTargetFlowRatePerDaa(targetFlowRatePerDaa);
    setTargetFlowRatePerMin(targetFlowRatePerMin);
    setFlowCoeff(flowCoeff);
    setFlowMinValue(flowMinValue);
    setFlowMaxValue(flowMaxValue);
    setBoomWidth(boomWidth);
    setMinWorkingSpeed(minWorkingSpeed);
    setAutoRefreshPeriod(autoRefreshPeriod);
    setHeartBeatPeriod(heartBeatPeriod);
    setSpeedSource(speedSource);
    setLeftActuatorOffset(leftActuatorOffset);
    setRightActuatorOffset(rightActuatorOffset);
}

// === Placeholder: DS18B20 ID ===
String UserInterface::readDS18B20ID() {
    // You’ll implement OneWire + DallasTemperature logic here
    return "28FF123456789ABC";
}

// === Low Tank Warning ===
void UserInterface::updateLowTankWarning() {
    if (tankLevel <= 0.1f * flowMinValue) setError(LIQUID_TANK_EMPTY);
    else clearError(LIQUID_TANK_EMPTY);
}
