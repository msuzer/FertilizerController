// ============================================
// File: DispenserChannel.h
// Purpose: Represents and controls one fertilizer dispenser channel
// Part of: Control Layer
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once

#include <stdint.h>
#include <Arduino.h>
#include "PIController.h"
#include "io/VNH7070AS.h"
#include "io/IOConfig.h"
#include "io/ADS1115.h"
#include "core/SystemPreferences.h"

class SystemContext; // Forward declaration

constexpr float FLOW_ERROR_WARNING_THRESHOLD = 2.0f;
constexpr float MIN_POT_VOLTAGE = 0.00f; // Minimum voltage for potentiometer
constexpr float MAX_POT_VOLTAGE = 3.30f; // Maximum voltage for potentiometer

enum UserErrorCodes {
    NO_ERROR = 0,
    LIQUID_TANK_EMPTY = 1 << 0,
    INSUFFICIENT_FLOW = 1 << 1,
    FLOW_NOT_SETTLED = 1 << 2,
    MOTOR_STUCK = 1 << 3,
    DUMMY_ERROR = 1 << 4,
    BATTERY_LOW = 1 << 5,
    NO_SATELLITE_CONNECTED = 1 << 6,
    INVALID_SATELLITE_INFO = 1 << 7,
    INVALID_GPS_LOCATION = 1 << 8,
    INVALID_GPS_SPEED = 1 << 9,
    INVALID_PARAM_COUNT = 1 << 10,
    MESSAGE_PARSE_ERROR = 1 << 11,
    HARDWARE_ERROR = 1 << 12
};

enum class UserTaskState {
    Stopped,
    Started,
    Paused,
    Resuming,
    Testing
};

class DispenserChannel {
    friend class SystemContext; // Allow SystemContext to access private members
public:
    DispenserChannel(const DispenserChannel&) = delete;
    DispenserChannel& operator=(const DispenserChannel&) = delete;
    DispenserChannel(DispenserChannel&&) = delete;
    DispenserChannel& operator=(DispenserChannel&&) = delete;

    void init(String name, SystemContext* ctx, const VNH7070ASPins& motorPins);
    void printMotorCurrent(void);
    void testMotorRamp(void);

    // Setters
    bool setTaskState(UserTaskState state);
    inline void setPIKp(float Kp) { piController.setPIKp(Kp); }
    inline void setPIKi(float Ki) { piController.setPIKi(Ki); }
    inline void setPIParams(float Kp, float Ki) { piController.setPIKp(Kp); piController.setPIKi(Ki); }
    inline void setBoomWidth(float val) { boomWidth = val; }
    inline void setTargetFlowRatePerDaa(float val) { targetFlowRatePerDaa = val; }
    inline void setTargetFlowRatePerMin(float val) { targetFlowRatePerMin = val; }
    inline void setRealFlowRatePerDaa(float val) { realFlowRatePerDaa = val; }
    inline void setRealFlowRatePerMin(float val) { realFlowRatePerMin = val; }
    inline void setFlowCoeff(float val) { flowCoeff = val; }

    inline void setErrorFlags(uint32_t flags) { errorFlags = flags; }
    inline void setError(uint32_t mask) { errorFlags |= mask; }
    inline void clearError(uint32_t mask) { errorFlags &= ~mask; }
    inline void clearAllErrors() { errorFlags = 0; }

    // Getters
    inline VNH7070AS& getMotor() { return motorDriver; }
    inline const VNH7070AS& getMotor() const { return motorDriver; }
    inline PIController& getPIController() { return piController; }
    inline const PIController& getPIController() const { return piController; }
    inline float getPIKp() const { return piController.getPIKp(); }
    inline float getPIKi() const { return piController.getPIKi(); }
    inline float getTargetFlowRatePerDaa() const { return targetFlowRatePerDaa; }
    inline float getTargetFlowRatePerMin() const { return targetFlowRatePerMin; }
    inline float getRealFlowRatePerDaa() const { return realFlowRatePerDaa; }
    inline float getRealFlowRatePerMin() const { return realFlowRatePerMin; }
    inline float getFlowCoeff() const { return flowCoeff; }
    inline float getBoomWidth() const { return boomWidth; }
    inline UserTaskState getTaskState() const { return taskState; }
    inline uint32_t getErrorFlags() const { return errorFlags; }

