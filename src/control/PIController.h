// ============================================
// File: PIController.h
// Purpose: PI controller implementation for flow control
// Part of: Control Layer
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once

#include <stdint.h>
#include "core/SystemPreferences.h"

class DispenserChannel; // Forward declaration

constexpr int CONTROL_LOOP_UPDATE_FREQUENCY_HZ = 10; // Control loop frequency in Hz

class PIController {
    friend class DispenserChannel; // Allow DispenserChannel to access private members
public:
    PIController(const PIController&) = delete;
    PIController& operator=(const PIController&) = delete;
    PIController(PIController&&) = delete;
    PIController& operator=(PIController&&) = delete;

    const float getPIKp(void) const {return _Kp; }
    const float getPIKi(void) const {return _Ki; }
    void setPIKp(float value) { _Kp = value; }
    void setPIKi(float value) { _Ki = value; }
    float getError(void) const { return error; }
    bool isControlSignalChanged(void);
    const int getControlSignal(void) const {return controlSignal; }

    void setParams(float Kp, float Ki) {_Kp = Kp; _Ki = Ki; }
    float compute(float setpoint, float measurement);
    void reset(); // Reset integral term
private:
    PIController(float Kp = DEFAULT_KP_VALUE, float Ki = DEFAULT_KI_VALUE)
    : _Kp(Kp), _Ki(Ki), _integral(0.0f) { reset(); }

    float dt = 1.0f / CONTROL_LOOP_UPDATE_FREQUENCY_HZ;
    float _Kp, _Ki, controlSignal, error, _integral;
};
