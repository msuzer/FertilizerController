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
#include "control/ApplicationMetrics.h"
#include "control/TaskStateController.h"

class SystemContext; // Forward declaration

constexpr float MIN_POT_VOLTAGE = 0.00f; // Minimum voltage for potentiometer
constexpr float MAX_POT_VOLTAGE = 3.30f; // Maximum voltage for potentiometer

class DispenserChannel {
    friend class SystemContext; // Allow SystemContext to access private members
public:
    DispenserChannel(const DispenserChannel&) = delete;
    DispenserChannel& operator=(const DispenserChannel&) = delete;
    DispenserChannel(DispenserChannel&&) = delete;
    DispenserChannel& operator=(DispenserChannel&&) = delete;

    ApplicationMetrics& getMetrics() { return metrics; }
    const ApplicationMetrics& getMetrics() const { return metrics; }
    TaskStateController& getTaskController() { return taskStateController; }
    const TaskStateController& getTaskController() const { return taskStateController; }
    ErrorManager& getErrorManager() { return errorManager; }
    const ErrorManager& getErrorManager() const { return errorManager; }

    void init(String name, SystemContext* ctx, const VNH7070ASPins& motorPins);

    // Setters
    bool setTaskState(UserTaskState state);
    inline void setBoomWidth(float val) { boomWidth = val; }
    inline void setTargetFlowRatePerDaa(float val) { targetFlowRatePerDaa = val; }
    inline void setTargetFlowRatePerMin(float val) { targetFlowRatePerMin = val; }
    inline void setRealFlowRatePerDaa(float val) { realFlowRatePerDaa = val; }
    inline void setRealFlowRatePerMin(float val) { realFlowRatePerMin = val; }
    inline void setFlowCoeff(float val) { flowCoeff = val; }

    // Getters
    inline VNH7070AS& getMotor() { return motorDriver; }
    inline const VNH7070AS& getMotor() const { return motorDriver; }
    inline PIController& getPIController() { return piController; }
    inline const PIController& getPIController() const { return piController; }
    inline float getTargetFlowRatePerDaa() const { return targetFlowRatePerDaa; }
    inline float getTargetFlowRatePerMin() const { return targetFlowRatePerMin; }
    inline float getRealFlowRatePerDaa() const { return realFlowRatePerDaa; }
    inline float getRealFlowRatePerMin() const { return realFlowRatePerMin; }
    inline float getFlowCoeff() const { return flowCoeff; }
    inline float getBoomWidth() const { return boomWidth; }

    void checkLowSpeedState();
    void updateApplicationMetrics();
    float getProcessedAreaPerSec() const;
    void applyPIControl();
    void applyPIControl(float target, float measured);
    void reportErrorFlags(void);

    void printMotorCurrent(void);
    void testMotorRamp(void);

    inline static bool isClientInWorkZone() { return clientInWorkZone; }
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
    ApplicationMetrics metrics;
    ErrorManager errorManager;
    TaskStateController taskStateController;

    float targetFlowRatePerDaa = 0.0f;
    float targetFlowRatePerMin = 0.0f;
    float realFlowRatePerDaa = 0.0f;
    float realFlowRatePerMin = 0.0f;
    float flowCoeff = 1.0f;

    int counter = 0;
    bool lowSpeedFlag = false;

    static bool clientInWorkZone;

    float boomWidth = 0.0f; // in meters, used for area calculations

    int testTick = 0;
    bool testDirection = true;  // true = forward, false = backward
};
