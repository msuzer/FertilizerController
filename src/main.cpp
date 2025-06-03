#include <Arduino.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include "VNH7070AS.h"
#include "ADS1115.h"
#include "CircularBuffer.h"
#include "PIController.h"
#include "BLETextServer.h"
#include "BLECommandParser.h"

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

#define I2C_SDAPin         32
#define I2C_SCLPin         33

#define ADS1115_I2C_ADDRESS 0x48

// --- User thresholds (adjust as needed) ---
#define MOTOR_COUNT 2
const float STUCK_CURRENT_THRESHOLD = 2.5f; // Amps (example)
const int STUCK_DETECTION_COUNT = 5;        // Number of consecutive samples

// === User-Defined Timer Frequency ===
#define CONTROL_FREQUENCY_HZ 10                  // Control loop frequency in Hz
#define TIMER_PERIOD_US (1000000 / CONTROL_FREQUENCY_HZ)  // Period in microseconds

// --- Define buffer size ---
constexpr size_t BUF_SIZE = 8;

// --- Allocate fixed-size buffers for each ADS1115 channel ---
int16_t buffer0[BUF_SIZE];
int16_t buffer1[BUF_SIZE];
int16_t buffer2[BUF_SIZE];
int16_t buffer3[BUF_SIZE];

// --- Create CircularBuffer instances for each channel ---
CircularBuffer ch0(buffer0, BUF_SIZE);
CircularBuffer ch1(buffer1, BUF_SIZE);
CircularBuffer ch2(buffer2, BUF_SIZE);
CircularBuffer ch3(buffer3, BUF_SIZE);

// --- Create ADS1115 instance ---
ADS1115 ads(Wire);
TinyGPSPlus gpsModule;
extern BLECommandParser parser;
BLETextServer bleServer("MyBLEDevice");

void die(const char* message);
void verbose_print_reset_reason(int reason);

// PWM write function using ESP32's ledcWrite
void writePwmESP32(uint8_t pin, uint8_t duty) {
    ledcWrite(pin, duty);
}

