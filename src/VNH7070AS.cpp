#include "VNH7070AS.h"
#include <stdlib.h>

VNH7070AS::VNH7070AS(
    uint8_t inaPin,
    uint8_t inbPin,
    uint8_t pwmPin,
    uint8_t sel0Pin,
    PwmWriteCallback pwmWriteFn,
    DigitalWriteCallback digitalWriteFn
) :
    _inaPin(inaPin),
    _inbPin(inbPin),
    _pwmPin(pwmPin),
    _sel0Pin(sel0Pin),
    _pwmWriteFn(pwmWriteFn),
    _digitalWriteFn(digitalWriteFn)
{
    // User should handle pinMode setup elsewhere
}

void VNH7070AS::setSpeed(int8_t duty) {
    // Clamp duty
    if (duty > 100) duty = 100;
    if (duty < -100) duty = -100;

    // Set direction
    if (duty > 0) {
        _digitalWriteFn(_inaPin, true);
        _digitalWriteFn(_inbPin, false);
        selectDiagnostic(true);
    } else if (duty < 0) {
        _digitalWriteFn(_inaPin, false);
        _digitalWriteFn(_inbPin, true);
        selectDiagnostic(false);
    } else {
        _digitalWriteFn(_inaPin, false);
        _digitalWriteFn(_inbPin, false);
    }

    // Convert duty -100..100 to 0..255
    uint8_t pwmValue = (duty == 0) ? 0 : (uint8_t)(abs(duty) * 2.55f);
    _pwmWriteFn(_pwmPin, pwmValue);
}

void VNH7070AS::stop() {
    _digitalWriteFn(_inaPin, false);
    _digitalWriteFn(_inbPin, false);
    _pwmWriteFn(_pwmPin, 0);
}

void VNH7070AS::brake() {
    _digitalWriteFn(_inaPin, true);
    _digitalWriteFn(_inbPin, true);
    _pwmWriteFn(_pwmPin, 0);
}

void VNH7070AS::selectDiagnostic(bool sel0State) {
    _digitalWriteFn(_sel0Pin, sel0State);
}