    inline bool hasError(uint32_t mask) const { return (errorFlags & mask) != 0; }
    inline bool hasAnyError() const { return errorFlags != 0; }

    // Current task state
    inline bool isTaskStarted() const { return taskState == UserTaskState::Started; }
    inline bool isTaskPaused() const { return taskState == UserTaskState::Paused; }
    inline bool isTaskStopped() const { return taskState == UserTaskState::Stopped; }
    inline bool isTaskResuming() const { return taskState == UserTaskState::Resuming; }
    inline bool isTaskActive() const { return taskState == UserTaskState::Started || taskState == UserTaskState::Resuming; }
    const char* getTaskStateName() const;
    const char* taskStateToString(UserTaskState state) const;

    void checkLowSpeedState();
    void updateTaskMetrics();
    float getProcessedAreaPerSec() const;
    void applyPIControl();
    void applyPIControl(float target, float measured);
    void reportErrorFlags(void);

    // Task metrics per channel
    inline void incrementApplicationDuration() { applicationDuration++; }
    inline void increaseDistanceTaken(int length) { distanceTaken += length; }
    inline void increaseAreaProcessed(float value) { areaCompleted += value; }
    inline void increaseLiquidConsumed(float value) { liquidConsumed += value; }
    
    inline void clearApplicationDuration() { applicationDuration = 0; }
    inline void clearDistanceTaken() { distanceTaken = 0; }
    inline void clearAreaCompleted() { areaCompleted = 0.0f; }
    inline void clearLiquidConsumed() { liquidConsumed = 0.0f; }
    
    inline int getApplicationDuration() const { return applicationDuration; }
    inline int getDistanceTaken() const { return distanceTaken; }
    inline float getAreaCompleted() const { return areaCompleted; }
    inline float getLiquidConsumed() const { return liquidConsumed; }
    
    inline static float getTankLevel() { return tankLevel; }
    inline void decreaseTankLevel(float value) { tankLevel -= value; }
    inline static bool isClientInWorkZone() { return clientInWorkZone; }
    inline static void setTankLevel(float level) { tankLevel = level; }
    inline static void setClientInWorkZone(bool inWorkZone) { clientInWorkZone = inWorkZone; }

    float getCurrentPositionPercent() const;
    float getTargetPositionForRate(float desiredKgPerDaa) const;
private:
    DispenserChannel(String name = "") : channelName(name) { }

    String channelName;
    uint8_t channelIndex = 0;  // CH0 for left, CH1 for right
    ADS1115Channels adcChannel = ADS1115Channels::CH0; // Default to CH0
    static SystemContext* context;

    PIController piController;
    VNH7070AS motorDriver;

    float targetFlowRatePerDaa = 0.0f;
    float targetFlowRatePerMin = 0.0f;
    float realFlowRatePerDaa = 0.0f;
    float realFlowRatePerMin = 0.0f;
    float flowCoeff = 1.0f;

    int counter = 0;
    bool lowSpeedFlag = false;
    UserTaskState taskState = UserTaskState::Stopped;
    uint32_t errorFlags = NO_ERROR;

    int applicationDuration = 0;
    int distanceTaken = 0;
    float areaCompleted = 0.0f;
    float liquidConsumed = 0.0f;

    static float tankLevel; // in liters, used for area calculations
    static bool clientInWorkZone;

    float boomWidth = 0.0f; // in meters, used for area calculations

    int testTick = 0;
    bool testDirection = true;  // true = forward, false = backward
};