// GPIO write function
void writeDigitalESP32(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

// PI Controllers for two motors
PIController pi1(0.5f, 0.1f, -100.0f, 100.0f);
PIController pi2(0.5f, 0.1f, -100.0f, 100.0f);

// Instantiate the drivers
VNH7070AS motor1(VNH7070AS_INA1Pin, VNH7070AS_INB1Pin, VNH7070AS_PWM1Pin, VNH7070AS_SEL1Pin, writePwmESP32, writeDigitalESP32);
VNH7070AS motor2(VNH7070AS_INA2Pin, VNH7070AS_INB2Pin, VNH7070AS_PWM2Pin, VNH7070AS_SEL2Pin, writePwmESP32, writeDigitalESP32);

// Target setpoints (user-defined, e.g., from GUI or serial)
volatile float target1 = 50.0f;  // Example setpoint (%)
volatile float target2 = 70.0f;

// === Timer Setup ===
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR controlLoopISR() {
  portENTER_CRITICAL_ISR(&timerMux);

  // Calculate dt from CONTROL_FREQUENCY_HZ
  constexpr float dt = 1.0f / CONTROL_FREQUENCY_HZ;

  // Read feedback (pot values)
  float pos1 = 20.0f;
  float pos2 = 30.0f;

  // Compute outputs
  float duty1 = pi1.compute(target1, pos1, dt);
  float duty2 = pi2.compute(target2, pos2, dt);

  // Apply to motors
  motor1.setSpeed(static_cast<int8_t>(duty1));
  motor2.setSpeed(static_cast<int8_t>(duty2));

  portEXIT_CRITICAL_ISR(&timerMux);
}

// Your resistor values (example)
const float Rtop = 4700.0f;   // 4.7k
const float Rbottom = 1000.0f; // 1k

float adsToCurrent(float adsVoltage) {
  const float csSensitivity = 0.010f;
  const float dividerFactor = (Rtop + Rbottom) / Rbottom; // 5.7
  float csVoltage = adsVoltage * dividerFactor;
  return csVoltage / csSensitivity;
}

bool checkMotorStuck(int motorId, float current) {
    static int stuckCounters[MOTOR_COUNT] = {0};

    if (motorId >= MOTOR_COUNT) return false;

    if (current >= STUCK_CURRENT_THRESHOLD) {
        stuckCounters[motorId]++;
        if (stuckCounters[motorId] >= STUCK_DETECTION_COUNT) {
            return true;
        }
    } else {
        stuckCounters[motorId] = 0;
    }

    return false;
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
    Serial.println("Client connected!");
}

void onDisconnectCallback() {
    Serial.println("Client disconnected!");
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
    Wire.begin(I2C_SDAPin, I2C_SCLPin);

    printf("CPU0 reset reason: ");
    verbose_print_reset_reason(rtc_get_reset_reason(0));

    printf("CPU1 reset reason: ");
    verbose_print_reset_reason(rtc_get_reset_reason(1));

    uint64_t chipid = ESP.getEfuseMac();  // Returns 64-bit number (top 16 bits are often constant)
    printf("Chip ID: %04X%08X\n", (uint16_t)(chipid>>32), (uint32_t)chipid);

    if (!ads.begin(ADS1115_I2C_ADDRESS)) {
        Serial.println("ADS1115 not found!");
        while (true); // Halt
    }

    ads.setGain(ADS1115::Gain::FSR_4_096V); // Optional: Set gain

    // Timer setup
    timer = timerBegin(0, 80, true);                      // Timer 0, prescaler 80 (1 tick = 1 µs)
    timerAttachInterrupt(timer, &controlLoopISR, true);    // Attach ISR
    timerAlarmWrite(timer, TIMER_PERIOD_US, true);         // Set period
    timerAlarmEnable(timer);                               // Enable timer

    bleServer.onWrite(onWriteCallback);
    bleServer.onRead(onReadCallback);
    bleServer.onConnect(onConnectCallback);
    bleServer.onDisconnect(onDisconnectCallback);

    bleServer.start();
}

void loop() {
  // --- Read and store raw ADC data ---
    int16_t raw0 = ads.readSingleEnded(0);
    int16_t raw1 = ads.readSingleEnded(1);
    int16_t raw2 = ads.readSingleEnded(2);
    int16_t raw3 = ads.readSingleEnded(3);

    ch0.push(raw0);
    ch1.push(raw1);
    ch2.push(raw2);
    ch3.push(raw3);

    // --- Compute averages and convert to voltage ---
    int16_t avgRaw0 = ch0.average();
    int16_t avgRaw1 = ch1.average();
    int16_t avgRaw2 = ch2.average();
    int16_t avgRaw3 = ch3.average();

    float avgVoltage0 = ads.rawToVoltage(avgRaw0);
    float avgVoltage1 = ads.rawToVoltage(avgRaw1);
    float avgVoltage2 = ads.rawToVoltage(avgRaw2);
    float avgVoltage3 = ads.rawToVoltage(avgRaw3);

    float current1 = adsToCurrent(avgVoltage2);
    float current2 = adsToCurrent(avgVoltage3);

    // --- Print results ---
    Serial.printf("CH0 Avg: %.4f V | CH1 Avg: %.4f V | CH2 Avg: %.4f V | CH3 Avg: %.4f V\n",
                   avgVoltage0, avgVoltage1, avgVoltage2, avgVoltage3);

    Serial.printf("Motor1 Avg: %.2f A | Motor2 Avg: %.2f A\n", current1, current2);

    if (checkMotorStuck(0, current1)) {
        Serial.println("MOTOR 1 STUCK!");
    }

    if (checkMotorStuck(1, current2)) {
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

    delay(10); // Sample rate
}

void die(const char* message) {
  printf(message);
  digitalWrite(RGB_LEDGPin, LOW);
  digitalWrite(RGB_LEDBPin, LOW);
  while(1) {
    digitalWrite(RGB_LEDRPin, HIGH);
    delay(100);
    digitalWrite(RGB_LEDRPin, LOW);
    delay(1000);
  }
}

// This function is called by the Arduino core for printf() support
extern "C" {
  int _write(int fd, const void* data, size_t size) {
    return Serial.write((const uint8_t*)data, size);
  }
}

void verbose_print_reset_reason(int reason) {
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
