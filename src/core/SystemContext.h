#pragma once
#include <Preferences.h>
#include <TinyGPSPlus.h>

#include "SystemPreferences.h"
#include "ble/BLETextServer.h"
#include "ble/BLECommandParser.h"
#include "ble/CommandHandler.h"
#include "gps/GPSProvider.h"
#include "control/DispenserChannel.h"

#include "io/IOConfig.h"
#include "io/RGBLedPins.h"
#include "io/VNH7070ASPins.h"
#include "io/ADS1115Pins.h"
#include "io/DS18B20Pins.h"
#include "io/VNH7070AS.h"
#include "io/ADS1115.h"
#include "io/DS18B20Sensor.h"

#define FIRMWARE_VERSION                "04.06.2025"
#define DEVICE_VERSION                  "29.05.2025"

class SystemContext {
public:
    static SystemContext& instance();  // Singleton accessor
    
    void init();  // Initialize all services

    // Non-Const Accessors for services
    inline SystemPreferences& getPrefs() { return prefs; }
    inline BLETextServer& getBLETextServer() { return bleTextServer; }
    inline BLECommandParser& getBLECommandParser() { return bleCommandParser; }
    inline CommandHandler& getCommandHandler() { return commandHandler; }
    inline TinyGPSPlus& getGPSModule() { return gpsModule; }
    inline GPSProvider& getGPSProvider() { return gpsProvider; }
    inline ADS1115& getADS1115() { return ads1115; }
    inline DS18B20Sensor& getTempSensor() { return tempSensor; }
    inline DispenserChannel& getLeftChannel() { return leftChannel; }
    inline DispenserChannel& getRightChannel() { return rightChannel; }
    
    // Const accessors for services
    inline const SystemPreferences& getPrefs() const { return prefs; }
    inline const BLETextServer& getBLETextServer() const { return bleTextServer; }
    inline const BLECommandParser& getBLECommandParser() const { return bleCommandParser; }
    inline const CommandHandler& getCommandHandler() const { return commandHandler; }
    inline const TinyGPSPlus& getGPSModule() const { return gpsModule; }
    inline const GPSProvider& getGPSProvider() const { return gpsProvider; }
    inline const ADS1115& getADS1115() const { return ads1115; }
    inline const DS18B20Sensor& getTempSensor() const { return tempSensor; }
    inline const DispenserChannel& getLeftChannel() const { return leftChannel; }
    inline const DispenserChannel& getRightChannel() const { return rightChannel; }

    void writeRGBLEDs(uint8_t chR, uint8_t chG, uint8_t chB);
    float getGroundSpeed(bool useSim = false) const;

    // Identity
    inline void setBoardID(const String& id) { boardID = id; }
    inline void setEspID(const String& id) { espID = id; }
    inline void setBleMAC(const String& mac) { bleMAC = mac; }

    inline const String& getBoardID() const { return boardID; }
    inline const String& getEspID() const { return espID; }
    inline const String& getBleMAC() const { return bleMAC; }

private:
    // Pin Definitions
    static constexpr VNH7070ASPins leftChannelPins = { VNH7070AS_INA1Pin, VNH7070AS_INB1Pin, VNH7070AS_PWM1Pin, VNH7070AS_SEL1Pin };
    static constexpr VNH7070ASPins rightChannelPins = { VNH7070AS_INA2Pin, VNH7070AS_INB2Pin, VNH7070AS_PWM2Pin, VNH7070AS_SEL2Pin };
    static constexpr RGBLedPins rgbLEDPins = { RGB_LEDRPin, RGB_LEDGPin, RGB_LEDBPin };
    static constexpr ADS1115Pins adsPins = { I2C_SDAPin, I2C_SCLPin };
    static constexpr DS18B20Pins tempPins = { DS18B20_DataPin };
    static constexpr uint8_t ADS1115_I2C_ADDRESS = 0x48;

    // Services
    SystemPreferences prefs;
    BLETextServer bleTextServer;
    BLECommandParser bleCommandParser;
    CommandHandler commandHandler;
    TinyGPSPlus gpsModule;
    GPSProvider gpsProvider;
    ADS1115 ads1115;
    DS18B20Sensor tempSensor;
    DispenserChannel leftChannel;
    DispenserChannel rightChannel;

    // board specific identification
    String boardID;
    String espID;
    String bleMAC;

    String readChipUUID();
    String readBLEMAC();
    String readDS18B20ID();
};
