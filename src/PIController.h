#pragma once

#include <stdint.h>

class PIController {
public:
    PIController(float Kp, float Ki, float outputMin, float outputMax);

    float compute(float setpoint, float measurement, float dt); // dt in seconds

    void reset(); // Reset integral term

private:
    float _Kp, _Ki;
    float _outputMin, _outputMax;
    float _integral;
};
