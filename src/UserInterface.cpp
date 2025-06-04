#include "UserInterface.h"
#include "BLETextServer.h"
#include "TinyGPSPlus.h"
#include "CommandHandler.h"
#include "PIController.h"
#include "DS18B20Sensor.h"
#include <ESP32-hal.h>  // For ESP.getEfuseMac()

#define SAVE_VALVE_STATE_PERSISTENTLY

#define VALVE_CHANGE_STATE_TIMEOUT  4 // 4 seconds

static const char* prefName = "UIData";

extern const char* strSetValve;
extern const char* strReportWarning;

extern BLETextServer bleServer;
extern BLECommandParser parser;
extern TinyGPSPlus gpsModule;
extern PIController pi1;
extern PIController pi2;

UserInterface::UserInterface() {}

void UserInterface::begin() {
  espID = readChipUUID();
  bleMAC = readBLEMAC();
  boardID = readDS18B20ID();

  printf("Chip ID: %s | BLE MAC: %s | Board ID: %s\n", espID, bleMAC, boardID);

  loadFromPreferences();
}

// === Preferences ===
void UserInterface::loadFromPreferences() {
  realFlowRatePerDaa = 0.0f;
  realFlowRatePerMin = 0.0f;
  liquidConsumed = 0.0f;
  areaCompleted = 0.0f;
  taskDuration = 0;
  distanceTaken = 0;
  clientInWorkZone = false;
  errorFlags = NO_ERROR;

  prefs.begin(prefName, true);
  targetFlowRatePerDaa = prefs.getFloat("rateDaa", DEFAULT_TARGET_RATE_KG_DAA);
  targetFlowRatePerMin = prefs.getFloat("rateMin", DEFAULT_TARGET_FLOW_PER_MIN);
  flowCoeff = prefs.getFloat("flowCoeff", DEFAULT_FLOW_COEFF);
  speedSource = prefs.getString("speedSrc", DEFAULT_SPEED_SOURCE);
  simSpeed = prefs.getFloat("simSpeed", DEFAULT_SIM_SPEED);
  minWorkingSpeed = prefs.getFloat("minSpeed", DEFAULT_MIN_WORKING_SPEED);
  autoRefreshPeriod = prefs.getInt("refresh", DEFAULT_AUTO_REFRESH_PERIOD);
  heartBeatPeriod = prefs.getInt("heartbeat", DEFAULT_HEARTBEAT_PERIOD);
  tankLevel = prefs.getFloat("tankLevel", DEFAULT_TANK_INITIAL_LEVEL);
  prefs.end();
}

String UserInterface::readDS18B20ID() {
    auto& sensor = DS18B20Sensor::getInstance();
    if (sensor.isReady()) {
        return sensor.getSensorID();
    }

    return "DS18B20 Not Found";
}

String UserInterface::readChipUUID(void) {
    uint64_t chipID = ESP.getEfuseMac();
    return String((uint32_t)(chipID >> 32), HEX) + String((uint32_t)chipID, HEX);
}

String UserInterface::readBLEMAC(void) {
    uint8_t mac[6];
    char macStr[18];
    esp_read_mac(mac, ESP_MAC_BT);
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

bool UserInterface::setTaskState(UserTaskState state) {
    bool validOp = false;

    switch (taskState) {
        case UserTaskState::Stopped:
            validOp = (state == UserTaskState::Started);
            break;
        case UserTaskState::Started:
            validOp = (state == UserTaskState::Paused || state == UserTaskState::Stopped);
            break;
        case UserTaskState::Paused:
            validOp = (state == UserTaskState::Resuming || state == UserTaskState::Stopped);
            break;
        case UserTaskState::Resuming:
            validOp = (state == UserTaskState::Started || state == UserTaskState::Paused || state == UserTaskState::Stopped);
            break;
    }

    if (validOp) {
        taskState = state;
        return true;
    }

    printf("[UI] Invalid state transition: %s → %s\n", getTaskStateName(), taskStateToString(state));
    return false;
}

const char* UserInterface::taskStateToString(UserTaskState state) const {
    switch (state) {
        case UserTaskState::Stopped:  return "Stopped";
        case UserTaskState::Started:  return "Started";
        case UserTaskState::Paused:   return "Paused";
        case UserTaskState::Resuming: return "Resuming";
        default:                      return "Unknown";
    }
}

const char* UserInterface::getTaskStateName() const {
    return taskStateToString(taskState);
}

bool UserInterface::isGPSDataValid(void) const {
  bool valid = 
    gpsModule.location.isValid() &&
    gpsModule.speed.isValid() &&
    gpsModule.satellites.isValid() &&
    gpsModule.satellites.value() >= MIN_SATELLITES_NEEDED &&
    gpsModule.hdop.isValid() &&
    gpsModule.hdop.hdop() <= MAX_HDOP_TOLERATED;

  return valid;
}

int UserInterface::getSatelliteCount(void) const {
  int sats = 0;
  if (gpsModule.satellites.isValid()) {
    sats = gpsModule.satellites.value();
  }

  return sats;
}

float UserInterface::getGPSSpeed(bool mps) const {
  float retVal = 0.0f;

  if (isGPSDataValid()) {
      retVal = mps ? gpsModule.speed.mps() : gpsModule.speed.kmph();
  }

  return retVal;
}

Location_t UserInterface::getGPSLocation(void) const {
    if (isGPSDataValid()) {
        double lat = gpsModule.location.lat();
        double lng = gpsModule.location.lng();
        return Location_t(lat, lng);
    }

    return Location_t();
}