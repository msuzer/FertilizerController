#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>

class DS18B20Sensor {
public:
    static DS18B20Sensor& getInstance();

    bool begin(uint8_t pin);  // call in setup
    float getTemperatureC();
    String getSensorID();
    bool isReady() const;

private:
    DS18B20Sensor(); // private constructor
    DS18B20Sensor(const DS18B20Sensor&) = delete;
    DS18B20Sensor& operator=(const DS18B20Sensor&) = delete;

    OneWire* oneWire;
    DallasTemperature* sensors;
    DeviceAddress deviceAddress;
    bool ready;
};
