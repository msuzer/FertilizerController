#include <Arduino.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include "io/VNH7070AS.h"
#include "io/ADS1115.h"
#include "control/PIController.h"
#include "ble/BLETextServer.h"
#include "ble/BLECommandParser.h"
#include "io/DS18B20Sensor.h"
#include "core/SystemContext.h"
#include "core/SystemPreferences.h"
#include "gps/GPSProvider.h"

#if CONFIG_IDF_TARGET_ESP32  // ESP32/PICO-D4
#include "esp32/rom/rtc.h"
#else
#error Target CONFIG_IDF_TARGET is not supported
#endif

// === User-Defined Timer Frequency ===
#define TASK_LOOP_UPDATE_FREQUENCY_HZ         1  // Task loop frequency in Hz
#define TIMER_PERIOD_US(Freq)                 (1000000 / Freq)  // Period in microseconds

// --- Create Services ---
SystemContext context;

// === Timer Setup ===
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
bool notifyDeferredTasks = false;
static bool timeToRefresh = false;

void die(const char* message);
void printResetReason(int reason);
static void taskLoopUpdateCallback(void *p);
static void controlLoopUpdateCallback(void *p);
void printRealTimeData(void);
static void printErrorCode(int errorCode);

// periodic callback on 1 second interval
static void taskLoopUpdateCallback(void *p) {
  static int counterRefresh = 0;

  // Process each channel
  context.getLeftChannel().updateTaskMetrics();
  context.getRightChannel().updateTaskMetrics();

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

esp_err_t setupPeriodicAlarm(const esp_timer_create_args_t & timer_args, uint64_t period) {
  esp_timer_handle_t timer_handle;
  esp_err_t ret;

  ret = esp_timer_create(&timer_args, &timer_handle);
  if (ret != ESP_OK) {
    return ret;
  }

  ret = esp_timer_start_periodic(timer_handle, period);
  if (ret != ESP_OK) {
    return ret;
  }

  return ESP_OK;
}

void setup() {
  pinMode(RGB_LEDRPin, OUTPUT);
  pinMode(RGB_LEDGPin, OUTPUT);
  pinMode(RGB_LEDBPin, OUTPUT);

  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);

  context.writeRGBLEDs(LOW, HIGH, LOW);

  printf("CPU0 reset reason: ");
  printResetReason(rtc_get_reset_reason(0));

  printf("CPU1 reset reason: ");
  printResetReason(rtc_get_reset_reason(1));

  esp_timer_create_args_t timer_args1 = {
    .callback = taskLoopUpdateCallback,
    .arg = NULL,
    .name = "user_timer"
  };

  if (setupPeriodicAlarm(timer_args1, TIMER_PERIOD_US(TASK_LOOP_UPDATE_FREQUENCY_HZ)) != ESP_OK) {
    die("User Timer Setup Failed!\n");
  }

  esp_timer_create_args_t timer_args2 = {
    .callback = controlLoopUpdateCallback,
    .arg = NULL,
    .name = "pcnt_timer"
  };

  if (setupPeriodicAlarm(timer_args2, TIMER_PERIOD_US(CONTROL_LOOP_UPDATE_FREQUENCY_HZ)) != ESP_OK) {
    die("PCNT Timer Setup Failed!\n");
  }

  context.init();
  
  DS18B20Sensor& tempSensor = context.getTempSensor();
  if (tempSensor.isReady()) {
      Serial.println("DS18B20 found!");
      float temp = tempSensor.getTemperatureC();
      String id = tempSensor.getSensorID();
      Serial.printf("Sensor ID: %s | Temperature: %.2f °C\n", id.c_str(), temp);
  } else {
      Serial.println("DS18B20 not found.");
  }

  BLETextServer& bleServer = context.getBLETextServer();

  bleServer.start();
}

void loop() {
  ADS1115& ads1115 = context.getADS1115();
  BLETextServer& bleServer = context.getBLETextServer();
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
  
    // --- Print results ---
    Serial.printf("Pot1: %.4f V | Pot2: %.4f V | Curr1: %.4f V | Curr2: %.4f V\n",
                   pos1, pos2, current1, current2);

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
      printRealTimeData();
    }
}

void printRealTimeData(void) {
  printf("[LOG] Time: %lu | TargetFlow: %.2f | RealFlow: %.2f | Error: %.2f | CtrlSig: %d | "
    "Distance: %.2f | AreaPerSec: %.2f | Liquid: %.2f\n",
     millis(),
     0,
     0,
     0,
     0,
     0,
     0,
     0
   );

   context.getGPSProvider().printGPSData();
}

static void printErrorCode(int errorCode) {
  printf("Error Code: %08X Message: ", errorCode);

  if (errorCode & LIQUID_TANK_EMPTY) {
    printf("Liquid Tank Empty!\n");
  }
  if (errorCode & INSUFFICIENT_FLOW) {
    printf("Insufficient Flow!\n");
  }
  if (errorCode & FLOW_NOT_SETTLED) {
    printf("Flow Not Settled!\n");
  }
  if (errorCode & BATTERY_LOW) {
    printf("Battery Low!\n");
  }
  if (errorCode & NO_SATELLITE_CONNECTED) {
    printf("No Satellite Connected!\n");
  }
  if (errorCode & INVALID_SATELLITE_INFO) {
    printf("Invalid Satellite Info!\n");
  }
  if (errorCode & INVALID_GPS_LOCATION) {
    printf("Invalid GPS Location!\n");
  }
  if (errorCode & INVALID_GPS_SPEED) {
    printf("Invalid GPS Speed!\n");
  }
  if (errorCode & INVALID_PARAM_COUNT) {
    printf("Invalid Param Count!\n");
  }
  if (errorCode & MESSAGE_PARSE_ERROR) {
    printf("Message Parse Error!\n");
  }
  if (errorCode & HARDWARE_ERROR) {
    printf("Hardware Error!\n");
  }
  if (errorCode == 0) {
    printf("No Error!\n");
  }
}

// This function is called by the Arduino core for printf() support
extern "C" {
  int _write(int fd, const void* data, size_t size) {
    return Serial.write((const uint8_t*)data, size);
  }
}

void printResetReason(int reason) {
  switch (reason) {
    case 1:  printf("Vbat power on reset\n"); break;
    case 3:  printf("Software reset digital core\n"); break;
    case 4:  printf("Legacy watch dog reset digital core\n"); break;
    case 5:  printf("Deep Sleep reset digital core\n"); break;
    case 6:  printf("Reset by SLC module, reset digital core\n"); break;
    case 7:  printf("Timer Group0 Watch dog reset digital core\n"); break;
    case 8:  printf("Timer Group1 Watch dog reset digital core\n"); break;
    case 9:  printf("RTC Watch dog Reset digital core\n"); break;
    case 10: printf("Instrusion tested to reset CPU\n"); break;
    case 11: printf("Time Group reset CPU\n"); break;
    case 12: printf("Software reset CPU\n"); break;
    case 13: printf("RTC Watch dog Reset CPU\n"); break;
    case 14: printf("for APP CPU, reset by PRO CPU\n"); break;
    case 15: printf("Reset when the vdd voltage is not stable\n"); break;
    case 16: printf("RTC Watch dog reset digital core and rtc module\n"); break;
    default: printf("Unspecified error caused Reset\n");
  }
}
