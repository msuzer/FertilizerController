// ============================================
// File: DS18B20Sensor.cpp
// Purpose: Driver for DS18B20 temperature sensor
// Part of: Hardware Abstraction Layer (HAL)
// Dependencies: DallasTemperature, OneWire
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include "DS18B20Sensor.h"

bool DS18B20Sensor::init(uint8_t pin) {
    oneWire = new OneWire(pin);
    sensors = new DallasTemperature(oneWire);
    sensors->begin();

    if (sensors->getDeviceCount() < 1 || !sensors->getAddress(deviceAddress, 0)) {
        ready = false;
        return false;
    }

    sensors->setResolution(deviceAddress, 12);
    ready = true;
    return true;
}

float DS18B20Sensor::getTemperatureC() {
    if (!ready) return NAN;
    sensors->requestTemperatures();
    return sensors->getTempC(deviceAddress);
}

String DS18B20Sensor::getSensorID() {
    if (!ready) return "";
    char id[17];
    for (uint8_t i = 0; i < 8; i++) {
        sprintf(id + i * 2, "%02X", deviceAddress[i]);
    }
    return String(id);
}

bool DS18B20Sensor::isReady() const {
    return ready;
}
