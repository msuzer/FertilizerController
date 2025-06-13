// ============================================
// File: DS18B20Sensor.h
// Purpose: Driver for DS18B20 temperature sensor
// Part of: Hardware Abstraction Layer (HAL)
// Dependencies: DallasTemperature, OneWire
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>

class SystemContext; // Forward declaration

class DS18B20Sensor {
    friend class SystemContext; // Allow SystemContext to access private members
public:
    DS18B20Sensor(const DS18B20Sensor&) = delete;
    DS18B20Sensor& operator=(const DS18B20Sensor&) = delete;
    DS18B20Sensor(DS18B20Sensor&&) = delete;
    DS18B20Sensor& operator=(DS18B20Sensor&&) = delete;

    bool init(uint8_t pin);  // call in setup
    float getTemperatureC();
    String getSensorID();
    bool isReady() const;

private:
    DS18B20Sensor() = default; // private constructor

    OneWire* oneWire;
    DallasTemperature* sensors;
    DeviceAddress deviceAddress;
    bool ready;
};
