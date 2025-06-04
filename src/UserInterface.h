#pragma once
#include <Preferences.h>

#define FIRMWARE_VERSION                "04.06.2025"
#define DEVICE_VERSION                  "29.05.2025"

// === Default Parameters ===
#define DEFAULT_BLE_DEVICE_NAME         "AgroFertilizer"
#define DEFAULT_TARGET_RATE_KG_DAA      20.0f
#define DEFAULT_TARGET_FLOW_PER_MIN     15.0f
#define DEFAULT_FLOW_COEFF              1.0f
#define DEFAULT_MIN_WORKING_SPEED       1.0f    // m/Sec.
#define DEFAULT_AUTO_REFRESH_PERIOD     4       // Sec.
#define DEFAULT_HEARTBEAT_PERIOD        25      // Sec.
#define DEFAULT_SPEED_SOURCE            "GPS"
#define DEFAULT_TANK_INITIAL_LEVEL      1000    // Liter
#define DEFAULT_SIM_SPEED               1.0f         // m/sn

#define FLOW_ERROR_WARNING_THRESHOLD    2.0f
#define MIN_SATELLITES_NEEDED           4
#define MAX_HDOP_TOLERATED              25.0f

enum class UserTaskState {
    Stopped,
    Started,
    Paused,
    Resuming
};

enum SpeedSources {
    SPEED_SOURCE_SIM = 0,
    SPEED_SOURCE_GPS,
    SPEED_SOURCE_CAN
};

enum UserErrorCodes {
    NO_ERROR = 0,
    LIQUID_TANK_EMPTY = 1 << 0,
    INSUFFICIENT_FLOW = 1 << 1,
    FLOW_NOT_SETTLED = 1 << 2,
    MOTOR1_STUCK = 1 << 3,
    MOTOR2_STUCK = 1 << 4,
    BATTERY_LOW = 1 << 5,
    NO_SATELLITE_CONNECTED = 1 << 6,
    INVALID_SATELLITE_INFO = 1 << 7,
    INVALID_GPS_LOCATION = 1 << 8,
    INVALID_GPS_SPEED = 1 << 9,
    INVALID_PARAM_COUNT = 1 << 10,
    MESSAGE_PARSE_ERROR = 1 << 11,
    HARDWARE_ERROR = 1 << 12
};

struct Location_t {
    double lat;
    double lng;

    Location_t(double latitude = 0.0, double longitude = 0.0)
        : lat(latitude), lng(longitude) {}
};

class UserInterface {
public:
    UserInterface();
    void begin();

    static constexpr size_t BUFFER_SIZE = 256;

#define SET_AND_SAVE_FLOAT(var, key, value) do { var = value; prefs.begin("UI", false); prefs.putFloat(key, value); prefs.end(); } while(0)
#define SET_AND_SAVE_INT(var, key, value) do { var = value; prefs.begin("UI", false); prefs.putInt(key, value); prefs.end(); } while(0)
#define SET_AND_SAVE_STRING(var, key, value) do { var = value; prefs.begin("UI", false); prefs.putString(key, value); prefs.end(); } while(0)

    // === Setters ===
    inline void setTargetFlowRatePerDaa(float val) { targetFlowRatePerDaa = val; }
    inline void setTargetFlowRatePerMin(float val) { targetFlowRatePerMin = val; }
    inline void setFlowCoeff(float val) { flowCoeff = val; }
    inline void setSpeedSource(const String& val) { speedSource = val; }
    inline void setMinWorkingSpeed(float val) { minWorkingSpeed = val; }
    inline void setAutoRefreshPeriod(int val) { autoRefreshPeriod = val; }
    inline void setHeartBeatPeriod(int val) { heartBeatPeriod = val; }

    inline void setRealFlowRatePerDaa(float val) { realFlowRatePerDaa = val; }
    inline void setRealFlowRatePerMin(float val) { realFlowRatePerMin = val; }
    inline void setSimSpeed(float val) { simSpeed = val; }
    inline void setTankLevel(float val) { tankLevel = val; }
    inline void setLiquidConsumed(float val) { liquidConsumed = val; }
    inline void setAreaCompleted(float val) { areaCompleted = val; }
    inline void setTaskDuration(int val) { taskDuration = val; }
    inline void setDistanceTaken(int val) { distanceTaken = val; }
    inline void setClientInWorkZone(bool val) { clientInWorkZone = val; }

    inline void setBoardID(const String& id) { boardID = id; }
    inline void setEspID(const String& id) { espID = id; }
    inline void setBleMAC(const String& mac) { bleMAC = mac; }

