#pragma once

#include <stdint.h>

#define FLOW_ERROR_WARNING_THRESHOLD    2.0f

enum UserErrorCodes {
    NO_ERROR = 0,
    LIQUID_TANK_EMPTY = 1 << 0,
    INSUFFICIENT_FLOW = 1 << 1,
    FLOW_NOT_SETTLED = 1 << 2,
    MOTOR1_STUCK = 1 << 3,
    MOTOR2_STUCK = 1 << 4,
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
    Resuming
};

class DispenserChannel {
public:
    DispenserChannel();

    // Setters
    bool setTaskState(UserTaskState state);
    
    void setTargetFlowRatePerDaa(float val) { targetFlowRatePerDaa = val; }
    void setTargetFlowRatePerMin(float val) { targetFlowRatePerMin = val; }
    void setRealFlowRatePerDaa(float val) { realFlowRatePerDaa = val; }
    void setRealFlowRatePerMin(float val) { realFlowRatePerMin = val; }
    void setFlowCoeff(float val) { flowCoeff = val; }

    void setErrorFlags(uint32_t flags) { errorFlags = flags; }
    void setError(uint32_t mask) { errorFlags |= mask; }
    void clearError(uint32_t mask) { errorFlags &= ~mask; }
    void clearAllErrors() { errorFlags = 0; }

    // Getters
    float getTargetFlowRatePerDaa() const { return targetFlowRatePerDaa; }
    float getTargetFlowRatePerMin() const { return targetFlowRatePerMin; }
    float getRealFlowRatePerDaa() const { return realFlowRatePerDaa; }
    float getRealFlowRatePerMin() const { return realFlowRatePerMin; }
    float getFlowCoeff() const { return flowCoeff; }
    UserTaskState getTaskState() const { return taskState; }
    uint32_t getErrorFlags() const { return errorFlags; }

    bool hasError(uint32_t mask) const { return (errorFlags & mask) != 0; }
    bool hasAnyError() const { return errorFlags != 0; }

    // Current task state
    inline bool isTaskStarted() const { return taskState == UserTaskState::Started; }
    inline bool isTaskPaused() const { return taskState == UserTaskState::Paused; }
    inline bool isTaskStopped() const { return taskState == UserTaskState::Stopped; }
    inline bool isTaskResuming() const { return taskState == UserTaskState::Resuming; }
    inline bool isTaskActive() const { return taskState == UserTaskState::Started || taskState == UserTaskState::Resuming; }
    const char* getTaskStateName() const;
    const char* taskStateToString(UserTaskState state) const;

private:
    float targetFlowRatePerDaa = 0.0f;
    float targetFlowRatePerMin = 0.0f;
    float realFlowRatePerDaa = 0.0f;
    float realFlowRatePerMin = 0.0f;
    float flowCoeff = 1.0f;

    UserTaskState taskState = UserTaskState::Stopped;
    uint32_t errorFlags = NO_ERROR;
};
