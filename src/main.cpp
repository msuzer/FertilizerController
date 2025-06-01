#include <Arduino.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include "VNH7070AS.h"
#include "ADS1115.h"
#include "CircularBuffer.h"
#include "PIController.h"

#define INA1  25
#define INB1  14
#define PWM1  26
#define SEL1  27

#define INA2  19
#define INB2  16
#define PWM2  18
#define SEL2  17

#define GPS_UART_RX_PIN           13
#define GPS_UART_TX_PIN           21

#define RGB_LED_R   15
#define RGB_LED_G   2
#define RGB_LED_B   4

// === User-Defined Timer Frequency ===
#define CONTROL_FREQUENCY_HZ 10                  // Control loop frequency in Hz
#define TIMER_PERIOD_US (1000000 / CONTROL_FREQUENCY_HZ)  // Period in microseconds

// --- Define buffer size ---
constexpr size_t BUF_SIZE = 10;

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
VNH7070AS motor1(INA1, INB1, PWM1, SEL1, writePwmESP32, writeDigitalESP32);
VNH7070AS motor2(INA2, INB2, PWM2, SEL2, writePwmESP32, writeDigitalESP32);

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
  float pos1 = 20.0f; // ads1115Read(0);
  float pos2 = 30.0f; // ads1115Read(1);

  // Compute outputs
  float duty1 = pi1.compute(target1, pos1, dt);
  float duty2 = pi2.compute(target2, pos2, dt);

  // Apply to motors
  motor1.setSpeed(static_cast<int8_t>(duty1));
  motor2.setSpeed(static_cast<int8_t>(duty2));

  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
    Wire.begin(21, 22); // ESP32 default I2C pins

    if (!ads.begin(0x48)) {
        Serial.println("ADS1115 not found!");
        while (true); // Halt
    }

    ads.setGain(ADS1115::Gain::FSR_4_096V); // Optional: Set gain

    // Timer setup
    timer = timerBegin(0, 80, true);                      // Timer 0, prescaler 80 (1 tick = 1 µs)
    timerAttachInterrupt(timer, &controlLoopISR, true);    // Attach ISR
    timerAlarmWrite(timer, TIMER_PERIOD_US, true);         // Set period
    timerAlarmEnable(timer);                               // Enable timer
}

void loop() {
    // Example usage
    motor1.setSpeed(70);     // 70% forward
    motor2.setSpeed(-50);    // 50% reverse
    motor1.setSpeed(0);      // Stop

    motor1.selectDiagnostic(true);

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

    // --- Print results ---
    Serial.printf("CH0 Avg: %.4f V | CH1 Avg: %.4f V | CH2 Avg: %.4f V | CH3 Avg: %.4f V\n",
                   avgVoltage0, avgVoltage1, avgVoltage2, avgVoltage3);

    while (Serial1.available()) {
      gpsModule.encode(Serial1.read());
    }

    delay(500); // Sample rate
}
