# Fertilizer Dispenser Control System

This project implements a precision fertilizer dispenser system using ESP32, with BLE-based control and monitoring, PI control of flow, GPS-based speed compensation, and real-time diagnostics.

## Main Components

- **SystemContext** — Main object managing all system components
- **DebugInfoPrinter** — Centralized debug printing for diagnostics
- **BLETextServer** — BLE GATT server for communication
- **CommandHandler** — Parses and handles BLE commands
- **DispenserChannel** — Manages one dispenser channel, flow PI control
- **PIController** — PI controller for flow control
- **GPSProvider** — Interface to TinyGPSPlus module
- **ADS1115** — ADC driver (filtered) for potentiometer and current readings
- **DS18B20Sensor** — Temperature sensor driver
- **VNH7070AS** — Motor driver interface

## PlatformIO Configuration

PlatformIO uses the following configuration (see `platformio.ini`):

- **Platform:** espressif32
- **Board:** esp32dev
- **Framework:** arduino

### Libraries
- TinyGPSPlus
- NimBLE-Arduino
- DallasTemperature
- OneWire

### Build Flags

```
-I src
```

### Extra Scripts
- `extra_scripts/post_merge_bin.py`

## Build and Run

```bash
pio run
pio upload
pio monitor
```

## Notes

- Real-time data and diagnostics are provided via DebugInfoPrinter
- BLE commands follow the format handled by CommandHandler
- `printAll()` can be called periodically for full system status

## License

Proprietary License
