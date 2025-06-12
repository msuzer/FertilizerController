// ============================================
// Main Application Entry Point
// 
// - SystemContext manages all core services
// - DebugInfoPrinter provides system diagnostics and debug printing
// - Periodic tasks run via ESP timers (taskLoop, controlLoop)
// - BLETextServer provides BLE communication interface
// - DS18B20, GPS, Motors, Flow Control handled via core services
//
// Structure:
//   - setup(): Initializes system, BLE, timers
//   - loop(): Processes periodic control tasks and GPS data
//   - taskLoopUpdateCallback(): Task state and metrics updates
//   - controlLoopUpdateCallback(): PI control loop updates
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================

#include <Arduino.h>
#include <esp32/rom/rtc.h>
#include <Wire.h>
#include <TinyGPSPlus.h>

#include "ble/BLETextServer.h"
#include "io/VNH7070AS.h"
#include "io/ADS1115.h"
#include "io/DS18B20Sensor.h"

#include "core/SystemContext.h"
#include "core/DebugInfoPrinter.h"

// === User-Defined Timer Frequency ===
#define TASK_LOOP_UPDATE_FREQUENCY_HZ         1  // Task loop frequency in Hz
#define TIMER_PERIOD_US(Freq)                 (1000000ull / Freq)  // Period in microseconds


// --- Create Services ---
SystemContext context;

// === Timer Setup ===
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
bool notifyDeferredTasks = false;
static bool timeToRefresh = false;

void die(const char* message);
static void taskLoopUpdateCallback(void *p);
static void controlLoopUpdateCallback(void *p);

// periodic callback on 1 second interval
static void taskLoopUpdateCallback(void *p) {
  static int counterRefresh = 0;

  DispenserChannel& leftChannel = context.getLeftChannel();
  DispenserChannel& rightChannel = context.getRightChannel();

  // Process each channel
  leftChannel.checkLowSpeedState();
  rightChannel.checkLowSpeedState();

  leftChannel.updateTaskMetrics();
  rightChannel.updateTaskMetrics();

  leftChannel.reportErrorFlags();
  rightChannel.reportErrorFlags();

  // Auto-refresh
  int arPeriod = context.getPrefs().getParams().autoRefreshPeriod;
  if ((arPeriod > 0) && (++counterRefresh >= arPeriod)) {
    counterRefresh = 0;
    timeToRefresh = true;
  }
}

// periodic callback 10 Hz frequency
static void controlLoopUpdateCallback(void *p) {
  portENTER_CRITICAL_ISR(&timerMux);

  context.getLeftChannel().applyPIControl();
  context.getRightChannel().applyPIControl();

  notifyDeferredTasks = true;

  portEXIT_CRITICAL_ISR(&timerMux);
}

void die(const char* message) {
  printf(message);
  while(1) {
    context.writeRGBLEDs(HIGH, LOW, LOW);
    delay(100);
    context.writeRGBLEDs(LOW, LOW, LOW);
    delay(1000);
  }
}

esp_err_t setupPeriodicAlarmWrapper(const char* timerName, esp_timer_cb_t callback, uint64_t periodUs) {
    esp_timer_create_args_t timer_args = {
        .callback = callback,
        .arg = NULL,
        .name = timerName
    };

    esp_timer_handle_t timer_handle;
    esp_err_t ret = esp_timer_create(&timer_args, &timer_handle);
    if (ret != ESP_OK) {
        printf("[TIMER] ERROR creating timer '%s'!\n", timerName);
        return ret;
    }

    ret = esp_timer_start_periodic(timer_handle, periodUs);
    if (ret != ESP_OK) {
        printf("[TIMER] ERROR starting timer '%s'!\n", timerName);
        return ret;
    }

    printf("[TIMER] Created '%s' period %llu ms\n", timerName, periodUs / 1000ull);
    return ESP_OK;
}

void setup() {
  pinMode(RGB_LEDRPin, OUTPUT);
  pinMode(RGB_LEDGPin, OUTPUT);
  pinMode(RGB_LEDBPin, OUTPUT);

  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);

  context.writeRGBLEDs(LOW, HIGH, LOW);

  DebugInfoPrinter::printResetReason("CPU0", rtc_get_reset_reason(0));
  DebugInfoPrinter::printResetReason("CPU1", rtc_get_reset_reason(1));

  DebugInfoPrinter::printVersionInfo();

  esp_err_t ret = setupPeriodicAlarmWrapper("taskLoop_timer", 
    taskLoopUpdateCallback, TIMER_PERIOD_US(TASK_LOOP_UPDATE_FREQUENCY_HZ));

  if (ret != ESP_OK) {
    die("Task Loop Timer Setup Failed!\n");
  }

  ret = setupPeriodicAlarmWrapper("controlLoop_timer", 
    controlLoopUpdateCallback, TIMER_PERIOD_US(CONTROL_LOOP_UPDATE_FREQUENCY_HZ));

  if (ret != ESP_OK) {
    die("Control Loop Timer Setup Failed!\n");
  }

  context.init();
  
  DebugInfoPrinter::printTempSensorStatus(context.getTempSensor());

  BLETextServer& bleServer = context.getBLETextServer();

  bleServer.start();
}

void loop() {
  ADS1115& ads1115 = context.getADS1115();
  VNH7070AS& motor1 = context.getLeftChannel().getMotor();
  VNH7070AS& motor2 = context.getRightChannel().getMotor();
  TinyGPSPlus& gpsModule = context.getGPSModule();

  if (notifyDeferredTasks) {
    notifyDeferredTasks = false;

    ads1115.pushBuffer(ADS1115Channels::CH0);
    ads1115.pushBuffer(ADS1115Channels::CH1);
    ads1115.pushBuffer(ADS1115Channels::CH2);
    ads1115.pushBuffer(ADS1115Channels::CH3);

    // --- Compute averages and convert to voltage ---
    float pos1 = ads1115.readFilteredVoltage(ADS1115Channels::CH0);
    float pos2 = ads1115.readFilteredVoltage(ADS1115Channels::CH1);
    float current1 = ads1115.readFilteredCurrent(ADS1115Channels::CH2);
    float current2 = ads1115.readFilteredCurrent(ADS1115Channels::CH3);
  
    DebugInfoPrinter::printMotorDiagnostics(pos1, pos2, current1, current2);

    if (motor1.checkStuck(current1)) {
        Serial.println("MOTOR 1 STUCK!");
        context.getLeftChannel().setError(MOTOR_STUCK);
        context.getLeftChannel().setTaskState(UserTaskState::Paused);
    }

    if (motor2.checkStuck(current2)) {
        Serial.println("MOTOR 2 STUCK!");
        context.getRightChannel().setError(MOTOR_STUCK);
        context.getRightChannel().setTaskState(UserTaskState::Paused);
    }
  }

  while (Serial1.available()) {
    gpsModule.encode(Serial1.read());
  }

  if (timeToRefresh) {
    timeToRefresh = false;
    if (context.getLeftChannel().isClientInWorkZone()) {
      CommandHandler::getInstance().handlerGetTaskInfo({}); // pass empty ParsedInstruction
    }
    DebugInfoPrinter::printAll(context);
  }
}

// This function is called by the Arduino core for printf() support
extern "C" {
  int _write(int fd, const void* data, size_t size) {
    return Serial.write((const uint8_t*)data, size);
  }
}
