#pragma once

#include <stdint.h>

class PIController {
public:
    PIController(float Kp, float Ki, float outputMin, float outputMax);
    float getPIKp(void) {return _Kp; }
    float getPIKi(void) {return _Ki; }
    void setPIKp(float value) { _Kp = value; }
    void setPIKi(float value) { _Ki = value; }
    float getError(void) const { return error; }
    bool isControlSignalChanged(void);
    int getControlSignal(void) {return controlSignal; }

    void setParams(float Kp, float Ki) {_Kp = Kp; _Ki = Ki; }
    float compute(float setpoint, float measurement, float dt); // dt in seconds
    void reset(); // Reset integral term
private:
    float _Kp, _Ki;
    float controlSignal, _outputMin, _outputMax;
    float error, _integral;
};
