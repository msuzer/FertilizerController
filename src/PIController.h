#pragma once

#include <stdint.h>

#define DEFAULT_KP_VALUE        25.0f
#define DEFAULT_KI_VALUE        4.0f

class PIController {
public:
    PIController(float Kp, float Ki, float outputMin, float outputMax);
    void begin();
    float getPIKp(void) {return _Kp; }
    float getPIKi(void) {return _Ki; }
    void setPIKp(float value) { _Kp = value; }
    void setPIKi(float value) { _Ki = value; }
    void setParams(float Kp, float Ki) {_Kp = Kp; _Ki = Ki; }
    float compute(float setpoint, float measurement, float dt); // dt in seconds
    void reset(); // Reset integral term
private:
    float _Kp, _Ki;
    float _outputMin, _outputMax;
    float _integral;
};
