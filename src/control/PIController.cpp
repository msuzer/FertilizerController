// ============================================
// File: PIController.cpp
// Purpose: PI controller implementation for flow control
// Part of: Control Layer
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include "PIController.h"

float PIController::compute(float setpoint, float measurement) {
    setpoint = constrain(setpoint, 0.0f, 100.0f);
    measurement = constrain(measurement, 0.0f, 100.0f);
    
    error = setpoint - measurement;

    // Update integral
    _integral += error * dt;

    // Anti-windup: clamp integral to output limits / Ki
    if (_Ki != 0.0f) {
        float value = _integral * _Ki;
        if (value > _outputMax) {
            _integral = _outputMax / _Ki;
        } else if (value < _outputMin) {
            _integral = _outputMin / _Ki;
        }
    }

    controlSignal = _Kp * error + _Ki * _integral;
    controlSignal = constrain(controlSignal, _outputMin, _outputMax);

    return controlSignal;
}

void PIController::reset() {
    _integral = 0.0f;
    error = 0.0f;
    controlSignal = 0.0f;
}

bool PIController::isControlSignalChanged(void) {
    static int oldSignal = 0;
  
    if (oldSignal != controlSignal) {
      oldSignal = controlSignal;
      return true;
    }
  
    return false;
  }
  