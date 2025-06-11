#pragma once
#include <NimBLEDevice.h>

typedef void (*BLEWriteCallback)(const char* data, size_t length);
typedef const char* (*BLEReadCallback)(void);
typedef void (*BLEConnCallback)(void);

class BLETextCallbacks;

class BLETextServer {
    friend class BLETextCallbacks;  // Allow access for callback handling

public:
    static constexpr size_t BUFFER_SIZE = 256;

    BLETextServer(const char* defaultName = "BLE-TextServer");

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
