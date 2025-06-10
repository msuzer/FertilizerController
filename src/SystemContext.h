#pragma once
#include <Preferences.h>
#include "DispenserChannel.h"
#include "GPSProvider.h"
#include "AppServices.h"

#define DEFAULT_BLE_DEVICE_NAME         "AgroFertilizer"
#define FIRMWARE_VERSION                "04.06.2025"
#define DEVICE_VERSION                  "29.05.2025"

class SystemContext {
public:
    SystemContext();
    void begin();

    void injectServices(AppServices* s)  { services = s; }
    inline AppServices* getServices() const { return services; }

    // Shared state setters
    inline void setAutoRefreshPeriod(int val) { autoRefreshPeriod = val; }
    inline void setHeartBeatPeriod(int val) { heartBeatPeriod = val; }

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

    int autoRefreshPeriod;
    int heartBeatPeriod;

    String boardID;
    String espID;
    String bleMAC;

    String readChipUUID();
    String readBLEMAC();
    String readDS18B20ID();
};
