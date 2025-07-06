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
#include "core/Constants.h"
#include <driver/ledc.h>

float DispenserChannel::tankLevel = 0.0f; // Default tank level
bool DispenserChannel::clientInWorkZone = false; // Default client work zone status
SystemContext* DispenserChannel::context = nullptr;

void DispenserChannel::init(String name, SystemContext* ctx, const VNH7070ASPins &motorPins) {
  context = ctx;
  channelName = name;
  channelIndex = 0;
  adcChannel = ADS1115Channels::CH0;  

  if (name == "Right") {
    channelIndex = 1;
    adcChannel = ADS1115Channels::CH1;
  }

  motorDriver.init(motorPins, channelIndex);
}

bool DispenserChannel::setTaskState(UserTaskState state) {
    bool validOp = false;

    switch (taskState) {
        case UserTaskState::Stopped:
            validOp = (state == UserTaskState::Started || state == UserTaskState::Testing);
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
        case UserTaskState::Testing:
            validOp = (state == UserTaskState::Stopped);
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
        incrementApplicationDuration();

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
  // Area covered per second in m²/s = speed × boom width
  return context->getGroundSpeed(true) * getBoomWidth();
}

float DispenserChannel::getTargetPositionForRate(float desiredKgPerDaa) const {
    float areaPerSec = getProcessedAreaPerSec();
    if (!isfinite(areaPerSec) || areaPerSec <= 0.0f || flowCoeff <= 0.0f) return 0.0f;

    float desiredFlowPerSec = (desiredKgPerDaa / Units::SQUARE_METERS_PER_DAA) * areaPerSec;
    float desiredPositionPercent = desiredFlowPerSec * flowCoeff;

    return constrain(desiredPositionPercent, 0.0f, 100.0f);
}

float DispenserChannel::getCurrentPositionPercent() const {
    float voltage = context->getADS1115().readFilteredVoltage(adcChannel);
    voltage = constrain(voltage, MIN_POT_VOLTAGE, MAX_POT_VOLTAGE);
    
    return (voltage - MIN_POT_VOLTAGE) / (MAX_POT_VOLTAGE - MIN_POT_VOLTAGE) * 100.0f;
}

/*
  position zero means no flow, position 100 means maximum flow.
  The position is calculated based on the voltage read from the potentiometer.
  The voltage is mapped to a percentage of the full range (0-100%).
*/
void DispenserChannel::applyPIControl() {
  float measured = getCurrentPositionPercent();
  float target = getTargetPositionForRate(targetFlowRatePerDaa);

  if (getTaskState() == UserTaskState::Paused || getTaskState() == UserTaskState::Stopped) {
    target = 0.0f; // If stopped, no flow
  } else if (getTaskState() == UserTaskState::Testing) {
    testTick += (testDirection ? 1 : -1);

    if (testTick >= 120) { // additional 20 ticks to keep servo staying at 100% for 2 secs.
      testDirection = false; // Reverse direction
    } else if (testTick <= 0) {
      // do not call 'setTaskState()' here (from timer isr), it logs to serial port which takes too long
      taskState = UserTaskState::Stopped; // Stop after one full cycle
      testTick = 0; // Reset test tick on stop
      testDirection = true;
    }

    target = testTick;
  }

  applyPIControl(target, measured);
}

void DispenserChannel::applyPIControl(float target, float measured) {
  float signal = piController.compute(target, measured);
  if (piController.isControlSignalChanged()) {
    motorDriver.setSpeed(static_cast<int8_t>(signal));
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
