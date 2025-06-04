#pragma once

#include <stdint.h>

// C-style callback types
typedef void (*DigitalWriteCallback)(uint8_t pin, bool state);
typedef void (*PwmWriteCallback)(uint8_t pin, uint8_t duty);  // 0-255

class VNH7070AS {
public:
    VNH7070AS(
        uint8_t inaPin,
        uint8_t inbPin,
        uint8_t pwmPin,
        uint8_t sel0Pin,
        PwmWriteCallback pwmWriteFn,
        DigitalWriteCallback digitalWriteFn
    );

    void setupPins(void);
    void setSpeed(int8_t duty);  // -100 to +100, 0 = stop
    void stop();                 // INA/INB LOW
    void brake();                // INA/INB HIGH
    bool isStuck(void) {return _isStuck; }
    bool checkStuck(float current);
    void selectDiagnostic(bool sel0State);

private:
    uint8_t _inaPin, _inbPin, _pwmPin, _sel0Pin;
    PwmWriteCallback _pwmWriteFn;
    DigitalWriteCallback _digitalWriteFn;
    int stuckCounter = 0;
    bool _isStuck = false;
};
