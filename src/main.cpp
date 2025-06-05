#include <Arduino.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include "VNH7070AS.h"
#include "ADS1115.h"
#include "CircularBuffer.h"
#include "PIController.h"
#include "BLETextServer.h"
#include "BLECommandParser.h"
#include "DS18B20Sensor.h"
#include "AppServices.h"
#include "SystemContext.h"
#include "SystemPreferences.h"
#include "GPSProvider.h"

#if CONFIG_IDF_TARGET_ESP32  // ESP32/PICO-D4
#include "esp32/rom/rtc.h"
#else
#error Target CONFIG_IDF_TARGET is not supported
#endif

#define VNH7070AS_INA1Pin  25
#define VNH7070AS_INB1Pin  14
#define VNH7070AS_PWM1Pin  26
#define VNH7070AS_SEL1Pin  27

#define VNH7070AS_INA2Pin  19
#define VNH7070AS_INB2Pin  16
#define VNH7070AS_PWM2Pin  18
#define VNH7070AS_SEL2Pin  17

#define GPS_UART_RX_PIN    13
#define GPS_UART_TX_PIN    21

#define RGB_LEDRPin        15
#define RGB_LEDGPin        2
#define RGB_LEDBPin        4

#define DS18B20_DataPin    22

#define I2C_SDAPin         32
#define I2C_SCLPin         33

#define ADS1115_I2C_ADDRESS 0x48

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
ADS1115 ads1115(Wire);

SystemContext ctx;
SystemPreferences prefs;
BLETextServer bleServer(DEFAULT_BLE_DEVICE_NAME);
BLECommandParser parser;
TinyGPSPlus gpsModule;
GPSProvider gpsProvider;
PIController pi1(DEFAULT_KP_VALUE, DEFAULT_KI_VALUE, -100.0f, 100.0f);
PIController pi2(DEFAULT_KP_VALUE, DEFAULT_KI_VALUE, -100.0f, 100.0f);

AppServices services(&ctx, &prefs, &bleServer, &parser, &gpsModule, &gpsProvider, &pi1, &pi2);

// Instantiate the drivers
// PWM write function using ESP32's ledcWrite
inline void writePwmESP32(uint8_t pin, uint8_t duty) { ledcWrite(pin, duty); }
inline void writeDigitalESP32(uint8_t pin, bool state) { digitalWrite(pin, state ? HIGH : LOW); }
VNH7070AS motor1(VNH7070AS_INA1Pin, VNH7070AS_INB1Pin, VNH7070AS_PWM1Pin, VNH7070AS_SEL1Pin, writePwmESP32, writeDigitalESP32);
VNH7070AS motor2(VNH7070AS_INA2Pin, VNH7070AS_INB2Pin, VNH7070AS_PWM2Pin, VNH7070AS_SEL2Pin, writePwmESP32, writeDigitalESP32);

// === Timer Setup ===
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
bool timerAlarmOccurred = false;

void die(const char* message);
void printResetReason(int reason);
static void taskLoopUpdateCallback(void *p);
static void controlLoopUpdateCallback(void *p);

static void taskLoopUpdateCallback(void *p) {
  // TODO Update Task Parameters Here!
}

static void controlLoopUpdateCallback(void *p) {
  portENTER_CRITICAL_ISR(&timerMux);

  constexpr float dt = 1.0f / CONTROL_LOOP_UPDATE_FREQUENCY_HZ;

  // Read feedback (pot values)
  // --- Compute averages and convert to voltage ---
  int16_t avgRaw0 = ch0.average();
  int16_t avgRaw1 = ch1.average();
  int16_t avgRaw2 = ch2.average();
  int16_t avgRaw3 = ch3.average();

  float pos1 = ads1115.rawToVoltage(avgRaw0);
  float pos2 = ads1115.rawToVoltage(avgRaw1);
  float current1 = ads1115.rawToCurrent(avgRaw2);
  float current2 = ads1115.rawToCurrent(avgRaw3);

  // Target setpoints (user-defined, e.g., from GUI or serial)
  volatile float target1 = ctx.getLeftChannel().getTargetFlowRatePerDaa();  // Example setpoint
  volatile float target2 = ctx.getRightChannel().getTargetFlowRatePerDaa();

  float duty1 = pi1.compute(target1, pos1, dt);
  float duty2 = pi2.compute(target2, pos2, dt);

  motor1.setSpeed(static_cast<int8_t>(duty1));
  motor2.setSpeed(static_cast<int8_t>(duty2));

  motor1.checkStuck(current1);
  motor2.checkStuck(current2);

  timerAlarmOccurred = true;

  portEXIT_CRITICAL_ISR(&timerMux);
}

void writeRGBLEDs(uint8_t chR, uint8_t chG, uint8_t chB) {
  digitalWrite(RGB_LEDRPin, chR);
  digitalWrite(RGB_LEDGPin, chG);
  digitalWrite(RGB_LEDBPin, chB);
}

void onWriteCallback(const char* message, size_t len) {
    if (message != nullptr) {
      Serial.printf("Received: %.*s\n", len, message);
      parser.dispatchInstruction(message);
    }
}

const char* onReadCallback() {
    return "ESP32 says hi!";
}

void onConnectCallback() {
  writeRGBLEDs(LOW, LOW, HIGH);
  Serial.println("Client connected!");
}

void onDisconnectCallback() {
  writeRGBLEDs(LOW, HIGH, LOW);
  Serial.println("Client disconnected!");
}

void die(const char* message) {
  printf(message);
  while(1) {
    writeRGBLEDs(HIGH, LOW, LOW);
    delay(100);
    writeRGBLEDs(LOW, LOW, LOW);
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

  motor1.setupPins();
  motor2.setupPins();

  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
  Wire.begin(I2C_SDAPin, I2C_SCLPin);

  writeRGBLEDs(LOW, HIGH, LOW);

  if (DS18B20Sensor::getInstance().begin(DS18B20_DataPin)) {
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

  if (!ads1115.begin(ADS1115_I2C_ADDRESS)) {
      die("ADS1115 not found!");
  }

  ads1115.setGain(ADS1115::Gain::FSR_4_096V); // Optional: Set gain

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

  gpsProvider.injectServices(&services);
  prefs.injectServices(&services);
  ctx.injectServices(&services);

  ctx.begin();
  
  bleServer.onWrite(onWriteCallback);
  bleServer.onRead(onReadCallback);
  bleServer.onConnect(onConnectCallback);
  bleServer.onDisconnect(onDisconnectCallback);

  bleServer.start();
}

void loop() {
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
