#include "DispenserChannel.h"
#include "gps/GPSProvider.h"
#include "core/SystemContext.h"

float DispenserChannel::tankLevel = 0.0f; // Default tank level
bool DispenserChannel::clientInWorkZone = false; // Default client work zone status
SystemContext* DispenserChannel::context = nullptr;

void DispenserChannel::init(SystemContext* ctx, const VNH7070ASPins &motorPins) {
  context = ctx;
  motorDriver.init(motorPins, writePwmESP32, writeDigitalESP32);
}

bool DispenserChannel::setTaskState(UserTaskState state)
{
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

        // If transitioning to inactive state, clear relevant error flags
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

            clearError(ERRORS_TO_CLEAR);

            printf("[UI] Cleared relevant error flags due to task state transition to %s\n", getTaskStateName());
            printf("[UI] Aligning Motor To End\n");

            alignToEnd();
        }

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
    SystemParams & params = context->getPrefs().getParams();
    if (getTargetFlowRatePerDaa() > 0.0f) {
        if (context->getGroundSpeed() < params.minWorkingSpeed) {
            if (params.minWorkingSpeed > 0) {
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

void DispenserChannel::updateTaskMetrics() {
  if (!isTaskActive()) {
    return;  // Don't update metrics if not active
  }

  SystemParams & params = context->getPrefs().getParams();
  float groundSpeedKMPH = context->getGroundSpeed();
  float groundSpeedMPS = context->getGroundSpeed(true);

  int satCount = context->getGPSProvider().getSatelliteCount();
  float flowRatePerMin = getRealFlowRatePerMin();
    bool isBoomWidthOK = (getBoomWidth() > 0);
    bool isSpeedOK = (groundSpeedKMPH >= params.minWorkingSpeed);
    bool isFlowOK = (flowRatePerMin > 0);
    const float deltaTime = 1.0f;

    checkLowSpeedState();

    if (isSpeedOK && isBoomWidthOK) {
      if (isFlowOK) {
        // Update shared metrics only once per update (safe here — same slice for both channels)
        float processedAreaPerSec = getProcessedAreaPerSec();

        increaseDistanceTaken(groundSpeedMPS * deltaTime);
        increaseAreaProcessed(processedAreaPerSec);
        incrementTaskDuration();

        float slice = flowRatePerMin / 60.0f;
        decreaseTankLevel(slice);
        increaseLiquidConsumed(slice);

        printf("Ground Speed, Boom Width and Min Flow OK for one channel!\n");

        clearError(INSUFFICIENT_FLOW);

        if (abs(piController.getError()) >= FLOW_ERROR_WARNING_THRESHOLD) {
          if (++counter >= params.heartBeatPeriod) {
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
    if (getTankLevel() > 0.0f) {
      clearError(LIQUID_TANK_EMPTY);
    } else {
      setError(LIQUID_TANK_EMPTY);
    }

    // GPS satellite check (same for both)
    if (satCount < GPSProvider::MIN_SATELLITES_NEEDED) {
      setError(NO_SATELLITE_CONNECTED);
    } else {
      clearError(NO_SATELLITE_CONNECTED);
    }
}

float DispenserChannel::getProcessedAreaPerSec() const {
  return context->getGroundSpeed(true) * getBoomWidth();
}

void DispenserChannel::applyPIControl() {
  ADS1115& ads1115 = context->getADS1115();
  float measured = ads1115.readFilteredVoltage(channelIndex);
  applyPIControl(measured);
}

void DispenserChannel::applyPIControl(float measured) {
  if (isTaskActive()) {
    float target = getTargetFlowRatePerMin();
    // measured = getKgPerDaaInstantaneous(measured);
    float signal = piController.compute(target, measured);
    if (piController.isControlSignalChanged()) {
      motorDriver.setSpeed(static_cast<int8_t>(signal));
    }
  }
}

void DispenserChannel::alignToEnd(bool forward) {
  const float TARGET_POS = forward ? forwardLimitVoltage : backwardLimitVoltage;
  const float TOLERANCE = 0.05f; // Voltage tolerance
  const int ALIGN_SPEED = forward ? alignSpeed : -alignSpeed; // Fixed motor speed

  ADS1115& ads1115 = context->getADS1115();
  float pos = ads1115.readFilteredVoltage(channelIndex);

  if (abs(pos - TARGET_POS) > TOLERANCE) {
      motorDriver.setSpeed(ALIGN_SPEED);
  } else {
      motorDriver.setSpeed(0);
      printf("[%s] Motor aligned to %s end.\n", channelName.c_str(), forward ? "FORWARD" : "BACKWARD");
  }
}

const float DispenserChannel::getKgPerDaaInstantaneous(float potVoltage) const {
    float flowKgPerSec = potVoltage * flowCoeff;

    float areaPerSec = getProcessedAreaPerSec();

    if (areaPerSec > 0.0f) {
        return (flowKgPerSec / areaPerSec) * 1000.0f;
    } else {
        return 0.0f; // avoid division by zero
    }
}
