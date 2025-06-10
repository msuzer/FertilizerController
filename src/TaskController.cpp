#include "TaskController.h"

float TaskController::getGroundSpeed(bool useSim = false) const {
    if (speedSource == "GPS") {
        return context->getServices()->gpsProvider->getSpeed();
    }

    return simSpeed;
}