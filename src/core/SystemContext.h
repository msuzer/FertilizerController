#pragma once
#include <Preferences.h>
#include "gps/GPSProvider.h"
#include "core/AppServices.h"
#include "control/DispenserChannel.h"

#define DEFAULT_BLE_DEVICE_NAME         "AgroFertilizer"
#define FIRMWARE_VERSION                "04.06.2025"
#define DEVICE_VERSION                  "29.05.2025"

class SystemContext {
public:
    SystemContext() : leftChannel("Left"), rightChannel("Right") {}
    void begin();

    void injectServices(AppServices* s)  { services = s; }
    AppServices* getServices() { return services; }

    // Shared state setters
    inline void setAutoRefreshPeriod(int val) { autoRefreshPeriod = val; }
    inline void setHeartBeatPeriod(int val) { heartBeatPeriod = val; }

    inline DispenserChannel& getLeftChannel() { return leftChannel; }
    inline DispenserChannel& getRightChannel() { return rightChannel; }

    inline const DispenserChannel& getLeftChannel() const { return leftChannel; }
    inline const DispenserChannel& getRightChannel() const { return rightChannel; }
    
    bool isClientInWorkZone() const { return clientInWorkZone; }
    void setClientInWorkZone(bool val) { clientInWorkZone = val; }

    inline float getMinWorkingSpeed() const { return minWorkingSpeed; }
    inline void setMinWorkingSpeed(float val) { minWorkingSpeed = val; }
    inline void setTankLevel(float val) { tankLevel = val; }
    inline float getTankLevel() const { return tankLevel; }
    inline void decreaseTankLevel(float value) { tankLevel -= value; }

    inline void setSimSpeed(float val) { simSpeed = val; }
    inline void setSpeedSource(const String& val) { speedSource = val; }
    float getGroundSpeed(bool useSim = false) const;
    bool isSpeedOK() const { return (getGroundSpeed() >= minWorkingSpeed); }
    inline const String& getSpeedSource() const { return speedSource; }
    inline float getSimSpeed() const { return simSpeed; }

    // Shared state getters
    inline int getAutoRefreshPeriod() const { return autoRefreshPeriod; }
    inline int getHeartBeatPeriod() const { return heartBeatPeriod; }

    // Identity
    inline void setBoardID(const String& id) { boardID = id; }
    inline void setEspID(const String& id) { espID = id; }
    inline void setBleMAC(const String& mac) { bleMAC = mac; }

    inline const String& getBoardID() const { return boardID; }
    inline const String& getEspID() const { return espID; }
    inline const String& getBleMAC() const { return bleMAC; }

    // BLE Callbacks
    void onBLEConnected();
    void onBLEDisconnected();
    void onBLEWrite(const String& characteristic, const String& value);
    void onBLERead(const String& characteristic);

private:
    AppServices* services = nullptr;

    DispenserChannel leftChannel;
    DispenserChannel rightChannel;

    bool clientInWorkZone = false;

    String speedSource;
    float simSpeed;
    float minWorkingSpeed = 0.0f;
    float tankLevel = 0.0f; // in liters, used for area calculations
    int autoRefreshPeriod;
    int heartBeatPeriod;
    
    String boardID;
    String espID;
    String bleMAC;

    String readChipUUID();
    String readBLEMAC();
    String readDS18B20ID();
};
