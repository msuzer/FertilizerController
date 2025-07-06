// ============================================
// File: VNH7070AS.h
// Purpose: Driver for VNH7070AS motor driver
// Part of: Hardware Abstraction Layer (HAL)
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once

#include "VNH7070ASPins.h"
#include <stdint.h>
#include <Arduino.h>
#include <driver/ledc.h>

class DispenserChannel; // Forward declaration

class VNH7070AS {
    friend class DispenserChannel; // Allow DispenserChannel to access private members
public:
    static constexpr int MAX_DUTY = 100;

    VNH7070AS(const VNH7070AS&) = delete;
    VNH7070AS& operator=(const VNH7070AS&) = delete;
    VNH7070AS(VNH7070AS&&) = delete;
    VNH7070AS& operator=(VNH7070AS&&) = delete;

    void init(const VNH7070ASPins& pins, const int channel = LEDC_CHANNEL_0); // Default to channel 0
    void setSpeed(int8_t duty);  // -100 to +100, 0 = stop
    void stop();                 // INA/INB LOW
    void brake();                // INA/INB HIGH
    bool isStuck(void) {return _isStuck; }
    bool checkStuck(float current);
    void selectDiagnostic(bool sel0State);

private:
    VNH7070AS() : _pins{-1, -1, -1, -1} {} // invalid pins initially

    VNH7070ASPins _pins;
    int stuckCounter = 0;
    bool _isStuck = false;
    ledc_channel_t _pwmChannel = LEDC_CHANNEL_0; // Default to channel 0
};
