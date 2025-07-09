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

    TaskStateController& getTaskController() { return taskStateController; }
    const TaskStateController& getTaskController() const { return taskStateController; }
    VNH7070AS& getMotor() { return motorDriver; }
    const VNH7070AS& getMotor() const { return motorDriver; }
    PIController& getPIController() { return piController; }
    const PIController& getPIController() const { return piController; }

    void init(String name, SystemContext* ctx, const VNH7070ASPins& motorPins);

    // Setters
    inline void setTargetFlowRatePerDaa(float val) { targetFlowRatePerDaa = val; }
    inline void setTargetFlowRatePerMin(float val) { targetFlowRatePerMin = val; }
    inline void setRealFlowRatePerDaa(float val) { realFlowRatePerDaa = val; }
    inline void setRealFlowRatePerMin(float val) { realFlowRatePerMin = val; }
    inline void setFlowCoeff(float val) { flowCoeff = val; }
    inline void setBoomWidth(float val) { boomWidth = val; }

    // Getters
    inline float getTargetFlowRatePerDaa() const { return targetFlowRatePerDaa; }
    inline float getTargetFlowRatePerMin() const { return targetFlowRatePerMin; }
    inline float getRealFlowRatePerDaa() const { return realFlowRatePerDaa; }
    inline float getRealFlowRatePerMin() const { return realFlowRatePerMin; }
    inline float getFlowCoeff() const { return flowCoeff; }
    inline float getBoomWidth() const { return boomWidth; }

    // Helper methods
    void checkLowSpeedState();
    void updateApplicationMetrics();
    float getProcessedAreaPerSec() const;
    float getCurrentPositionPercent() const;
    float getCurrentPositionPercent(ADS1115Channels adcChannel) const;
    float getTargetPositionForRate(float desiredKgPerDaa) const;
    void reportErrorFlags(void);
    void applyPIControl();
    void applyPIControl(float target, float measured);
    void printMotorCurrent(void);
    void testMotorRamp(void);

    static bool isClientInWorkZone() { return clientInWorkZone; }
    static void setClientInWorkZone(bool inWorkZone) { clientInWorkZone = inWorkZone; }
private:
    DispenserChannel(String name = "") : channelName(name) { }
    static SystemContext* context;

    PIController piController;
    VNH7070AS motorDriver;
    TaskStateController taskStateController;

    String channelName;
    uint8_t channelIndex = 0;  // CH0 for left, CH1 for right
    ADS1115Channels _adcChannel = ADS1115Channels::CH0; // Default to CH0

    float targetFlowRatePerDaa = 0.0f;
    float targetFlowRatePerMin = 0.0f;
    float realFlowRatePerDaa = 0.0f;
    float realFlowRatePerMin = 0.0f;
    float flowCoeff = 1.0f;
    float boomWidth = 0.0f; // in meters, used for area calculations

    int counter = 0;
    bool lowSpeedFlag = false;
    int testTick = 0;
    bool testDirection = true;  // true = forward, false = backward
    static bool clientInWorkZone;
};
