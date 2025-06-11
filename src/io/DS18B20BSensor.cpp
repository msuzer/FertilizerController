#include "DS18B20Sensor.h"

DS18B20Sensor& DS18B20Sensor::getInstance() {
    static DS18B20Sensor instance;
    return instance;
}

DS18B20Sensor::DS18B20Sensor() : oneWire(nullptr), sensors(nullptr), ready(false) {}

bool DS18B20Sensor::begin(uint8_t pin) {
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