    // === Getters ===
    inline float getTargetFlowRatePerDaa() const { return targetFlowRatePerDaa; }
    inline float getTargetFlowRatePerMin() const { return targetFlowRatePerMin; }
    inline float getFlowCoeff() const { return flowCoeff; }
    inline const String& getSpeedSource() const { return speedSource; }
    inline float getMinWorkingSpeed() const { return minWorkingSpeed; }
    inline int getAutoRefreshPeriod() const { return autoRefreshPeriod; }
    inline int getHeartBeatPeriod() const { return heartBeatPeriod; }

    inline float getRealFlowRatePerDaa() const { return realFlowRatePerDaa; }
    inline float getRealFlowRatePerMin() const { return realFlowRatePerMin; }
    inline float getSimSpeed() const { return simSpeed; }
    inline float getTankLevel() const { return tankLevel; }
    inline float getLiquidConsumed() const { return liquidConsumed; }
    inline float getAreaCompleted() const { return areaCompleted; }
    inline int getTaskDuration() const { return taskDuration; }
    inline int getDistanceTaken() const { return distanceTaken; }
    inline bool isClientInWorkZone() const { return clientInWorkZone; }

    inline const String& getBoardID() const { return boardID; }
    inline const String& getEspID() const { return espID; }
    inline const String& getBleMAC() const { return bleMAC; }

    // === Updaters ====
    inline void clearDistanceTaken(void) { distanceTaken = 0; }
    inline void increaseDistanceTaken(int length) { distanceTaken += length; }

    inline void clearLiquidConsumed(void) { liquidConsumed = 0.0f; }
    inline void increaseLiquidConsumed(float value) { liquidConsumed += value; }

    inline void clearAreaCompleted(void) { areaCompleted = 0.0f; }
    inline void increaseAreaProcessed(float value) { areaCompleted += value; }

    inline void clearTaskDuration(void) { taskDuration = 0; }
    inline void incrementTaskDuration(void) { taskDuration++; }

    inline void decreaseTankLevel(float value) { tankLevel -= value; }

    // === Error Handling ===
    inline void setErrorFlags(uint32_t flags) { errorFlags = flags; }
    inline uint32_t getErrorFlags() const { return errorFlags; }
    inline void setError(uint32_t mask) { errorFlags |= mask; }
    inline void clearError(uint32_t mask) { errorFlags &= ~mask; }
    inline void clearAllErrors() { errorFlags = 0; }
    inline bool hasError(uint32_t mask) const { return (errorFlags & mask) != 0; }
    inline bool hasAnyError(void) const { return (errorFlags != 0); }

    // Preferences
    void loadFromPreferences();
    float restoreSingleParam(const char* key, float defaultValue);
    void saveSingleParam(const char* key, const String& value);
    void saveSingleParam(const char* key, const int value);
    void saveSingleParam(const char* key, const float value);

    // Current task state
    bool setTaskState(UserTaskState state);
    inline UserTaskState getTaskState() const { return taskState; }

    // Convenience checks
    inline bool isTaskStarted() const { return taskState == UserTaskState::Started; }
    inline bool isTaskPaused() const { return taskState == UserTaskState::Paused; }
    inline bool isTaskStopped() const { return taskState == UserTaskState::Stopped; }
    inline bool isTaskResuming() const { return taskState == UserTaskState::Resuming; }
    inline bool isTaskActive() const { return taskState == UserTaskState::Started || taskState == UserTaskState::Resuming; }
    const char* getTaskStateName() const;
    const char* taskStateToString(UserTaskState state) const;

    // GPS Data
    bool isGPSDataValid(void) const;
    Location_t getGPSLocation(void) const;
    int getSatelliteCount(void) const;
    float getGPSSpeed(bool mps = false) const;

private:
    Preferences prefs;

    UserTaskState taskState = UserTaskState::Stopped;

    // User-Configurable
    float targetFlowRatePerDaa;
    float targetFlowRatePerMin;
    float flowCoeff;
    String speedSource;
    float simSpeed;
    float minWorkingSpeed;
    int autoRefreshPeriod;
    int heartBeatPeriod;
    float tankLevel;

    // Dynamic State
    float realFlowRatePerDaa = 0.0f;
    float realFlowRatePerMin = 0.0f;
    float liquidConsumed = 0.0f;
    float areaCompleted = 0.0f;
    int taskDuration = 0;
    int distanceTaken = 0;
    bool clientInWorkZone = false;

    // Error State
    uint32_t errorFlags = NO_ERROR;

    // Hardware IDs
    String boardID;
    String espID;
    String bleMAC;

    String readChipUUID(void);
    String readBLEMAC(void);
    String readDS18B20ID();
};
