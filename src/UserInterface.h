#pragma once
#include <Preferences.h>

// === Default Parameters ===
#define DEFAULT_TARGET_RATE_KG_DAA   20.0f
#define DEFAULT_TARGET_FLOW_PER_MIN  15.0f
#define DEFAULT_FLOW_COEFF           1.0f
#define DEFAULT_FLOW_MIN_VALUE       0.5f
#define DEFAULT_FLOW_MAX_VALUE       50.0f
#define DEFAULT_BOOM_WIDTH           12.0f    // meters
#define DEFAULT_MIN_WORKING_SPEED    3.0f     // km/h
#define DEFAULT_AUTO_REFRESH_PERIOD  1000     // ms
#define DEFAULT_HEARTBEAT_PERIOD     2000     // ms
#define DEFAULT_SPEED_SOURCE         0        // GPS

enum UserErrorCodes {
    NO_ERROR = 0,
    LIQUID_TANK_EMPTY = 1 << 0,
    INSUFFICIENT_FLOW = 1 << 1,
    FLOW_NOT_SETTLED = 1 << 2,
    LOW_PRESSURE = 1 << 3,
    HIGH_PRESSURE = 1 << 4,
    BATTERY_LOW = 1 << 5,
    NO_SATELLITE_CONNECTED = 1 << 6,
    INVALID_SATELLITE_INFO = 1 << 7,
    INVALID_GPS_LOCATION = 1 << 8,
    INVALID_GPS_SPEED = 1 << 9,
    INVALID_PARAM_COUNT = 1 << 10,
    MESSAGE_PARSE_ERROR = 1 << 11,
    HARDWARE_ERROR = 1 << 12
};

class UserInterface {
public:
    UserInterface();
    void begin();

#define SET_AND_SAVE_FLOAT(var, key, value) do { var = value; prefs.begin("UI", false); prefs.putFloat(key, value); prefs.end(); } while(0)
#define SET_AND_SAVE_INT(var, key, value) do { var = value; prefs.begin("UI", false); prefs.putInt(key, value); prefs.end(); } while(0)

    // === Inline Setters with Persistence ===
    inline void setTargetFlowRatePerDaa(float val) { SET_AND_SAVE_FLOAT(targetFlowRatePerDaa, "rateDaa", val); }
    inline void setTargetFlowRatePerMin(float val) { SET_AND_SAVE_FLOAT(targetFlowRatePerMin, "rateMin", val); }
    inline void setFlowCoeff(float val) { SET_AND_SAVE_FLOAT(flowCoeff, "flowCoeff", val); }
    inline void setFlowMinValue(float val) { SET_AND_SAVE_FLOAT(flowMinValue, "flowMin", val); }
    inline void setFlowMaxValue(float val) { SET_AND_SAVE_FLOAT(flowMaxValue, "flowMax", val); }
    inline void setBoomWidth(float val) { SET_AND_SAVE_FLOAT(boomWidth, "boomWidth", val); }
    inline void setMinWorkingSpeed(float val) { SET_AND_SAVE_FLOAT(minWorkingSpeed, "minSpeed", val); }
    inline void setAutoRefreshPeriod(int val) { SET_AND_SAVE_INT(autoRefreshPeriod, "refresh", val); }
    inline void setHeartBeatPeriod(int val) { SET_AND_SAVE_INT(heartBeatPeriod, "heartbeat", val); }
    inline void setSpeedSource(int val) { SET_AND_SAVE_INT(speedSource, "speedSrc", val); }
    inline void setLeftActuatorOffset(float val) { SET_AND_SAVE_FLOAT(leftActuatorOffset, "leftOffset", val); }
    inline void setRightActuatorOffset(float val) { SET_AND_SAVE_FLOAT(rightActuatorOffset, "rightOffset", val); }

    // === Inline Getters/Setters ===
    inline float getTargetFlowRatePerDaa() const { return targetFlowRatePerDaa; }
    inline float getTargetFlowRatePerMin() const { return targetFlowRatePerMin; }
    inline float getFlowCoeff() const { return flowCoeff; }
    inline float getFlowMinValue() const { return flowMinValue; }
    inline float getFlowMaxValue() const { return flowMaxValue; }
    inline float getBoomWidth() const { return boomWidth; }
    inline float getMinWorkingSpeed() const { return minWorkingSpeed; }
    inline int getAutoRefreshPeriod() const { return autoRefreshPeriod; }
    inline int getHeartBeatPeriod() const { return heartBeatPeriod; }
    inline int getSpeedSource() const { return speedSource; }
    inline float getLeftActuatorOffset() const { return leftActuatorOffset; }
    inline float getRightActuatorOffset() const { return rightActuatorOffset; }

    // === Dynamic State Updates ===
    inline void updateGroundSpeed(float speed) { groundSpeed = speed; }
    inline void updateTankLevel(float level) { tankLevel = level; updateLowTankWarning(); }
    inline void updateFlowRates(float perMin, float perDaa) { flowRatePerMin = perMin; flowRatePerDaa = perDaa; }
    inline void updateActuatorPositions(float leftPos, float rightPos) { leftActuatorPosition = leftPos; rightActuatorPosition = rightPos; }
    inline void updateLiquidConsumed(float consumed) { liquidConsumed = consumed; }
    inline void updateAreaCompleted(float area) { areaCompleted = area; }
    inline void updateTaskDuration(uint32_t duration) { taskDuration = duration; }
    inline void updateDistanceTaken(uint32_t distance) { distanceTaken = distance; }
    inline void setClientInWorkZone(bool inZone) { clientInWorkZone = inZone; }
    inline void setBtDevIndex(int index) { btDevIndex = index; }

    // === Error Handling ===
    inline void setError(UserErrorCodes error) { errorFlags |= error; }
    inline void clearError(UserErrorCodes error) { errorFlags &= ~error; }
    inline void clearAllErrors() { errorFlags = NO_ERROR; }
    inline bool hasError(UserErrorCodes error) const { return (errorFlags & error); }
    inline uint32_t getErrorFlags() const { return errorFlags; }

    // === Hardware IDs ===
    inline String getBoardID() const { return boardID; }
    inline String getEspID() const { return espID; }
    inline String getBleMAC() const { return bleMAC; }

    // Reporting
    void reportStatus();

    // Preferences
    void loadFromPreferences();
    void saveToPreferences();  // Optional, not used unless batch save

private:
    Preferences prefs;

    // User-Configurable
    float targetFlowRatePerDaa;
    float targetFlowRatePerMin;
    float flowCoeff;
    float flowMinValue;
    float flowMaxValue;
    float boomWidth;
    float minWorkingSpeed;
    int autoRefreshPeriod;
    int heartBeatPeriod;
    int speedSource;
    float leftActuatorOffset;
    float rightActuatorOffset;

    // Dynamic State
    float flowRatePerMin = 0.0f;
    float flowRatePerDaa = 0.0f;
    float groundSpeed = 0.0f;
    float tankLevel = 0.0f;
    float liquidConsumed = 0.0f;
    float areaCompleted = 0.0f;
    int taskDuration = 0;
    int distanceTaken = 0;
    float leftActuatorPosition = 0.0f;
    float rightActuatorPosition = 0.0f;
    bool clientInWorkZone = false;
    int btDevIndex = -1;

    // Error State
    uint32_t errorFlags = NO_ERROR;

    // Hardware IDs
    String boardID;
    String espID;
    String bleMAC;

    void updateLowTankWarning();
    String readDS18B20ID();  // You’ll implement this
};
