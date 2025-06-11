#include "PIController.h"

PIController::PIController(float Kp, float Ki, float outputMin, float outputMax)
    : _Kp(Kp), _Ki(Ki), _outputMin(outputMin), _outputMax(outputMax), _integral(0.0f)
{
}

float PIController::compute(float setpoint, float measurement, float dt) {
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

    // Clamp output
    if (controlSignal > _outputMax) controlSignal = _outputMax;
    if (controlSignal < _outputMin) controlSignal = _outputMin;

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
  