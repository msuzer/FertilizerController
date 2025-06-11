#include "SystemContext.h"
#include <TinyGPSPlus.h>

// BLE Callback Implementations
static void onWriteCallback(const char* message, size_t len);
static const char* onReadCallback();
static void onConnectCallback();
static void onDisconnectCallback();

constexpr ADS1115Pins SystemContext::adsPins;
constexpr VNH7070ASPins SystemContext::leftChannelPins;
constexpr VNH7070ASPins SystemContext::rightChannelPins;

SystemContext& SystemContext::instance() {
    static SystemContext ctx;
    return ctx;
}

void SystemContext::init() {
    espID = readChipUUID();
    bleMAC = readBLEMAC();
    boardID = readDS18B20ID();

    printf("Chip ID: %s | BLE MAC: %s | Board ID: %s\n", espID.c_str(), bleMAC.c_str(), boardID.c_str());

    ads1115.init(ADS1115_I2C_ADDRESS, adsPins);
    ads1115.setGain(ADS1115::Gain::FSR_4_096V); // Optional: Set gain

    prefs.init(*this);
    leftChannel.init(this, leftChannelPins);
    rightChannel.init(this, rightChannelPins);

    commandHandler.setContext(this);
    commandHandler.registerHandlers();

    gpsProvider.setModule(&gpsModule);

    bleTextServer.onWrite(onWriteCallback);
    bleTextServer.onRead(onReadCallback);
    bleTextServer.onConnect(onConnectCallback);
    bleTextServer.onDisconnect(onDisconnectCallback);

    // Initialize DS18B20 sensor
    tempSensor.init(tempPins.DQ);
}

String SystemContext::readChipUUID() {
    uint64_t chipID = ESP.getEfuseMac();
    return String((uint32_t)(chipID >> 32), HEX) + String((uint32_t)chipID, HEX);
}

String SystemContext::readBLEMAC() {
    uint8_t mac[6];
    char macStr[18];
    esp_read_mac(mac, ESP_MAC_BT);
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

String SystemContext::readDS18B20ID() {
    auto& sensor = DS18B20Sensor::getInstance();
    return sensor.isReady() ? sensor.getSensorID() : "DS18B20 Not Found";
}

float SystemContext::getGroundSpeed(bool useSim) const {
  const SystemParams& params = prefs.getParams();
    if (params.speedSource == "GPS") {
        return gpsProvider.getSpeed();
    }

    return params.simSpeed;
}

void SystemContext::writeRGBLEDs(uint8_t chR, uint8_t chG, uint8_t chB) {
  digitalWrite(RGB_LEDRPin, chR);
  digitalWrite(RGB_LEDGPin, chG);
  digitalWrite(RGB_LEDBPin, chB);
}

// BLE Callback Implementations

static void onWriteCallback(const char* message, size_t len) {
    if (message != nullptr) {
      Serial.printf("Received: %.*s\n", len, message);
      SystemContext::instance().getBLECommandParser().dispatchInstruction(message);
    }
}

static const char* onReadCallback() {
    return "ESP32 says hi!";
}

static void onConnectCallback() {
  // writeRGBLEDs(LOW, LOW, HIGH);
  Serial.println("Client connected!");
}

static void onDisconnectCallback() {
  // writeRGBLEDs(LOW, HIGH, LOW);
  Serial.println("Client disconnected!");
}
