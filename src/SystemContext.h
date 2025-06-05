// SystemContext.h (renamed from UserInterface.h)
#pragma once
#include <Preferences.h>
#include "DispenserChannel.h"
#include "GPSProvider.h"
#include "AppServices.h"

#define DEFAULT_BLE_DEVICE_NAME         "AgroFertilizer"
#define FIRMWARE_VERSION                "04.06.2025"
#define DEVICE_VERSION                  "29.05.2025"

enum SpeedSources {
    SPEED_SOURCE_SIM = 0,
    SPEED_SOURCE_GPS
};

class SystemContext {
public:
    SystemContext();
    void begin();

    void injectServices(AppServices* s)  { services = s; }

    inline DispenserChannel& getLeftChannel() { return leftChannel; }
    inline DispenserChannel& getRightChannel() { return rightChannel; }

    inline const DispenserChannel& getLeftChannel() const { return leftChannel; }
    inline const DispenserChannel& getRightChannel() const { return rightChannel; }

    // Shared state setters
    inline void setSimSpeed(float val) { simSpeed = val; }
    inline void setTankLevel(float val) { tankLevel = val; }
    inline void setLiquidConsumed(float val) { liquidConsumed = val; }
    inline void setAreaCompleted(float val) { areaCompleted = val; }
    inline void setTaskDuration(int val) { taskDuration = val; }
    inline void setDistanceTaken(int val) { distanceTaken = val; }
    inline void setClientInWorkZone(bool val) { clientInWorkZone = val; }
    inline void setSpeedSource(const String& val) { speedSource = val; }
    inline void setMinWorkingSpeed(float val) { minWorkingSpeed = val; }
    inline void setAutoRefreshPeriod(int val) { autoRefreshPeriod = val; }
    inline void setHeartBeatPeriod(int val) { heartBeatPeriod = val; }

    // Shared state getters
    inline const String& getSpeedSource() const { return speedSource; }
    inline float getMinWorkingSpeed() const { return minWorkingSpeed; }
    inline int getAutoRefreshPeriod() const { return autoRefreshPeriod; }
    inline int getHeartBeatPeriod() const { return heartBeatPeriod; }

    inline float getSimSpeed() const { return simSpeed; }
    inline float getTankLevel() const { return tankLevel; }
    inline float getLiquidConsumed() const { return liquidConsumed; }
    inline float getAreaCompleted() const { return areaCompleted; }
    inline int getTaskDuration() const { return taskDuration; }
    inline int getDistanceTaken() const { return distanceTaken; }
    inline bool isClientInWorkZone() const { return clientInWorkZone; }

    // Task Metrics Updaters
    inline void clearDistanceTaken(void) { distanceTaken = 0; }
    inline void increaseDistanceTaken(int length) { distanceTaken += length; }

    inline void clearLiquidConsumed(void) { liquidConsumed = 0.0f; }
    inline void increaseLiquidConsumed(float value) { liquidConsumed += value; }

    inline void clearAreaCompleted(void) { areaCompleted = 0.0f; }
    inline void increaseAreaProcessed(float value) { areaCompleted += value; }

    inline void clearTaskDuration(void) { taskDuration = 0; }
    inline void incrementTaskDuration(void) { taskDuration++; }

    inline void decreaseTankLevel(float value) { tankLevel -= value; }

    // Identity
    inline void setBoardID(const String& id) { boardID = id; }
    inline void setEspID(const String& id) { espID = id; }
    inline void setBleMAC(const String& mac) { bleMAC = mac; }
    
    inline const String& getBoardID() const { return boardID; }
    inline const String& getEspID() const { return espID; }
    inline const String& getBleMAC() const { return bleMAC; }

private:
    AppServices* services = nullptr;

    DispenserChannel leftChannel;
    DispenserChannel rightChannel;

    String speedSource;
    float simSpeed;
    float minWorkingSpeed;
    int autoRefreshPeriod;
    int heartBeatPeriod;
    float tankLevel;

    float liquidConsumed = 0.0f;
    float areaCompleted = 0.0f;
    int taskDuration = 0;
    int distanceTaken = 0;
    bool clientInWorkZone = false;

    String boardID;
    String espID;
    String bleMAC;

    String readChipUUID();
    String readBLEMAC();
    String readDS18B20ID();
};
