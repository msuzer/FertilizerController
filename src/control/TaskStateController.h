#pragma once

#include <cstdint>
#include "control/ErrorManager.h"
#include "control/ApplicationMetrics.h"

enum class UserTaskState {
    Stopped = 0,
    Started,
    Paused,
    Resuming,
    Testing
};

class TaskStateController {
private:
    UserTaskState taskState = UserTaskState::Stopped;
    ErrorManager errorManager;
    ApplicationMetrics metrics;

public:
    // Query helpers
    bool isTaskStarted() const { return taskState == UserTaskState::Started; }
    bool isTaskPaused() const { return taskState == UserTaskState::Paused; }
    bool isTaskStopped() const { return taskState == UserTaskState::Stopped; }
    bool isTaskResuming() const { return taskState == UserTaskState::Resuming; }
    bool isTaskActive() const {
        return taskState == UserTaskState::Started || taskState == UserTaskState::Resuming;
    }
    bool isTaskPassive() const {
        return taskState == UserTaskState::Stopped || taskState == UserTaskState::Paused;
    }

    // Getter
    UserTaskState getTaskState() const { return taskState; }

    // Transition validation + mutation
    bool setTaskState(UserTaskState newState);

    // State names
    const char* getTaskStateName() const { return taskStateToString(taskState); }
    static const char* taskStateToString(UserTaskState state);

    ErrorManager& getErrorManager() { return errorManager; }
    const ErrorManager& getErrorManager() const { return errorManager; }
    ApplicationMetrics& getMetrics() { return metrics; }
    const ApplicationMetrics& getMetrics() const { return metrics; }

};
