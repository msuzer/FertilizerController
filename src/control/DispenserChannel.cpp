#include "DispenserChannel.h"
#include "gps/GPSProvider.h"
#include "core/SystemContext.h"

AppServices* DispenserChannel::services = nullptr;

bool DispenserChannel::setTaskState(UserTaskState state) {
    bool validOp = false;

    switch (taskState) {
        case UserTaskState::Stopped:
            validOp = (state == UserTaskState::Started);
            break;
        case UserTaskState::Started:
            validOp = (state == UserTaskState::Paused || state == UserTaskState::Stopped);
            break;
        case UserTaskState::Paused:
            validOp = (state == UserTaskState::Resuming || state == UserTaskState::Stopped);
            break;
        case UserTaskState::Resuming:
            validOp = (state == UserTaskState::Started || state == UserTaskState::Paused || state == UserTaskState::Stopped);
            break;
    }

    if (validOp) {
        taskState = state;
        return true;
    }

    printf("[UI] Invalid state transition: %s → %s\n", getTaskStateName(), taskStateToString(state));
    return false;
}

const char* DispenserChannel::taskStateToString(UserTaskState state) const {
    switch (state) {
        case UserTaskState::Stopped:  return "Stopped";
        case UserTaskState::Started:  return "Started";
        case UserTaskState::Paused:   return "Paused";
        case UserTaskState::Resuming: return "Resuming";
        default:                      return "Unknown";
    }
}

const char* DispenserChannel::getTaskStateName() const {
    return taskStateToString(taskState);
}

void DispenserChannel::checkLowSpeedState() {
  SystemContext* context = services->systemContext;
    if (getTargetFlowRatePerDaa() > 0.0f) {
        if (context->getGroundSpeed() < context->getMinWorkingSpeed()) {
            if (context->getMinWorkingSpeed() > 0) {
                if (isTaskActive()) {
                    lowSpeedFlag = true;
                    setTaskState(UserTaskState::Paused);
                    printf("%s Channel Task Paused due to Low Speed\n", channelName);
                }
            }
        } else {
            if (lowSpeedFlag) {
                if (isTaskPaused()) {
                    printf("Resuming %s Channel Task\n", channelName);
                    lowSpeedFlag = false;
                    // Optional: channel.setTaskState(UserTaskState::Resuming);
                    setTaskState(UserTaskState::Resuming);
                }
            }
        }
    }
}

void DispenserChannel::updateChannel(PIController& pic) {
  SystemContext* context = services->systemContext;
    float flowRatePerMin = getRealFlowRatePerMin();
    bool isBoomWidthOK = (getBoomWidth() > 0);
    bool isSpeedOK = context->isSpeedOK();
    bool isFlowOK = (flowRatePerMin > 0);
    const float deltaTime = 1.0f;

    checkLowSpeedState();

    if (isSpeedOK && isBoomWidthOK) {
      if (isFlowOK) {
        // Update shared metrics only once per update (safe here — same slice for both channels)
        float groundSpeed = context->getGroundSpeed(true);
        float processedAreaPerSec = getProcessedAreaPerSec();

        increaseDistanceTaken(groundSpeed * deltaTime);
        increaseAreaProcessed(processedAreaPerSec);
        incrementTaskDuration();

        float slice = flowRatePerMin / 60.0f;
        context->decreaseTankLevel(slice);
        increaseLiquidConsumed(slice);

        printf("Ground Speed, Boom Width and Min Flow OK for one channel!\n");

        clearError(INSUFFICIENT_FLOW);

        if (abs(pic.getError()) >= FLOW_ERROR_WARNING_THRESHOLD) {
          if (++counter >= context->getHeartBeatPeriod()) {
            setError(FLOW_NOT_SETTLED);
            counter = 0;
          }
        } else {
          clearError(FLOW_NOT_SETTLED);
          counter = 0;
        }
      } else {
        counter = 0;
        setError(INSUFFICIENT_FLOW);
        clearError(FLOW_NOT_SETTLED);
      }
    } else {
      counter = 0;
      clearError(INSUFFICIENT_FLOW);
      clearError(FLOW_NOT_SETTLED);
    }

    // Tank level check (same for both)
    if (context->getTankLevel() > 0.0f) {
      clearError(LIQUID_TANK_EMPTY);
    } else {
      setError(LIQUID_TANK_EMPTY);
    }

    // GPS satellite check (same for both)
    if (services->gpsProvider->getSatelliteCount() < MIN_SATELLITES_NEEDED) {
      setError(NO_SATELLITE_CONNECTED);
    } else {
      clearError(NO_SATELLITE_CONNECTED);
    }
}

float DispenserChannel::getProcessedAreaPerSec() const {
  SystemContext* context = services->systemContext;
  return context->getGroundSpeed() * getBoomWidth();
}
