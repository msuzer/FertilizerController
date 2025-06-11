#pragma once

#include "VNH7070ASPins.h"
#include <stdint.h>

// C-style callback types
typedef void (*DigitalWriteCallback)(uint8_t pin, bool state);
typedef void (*PwmWriteCallback)(uint8_t pin, uint8_t duty);  // 0-255

class VNH7070AS {
public:
    VNH7070AS() : _pins{-1, -1, -1, -1} {} // invalid pins initially
    void init(const VNH7070ASPins& pins, PwmWriteCallback pwmWriteFn, DigitalWriteCallback digitalWriteFn);
    void setSpeed(int8_t duty);  // -100 to +100, 0 = stop
    void stop();                 // INA/INB LOW
    void brake();                // INA/INB HIGH
    bool isStuck(void) {return _isStuck; }
    bool checkStuck(float current);
    void selectDiagnostic(bool sel0State);

private:
    VNH7070ASPins _pins;
    PwmWriteCallback _pwmWriteFn;
    DigitalWriteCallback _digitalWriteFn;
    int stuckCounter = 0;
    bool _isStuck = false;
};
