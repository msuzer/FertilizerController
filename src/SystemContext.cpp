// SystemContext.cpp (renamed from UserInterface.cpp)
#include "SystemContext.h"
#include "BLETextServer.h"
#include "TinyGPSPlus.h"
#include "CommandHandler.h"
#include "PIController.h"
#include "DS18B20Sensor.h"
#include "SystemPreferences.h"

SystemContext::SystemContext() {}

void SystemContext::begin() {
    espID = readChipUUID();
    bleMAC = readBLEMAC();
    boardID = readDS18B20ID();

    printf("Chip ID: %s | BLE MAC: %s | Board ID: %s\n", espID.c_str(), bleMAC.c_str(), boardID.c_str());

    services->prefs->load(*this);
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
