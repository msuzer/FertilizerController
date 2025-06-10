#pragma once
#include "SystemContext.h"
#include "DispenserChannel.h"

class TaskController {
public:
    TaskController::TaskController(SystemContext* ctx) 
    : context(ctx), leftChannel("Left", ctx), rightChannel("Right", ctx) {}

    inline DispenserChannel& getLeftChannel() { return leftChannel; }
    inline DispenserChannel& getRightChannel() { return rightChannel; }

    inline const DispenserChannel& getLeftChannel() const { return leftChannel; }
    inline const DispenserChannel& getRightChannel() const { return rightChannel; }

    void incrementTaskDuration() { taskDuration++; }
    void increaseDistanceTaken(int length) { distanceTaken += length; }
    void increaseAreaProcessed(float value) { areaCompleted += value; }
    void increaseLiquidConsumed(float value) { liquidConsumed += value; }
    
    void clearTaskDuration() { taskDuration = 0; }
    void clearDistanceTaken() { distanceTaken = 0; }
    void clearAreaCompleted() { areaCompleted = 0.0f; }
    void clearLiquidConsumed() { liquidConsumed = 0.0f; }
    
    int getTaskDuration() const { return taskDuration; }
    int getDistanceTaken() const { return distanceTaken; }
    float getAreaCompleted() const { return areaCompleted; }
    float getLiquidConsumed() const { return liquidConsumed; }
    
    bool isClientInWorkZone() const { return clientInWorkZone; }
    void setClientInWorkZone(bool val) { clientInWorkZone = val; }

    inline float getMinWorkingSpeed() const { return minWorkingSpeed; }
    inline void setMinWorkingSpeed(float val) { minWorkingSpeed = val; }
    inline void setTankLevel(float val) { tankLevel = val; }
    inline void setBoomWidth(float val) { boomWidth = val; }
    inline float getTankLevel() const { return tankLevel; }
    inline float getBoomWidth() const { return boomWidth; }
    inline void decreaseTankLevel(float value) { tankLevel -= value; }

    inline void setSimSpeed(float val) { simSpeed = val; }
    inline void setSpeedSource(const String& val) { speedSource = val; }
    float getGroundSpeed(bool useSim = false) const;
    bool isSpeedOK() const { return (getGroundSpeed() >= minWorkingSpeed); }
    inline const String& getSpeedSource() const { return speedSource; }
    inline float getSimSpeed() const { return simSpeed; }
    float getProcessedAreaPerSec() { return getGroundSpeed() * getBoomWidth(); }

private:
    SystemContext* context;
    DispenserChannel leftChannel;
    DispenserChannel rightChannel;

    int taskDuration = 0;
    int distanceTaken = 0;
    float areaCompleted = 0.0f;
    float liquidConsumed = 0.0f;
    bool clientInWorkZone = false;

    String speedSource;
    float simSpeed;
    float minWorkingSpeed = 0.0f;
    float tankLevel = 0.0f; // in liters, used for area calculations
    float boomWidth = 0.0f; // in meters, used for area calculations
};
