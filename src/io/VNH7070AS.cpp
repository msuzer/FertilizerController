// ============================================
// File: VNH7070AS.h
// Purpose: Driver for VNH7070AS motor driver
// Part of: Hardware Abstraction Layer (HAL)
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include "VNH7070AS.h"
#include <stdlib.h>
#include <Arduino.h>
#include <driver/ledc.h>

#define STUCK_CURRENT_THRESHOLD     2.5f // Amps (example)
#define STUCK_DETECTION_COUNT       5        // Number of consecutive samples

void VNH7070AS::init(const VNH7070ASPins& pins, const int channel) {
    _pins = pins;
    _pwmChannel = (channel == 0) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_1;

    // Initialize pins
    pinMode(_pins.INA, OUTPUT);
    pinMode(_pins.INB, OUTPUT);
    pinMode(_pins.SEL, OUTPUT);
}

void VNH7070AS::setSpeed(int8_t duty) {
    duty = constrain(duty, -MAX_DUTY, MAX_DUTY);

    // Set direction
    if (duty > 0) {
        digitalWrite(_pins.INA, HIGH);
        digitalWrite(_pins.INB, LOW);
        selectDiagnostic(true);
    } else if (duty < 0) {
        digitalWrite(_pins.INA, LOW);
        digitalWrite(_pins.INB, HIGH);
        selectDiagnostic(false);
    } else {
        digitalWrite(_pins.INA, LOW);
        digitalWrite(_pins.INB, LOW);
    }

    // Convert duty -100..100 to 0..255
    uint8_t pwmValue = (duty == 0) ? 0 : static_cast<uint8_t>(abs(duty) * (255.0f / MAX_DUTY));

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t) _pwmChannel, pwmValue);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t) _pwmChannel);

    // ledcWrite(_pins.PWM, pwmValue);
}

void VNH7070AS::stop() {
    digitalWrite(_pins.INA, LOW);
    digitalWrite(_pins.INB, LOW);
    ledcWrite(_pins.PWM, 0);
}

void VNH7070AS::brake() {
    digitalWrite(_pins.INA, HIGH);
    digitalWrite(_pins.INB, HIGH);
    ledcWrite(_pins.PWM, 0);
}

bool VNH7070AS::checkStuck(float current) {
    if (current >= STUCK_CURRENT_THRESHOLD) {
        if (++stuckCounter >= STUCK_DETECTION_COUNT) {
            return _isStuck = true;
        }
    } else {
        stuckCounter = 0;
    }
    return _isStuck = false;
}

void VNH7070AS::selectDiagnostic(bool sel0State) {
    digitalWrite(_pins.SEL, sel0State ? HIGH : LOW);
}
