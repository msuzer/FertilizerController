#include <Arduino.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include "io/VNH7070AS.h"
#include "io/ADS1115.h"
#include "io/CircularBuffer.h"
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
#define CONTROL_LOOP_UPDATE_FREQUENCY_HZ      10 // Control loop frequency in Hz
#define TIMER_PERIOD_US(Freq)                 (1000000 / Freq)  // Period in microseconds

// --- Allocate fixed-size buffers for each ADS1115 channel ---
constexpr size_t ADS1115_BUF_SIZE = 8;

int16_t buffer0[ADS1115_BUF_SIZE];
int16_t buffer1[ADS1115_BUF_SIZE];
int16_t buffer2[ADS1115_BUF_SIZE];
int16_t buffer3[ADS1115_BUF_SIZE];

// --- Create CircularBuffer instances for each channel ---
CircularBuffer ch0(buffer0, ADS1115_BUF_SIZE);
CircularBuffer ch1(buffer1, ADS1115_BUF_SIZE);
CircularBuffer ch2(buffer2, ADS1115_BUF_SIZE);
CircularBuffer ch3(buffer3, ADS1115_BUF_SIZE);

// --- Create Services ---

SystemContext context;

// Instantiate the drivers
// PWM write function using ESP32's ledcWrite

// === Timer Setup ===
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
bool timerAlarmOccurred = false;
static bool timeToRefresh = false;

void die(const char* message);
void printResetReason(int reason);
static void taskLoopUpdateCallback(void *p);
static void controlLoopUpdateCallback(void *p);

static void taskLoopUpdateCallback(void *p) {
  static int counterRefresh = 0;

  // Process each channel
  context.getLeftChannel().updateChannel();
  context.getLeftChannel().updateChannel();

  // Auto-refresh
  int arPeriod = context.getPrefs().getParams().autoRefreshPeriod;
  if ((arPeriod > 0) && (++counterRefresh >= arPeriod)) {
    counterRefresh = 0;
    timeToRefresh = true;
  }
}

static void controlLoopUpdateCallback(void *p) {
  portENTER_CRITICAL_ISR(&timerMux);

  constexpr float dt = 1.0f / CONTROL_LOOP_UPDATE_FREQUENCY_HZ;

  // Read feedback (pot values)
  // --- Compute averages and convert to voltage ---
  ADS1115& ads1115 = context.getADS1115();
  float pos1 = ads1115.rawToVoltage(ch0.average());
  float pos2 = ads1115.rawToVoltage(ch1.average());
  float current1 = ads1115.rawToCurrent(ch2.average());
  float current2 = ads1115.rawToCurrent(ch3.average());

  // Target setpoints (user-defined, e.g., from GUI or serial)
  volatile float target1 = context.getLeftChannel().getTargetFlowRatePerMin();  // Example setpoint
  volatile float target2 = context.getRightChannel().getTargetFlowRatePerMin();

  PIController& pi1 = context.getLeftChannel().getPIController();
  PIController& pi2 = context.getRightChannel().getPIController();
  float duty1 = pi1.compute(target1, pos1, dt);
  float duty2 = pi2.compute(target2, pos2, dt);

  VNH7070AS& motor1 = context.getLeftChannel().getMotor();
  VNH7070AS& motor2 = context.getRightChannel().getMotor();

  motor1.setSpeed(static_cast<int8_t>(duty1));
  motor2.setSpeed(static_cast<int8_t>(duty2));

  motor1.checkStuck(current1);
  motor2.checkStuck(current2);

  timerAlarmOccurred = true;

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

  if (DS18B20Sensor::getInstance().init(DS18B20_DataPin)) {
      Serial.println("DS18B20 found!");
      float temp = DS18B20Sensor::getInstance().getTemperatureC();
      String id = DS18B20Sensor::getInstance().getSensorID();
      Serial.printf("Sensor ID: %s | Temperature: %.2f °C\n", id.c_str(), temp);
  } else {
      Serial.println("DS18B20 not found.");
  }

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
  
  BLETextServer& bleServer = context.getBLETextServer();

  bleServer.start();
}

void loop() {
  ADS1115& ads1115 = context.getADS1115();
  BLETextServer& bleServer = context.getBLETextServer();
  VNH7070AS& motor1 = context.getLeftChannel().getMotor();
  VNH7070AS& motor2 = context.getRightChannel().getMotor();
  TinyGPSPlus& gpsModule = context.getGPSModule();

  if (timerAlarmOccurred) {
    timerAlarmOccurred = false;

    // --- Read and store raw ADC data ---
    int16_t raw0 = ads1115.readSingleEnded(0);
    int16_t raw1 = ads1115.readSingleEnded(1);
    int16_t raw2 = ads1115.readSingleEnded(2);
    int16_t raw3 = ads1115.readSingleEnded(3);

    ch0.push(raw0);
    ch1.push(raw1);
    ch2.push(raw2);
    ch3.push(raw3);
  }

    // --- Compute averages and convert to voltage ---
    int16_t avgRaw0 = ch0.average();
    int16_t avgRaw1 = ch1.average();
    int16_t avgRaw2 = ch2.average();
    int16_t avgRaw3 = ch3.average();

    float avgVoltage0 = ads1115.rawToVoltage(avgRaw0);
    float avgVoltage1 = ads1115.rawToVoltage(avgRaw1);

    float current1 = ads1115.rawToCurrent(avgRaw2);
    float current2 = ads1115.rawToCurrent(avgRaw3);

    // --- Print results ---
    Serial.printf("Pot1: %.4f V | Pot2: %.4f V | Curr1: %.4f V | Curr2: %.4f V\n",
                   avgVoltage0, avgVoltage1, current1, current2);

    if (motor1.isStuck()) {
        Serial.println("MOTOR 1 STUCK!");
    }

    if (motor2.isStuck()) {
        Serial.println("MOTOR 2 STUCK!");
    }

    while (Serial1.available()) {
      gpsModule.encode(Serial1.read());
    }

    const char* msg = bleServer.getReceived();
    if (msg) {
        Serial.printf("Buffered: %s\n", msg);
    }

    static uint32_t last = 0;
    if (millis() - last > 5000) {
        last = millis();
        bleServer.notifyFormatted("Status update at %lu ms", last);
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
