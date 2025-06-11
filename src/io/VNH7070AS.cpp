#include "VNH7070AS.h"
#include <stdlib.h>
#include <Arduino.h>

#define STUCK_CURRENT_THRESHOLD     2.5f // Amps (example)
#define STUCK_DETECTION_COUNT       5        // Number of consecutive samples

void VNH7070AS::init(const VNH7070ASPins& pins, PwmWriteCallback pwmWriteFn, DigitalWriteCallback digitalWriteFn) {
    _pins = pins;
    _pwmWriteFn = pwmWriteFn;
    _digitalWriteFn = digitalWriteFn;
    
    // Initialize pins
    pinMode(_pins.INA, OUTPUT);
    pinMode(_pins.INB, OUTPUT);
    pinMode(_pins.PWM, OUTPUT);
    pinMode(_pins.SEL, OUTPUT);
}

void VNH7070AS::setSpeed(int8_t duty) {
    // Clamp duty
    if (duty > 100) duty = 100;
    if (duty < -100) duty = -100;

    // Set direction
    if (duty > 0) {
        _digitalWriteFn(_pins.INA, true);
        _digitalWriteFn(_pins.INB, false);
        selectDiagnostic(true);
    } else if (duty < 0) {
        _digitalWriteFn(_pins.INA, false);
        _digitalWriteFn(_pins.INB, true);
        selectDiagnostic(false);
    } else {
        _digitalWriteFn(_pins.INA, false);
        _digitalWriteFn(_pins.INB, false);
    }

    // Convert duty -100..100 to 0..255
    uint8_t pwmValue = (duty == 0) ? 0 : (uint8_t)(abs(duty) * 2.55f);
    _pwmWriteFn(_pins.PWM, pwmValue);
}

void VNH7070AS::stop() {
    _digitalWriteFn(_pins.INA, false);
    _digitalWriteFn(_pins.INB, false);
    _pwmWriteFn(_pins.PWM, 0);
}

void VNH7070AS::brake() {
    _digitalWriteFn(_pins.INA, true);
    _digitalWriteFn(_pins.INB, true);
    _pwmWriteFn(_pins.PWM, 0);
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
    _digitalWriteFn(_pins.SEL, sel0State);
}
