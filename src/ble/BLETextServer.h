// ============================================
// File: BLETextServer.h
// Purpose: BLE GATT server for text-based communication
// Part of: BLE Layer / Communication
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once
#include <NimBLEDevice.h>

#define DEFAULT_BLE_DEVICE_NAME         "AgroFertilizer"

typedef void (*BLEWriteCallback)(const char* data, size_t length);
typedef const char* (*BLEReadCallback)(void);
typedef void (*BLEConnCallback)(void);

class BLETextCallbacks;
class SystemContext;  // Forward declaration for SystemContext

class BLETextServer {
    friend class BLETextCallbacks;  // Allow access for callback handling
    friend class SystemContext;  // Allow SystemContext to access private members

public:
    static constexpr size_t BUFFER_SIZE = 256;

    BLETextServer(const BLETextServer&) = delete;
    BLETextServer& operator=(const BLETextServer&) = delete;
    BLETextServer(BLETextServer&&) = delete;
    BLETextServer& operator=(BLETextServer&&) = delete;

    void onWrite(BLEWriteCallback cb);
    void onRead(BLEReadCallback cb);
    void onConnect(BLEConnCallback cb);
    void onDisconnect(BLEConnCallback cb);

    void setDeviceName(const char* name, bool persist = true);
    const char* getDeviceName() const;

    void start();
    void stop();

    void notify(const char* text);
    void notifyFormatted(const char* format, ...);
    void notifyString(const char* prefix, String str);
    void notifyValue(const char* prefix, int value);
    void notifyValue(const char* prefix, float value);
    void notifyIndexedValue(const char* prefix, int index, int value);
    void notifyIndexedValue(const char* prefix, int index, float value);
    const char* getReceived();

private:
    BLETextServer(const char* defaultName = DEFAULT_BLE_DEVICE_NAME);
    void handleWrite(const char* data, size_t len);
    void swapBuffers();

    std::string _deviceName;
    BLEWriteCallback _writeCb = nullptr;
    BLEReadCallback _readCb = nullptr;
    BLEConnCallback _connectCb = nullptr;
    BLEConnCallback _disconnectCb = nullptr;

    NimBLEServer* _server = nullptr;
    NimBLECharacteristic* _writeChar = nullptr;
    NimBLECharacteristic* _readChar = nullptr;
    BLETextCallbacks* _callbacks = nullptr;

    char _bufferA[BUFFER_SIZE];
    char _bufferB[BUFFER_SIZE];
    char* _activeBuffer = nullptr;
    char* _inactiveBuffer = nullptr;
};
