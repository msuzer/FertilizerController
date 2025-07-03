// ============================================
// File: DispenserChannel.cpp
// Purpose: Represents and controls one fertilizer dispenser channel
// Part of: Control Layer
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include "DispenserChannel.h"
#include "gps/GPSProvider.h"
#include "core/SystemContext.h"
#include "ble/UserInfoFormatter.h"
#include "core/LogUtils.h"
#include "core/DebugInfoPrinter.h"
#include <driver/ledc.h>

float DispenserChannel::tankLevel = 0.0f; // Default tank level
bool DispenserChannel::clientInWorkZone = false; // Default client work zone status
SystemContext* DispenserChannel::context = nullptr;

void DispenserChannel::init(String name, SystemContext* ctx, const VNH7070ASPins &motorPins) {
  context = ctx;
  channelName = name;
  uint8_t channelIndex = (channelName == "Left") ? LEDC_CHANNEL_0 : LEDC_CHANNEL_1;
  motorDriver.init(motorPins, channelIndex);
}

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

            LogUtils::info("[STATE] Cleared relevant error flags due to task state transition to %s\n", getTaskStateName());
            LogUtils::warn("[MOTOR] Aligning Motor To End\n");

            alignToEnd(true, 10000); // Align to end with a 10 seconds timeout
        }

        return true;
    }

    LogUtils::warn("[STATE] Invalid state transition: %s → %s\n", getTaskStateName(), taskStateToString(state));
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
                    LogUtils::warn("[FLOW] %s Channel Task Paused due to Low Speed\n", channelName);
                }
            }
        } else {
            if (lowSpeedFlag) {
                if (isTaskPaused()) {
                    LogUtils::info("[FLOW] Resuming %s Channel Task\n", channelName);
                    lowSpeedFlag = false;
                    // Optional: channel.setTaskState(UserTaskState::Resuming);
                    setTaskState(UserTaskState::Resuming);
                }
            }
        }
    }
}

void DispenserChannel::reportErrorFlags(void) {
  static int oldErrorFlags = NO_ERROR;
  static int counter = 0;
  int heartBeatPeriod = context->getPrefs().getParams().heartBeatPeriod;

  // error is reported periodically and instantly if it is only NO_ERROR
  bool reportInstantly = (oldErrorFlags != errorFlags) && (errorFlags == NO_ERROR);
  if (reportInstantly || (++counter == heartBeatPeriod)) {
    counter = 0;
    String packet = UserInfoFormatter::makeErrorInfoPacket(errorFlags, true);
    context->getCommandHandler().sendBLEPacketChecked(packet);
  }
  oldErrorFlags = errorFlags;
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

        LogUtils::info("[FLOW] Ground Speed, Boom Width and Min Flow OK for one channel!\n");

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
  uint8_t channelIndex = (channelName == "Left") ? ADS1115Channels::CH0 : ADS1115Channels::CH1;
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

bool DispenserChannel::alignToEnd(bool forward, unsigned long timeoutMs) {
  const float TARGET_POS = forward ? forwardLimitVoltage : backwardLimitVoltage;
  const float TOLERANCE = 0.05f; // Voltage tolerance
  const int ALIGN_SPEED = forward ? alignSpeed : -alignSpeed;
  const unsigned long POLL_INTERVAL_MS = 50;

  ADS1115& ads1115 = context->getADS1115();
  uint8_t channelIndex = (channelName == "Left") ? ADS1115Channels::CH0 : ADS1115Channels::CH1;

  unsigned long startTime = millis();
  float pos = ads1115.readFilteredVoltage(channelIndex);
  bool reachedEnd = (abs(pos - TARGET_POS) <= TOLERANCE);

  if (!reachedEnd) {
    motorDriver.setSpeed(ALIGN_SPEED);

    while (!reachedEnd && (millis() - startTime < timeoutMs)) {
      delay(POLL_INTERVAL_MS);  // blocking wait
      pos = ads1115.readFilteredVoltage(channelIndex);
      reachedEnd = (abs(pos - TARGET_POS) <= TOLERANCE);
    }

    motorDriver.setSpeed(0);  // stop motor

    if (reachedEnd) {
      LogUtils::info("[MOTOR] [%s] Motor aligned to %s end.\n",
                     channelName.c_str(), forward ? "FORWARD" : "BACKWARD");
    } else {
      LogUtils::warn("[MOTOR] [%s] Timeout while aligning to %s end!\n",
                     channelName.c_str(), forward ? "FORWARD" : "BACKWARD");
    }
  } else {
    motorDriver.setSpeed(0);  // already at end
    LogUtils::info("[MOTOR] [%s] Already at %s end.\n",
                   channelName.c_str(), forward ? "FORWARD" : "BACKWARD");
  }

  return reachedEnd;
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

void DispenserChannel::printMotorCurrent(void) {
  ADS1115& ads1115 = context->getADS1115();
  ads1115.pushBuffer(); // Push all channels to the buffer
  
  // --- Compute averages and convert to voltage ---
  float pos1 = ads1115.readFilteredVoltage(ADS1115Channels::CH0);
  float pos2 = ads1115.readFilteredVoltage(ADS1115Channels::CH1);
  float current1 = ads1115.readFilteredCurrent(ADS1115Channels::CH2);
  float current2 = ads1115.readFilteredCurrent(ADS1115Channels::CH3);

  DebugInfoPrinter::printMotorDiagnostics(pos1, pos2, current1, current2);
}

void DispenserChannel::testMotorRamp(void) {
  LogUtils::info("[MOTOR] [%s] Starting motor ramp test\n", channelName.c_str());

  // Ramp up forward
  LogUtils::info("[MOTOR] Ramp Up 0 -> 100\n");
  for (int pwm = 0; pwm <= VNH7070AS::MAX_DUTY; pwm++) {
    printMotorCurrent();
    motorDriver.setSpeed(pwm);
    delay(50);
  }

  // Dead time
  LogUtils::info("[MOTOR] at full speed (CW)!\n");
  delay(2000);

  // Ramp down forward
  LogUtils::info("[MOTOR] Ramp Down 100 -> 0\n");
  for (int pwm = VNH7070AS::MAX_DUTY; pwm >= 0; pwm--) {
    printMotorCurrent();
    motorDriver.setSpeed(pwm);
    delay(50);
  }

  // Dead time
  LogUtils::info("[MOTOR] Stop.\n");
  delay(2000);

  // Ramp up reverse
  LogUtils::info("[MOTOR] Ramp Up Reverse 0 -> -100\n");
  for (int pwm = 0; pwm >= -VNH7070AS::MAX_DUTY; pwm--) {
    printMotorCurrent();
    motorDriver.setSpeed(pwm);
    delay(50);
  }

  // Dead time
  LogUtils::info("[MOTOR] at full speed (CCW)!\n");
  delay(2000);

  // Ramp down reverse
  LogUtils::info("[MOTOR] Ramp Down Reverse -100 -> 0\n");
  for (int pwm = -VNH7070AS::MAX_DUTY; pwm <= 0; pwm++) {
    printMotorCurrent();
    motorDriver.setSpeed(pwm);
    delay(50);
  }

  LogUtils::info("[MOTOR] Stop.\n");
  delay(2000);
  LogUtils::info("[MOTOR] [%s] Motor ramp test complete\n", channelName.c_str());
}
