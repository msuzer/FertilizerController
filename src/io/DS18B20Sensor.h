#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>

class DS18B20Sensor {
public:
    static DS18B20Sensor& getInstance();
    DS18B20Sensor() = default; // private constructor
    DS18B20Sensor(const DS18B20Sensor&) = delete;
    DS18B20Sensor& operator=(const DS18B20Sensor&) = delete;

    bool init(uint8_t pin);  // call in setup
    float getTemperatureC();
    String getSensorID();
    bool isReady() const;

private:
    OneWire* oneWire;
    DallasTemperature* sensors;
    DeviceAddress deviceAddress;
    bool ready;
};
