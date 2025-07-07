#include "TaskStateController.h"
#include "core/LogUtils.h"  // If logging is used
#include "core/SystemPreferences.h"

bool TaskStateController::setTaskState(UserTaskState newState) {
    bool validOp = false;

    switch (taskState) {
        case UserTaskState::Stopped:
            validOp = (newState == UserTaskState::Started || newState == UserTaskState::Testing);
            break;
        case UserTaskState::Started:
            validOp = (newState == UserTaskState::Paused || newState == UserTaskState::Stopped);
            break;
        case UserTaskState::Paused:
            validOp = (newState == UserTaskState::Resuming || newState == UserTaskState::Stopped);
            break;
        case UserTaskState::Resuming:
            validOp = (newState == UserTaskState::Started || newState == UserTaskState::Paused || newState == UserTaskState::Stopped);
            break;
        case UserTaskState::Testing:
            validOp = (newState == UserTaskState::Stopped);
            break;
    }

    if (validOp) {
        if (taskState == UserTaskState::Stopped && newState == UserTaskState::Started) {
            errorManager.clearAllErrors();
            metrics.reset();
            float ftemp = SystemPreferences::getFloat(PrefKey::KEY_TANK_LEVEL, DEFAULT_TANK_INITIAL_LEVEL);
            ApplicationMetrics::setTankLevel(ftemp);
        }

        if (taskState == UserTaskState::Stopped || taskState == UserTaskState::Paused) {
            const uint32_t ERRORS_TO_CLEAR =
                INSUFFICIENT_FLOW |
                FLOW_NOT_SETTLED |
                NO_SATELLITE_CONNECTED |
                INVALID_SATELLITE_INFO |
                INVALID_GPS_LOCATION |
                INVALID_GPS_SPEED |
                INVALID_PARAM_COUNT |
                MESSAGE_PARSE_ERROR |
                HARDWARE_ERROR;

            errorManager.clearError(ERRORS_TO_CLEAR);

            LogUtils::info("[STATE] Cleared relevant error flags due to task state transition to %s\n", getTaskStateName());
            LogUtils::warn("[MOTOR] Aligning Motor To End\n");
        }
        taskState = newState;

        return true;
    }

    LogUtils::warn("[STATE] Invalid state transition: %s → %s\n", getTaskStateName(), taskStateToString(newState));
    return false;
}

const char* TaskStateController::taskStateToString(UserTaskState state) {
    switch (state) {
        case UserTaskState::Stopped:  return "Stopped";
        case UserTaskState::Started:  return "Started";
        case UserTaskState::Paused:   return "Paused";
        case UserTaskState::Resuming: return "Resuming";
        case UserTaskState::Testing:  return "Testing";
        default:                      return "Unknown";
    }
}
