#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include "BLETextServer.h"

#define BLE_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHAR_WRITE_UUID   "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHAR_READ_UUID    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

namespace {
    Preferences prefs;
}

class BLETextCallbacks : public NimBLECharacteristicCallbacks, public NimBLEServerCallbacks {
public:
    BLETextCallbacks(BLETextServer* server) : parent(server) {}

    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        parent->handleWrite(value.c_str(), value.length());
    }

    void onRead(NimBLECharacteristic* pCharacteristic) {
        if (parent->_readCb) {
            pCharacteristic->setValue(parent->_readCb());
        }
    }

    void onConnect(NimBLEServer* pServer) {
        if (parent->_connectCb) parent->_connectCb();
    }

    void onDisconnect(NimBLEServer* pServer) {
        if (parent->_disconnectCb) parent->_disconnectCb();
    }

private:
    BLETextServer* parent;
};

BLETextServer::BLETextServer(const char* defaultName) {
    prefs.begin("bletext", false);
    _deviceName = prefs.getString("devicename", defaultName).c_str();
}

void BLETextServer::onWrite(BLEWriteCallback cb) { _writeCb = cb; }
void BLETextServer::onRead(BLEReadCallback cb) { _readCb = cb; }
void BLETextServer::onConnect(BLEConnCallback cb) { _connectCb = cb; }
void BLETextServer::onDisconnect(BLEConnCallback cb) { _disconnectCb = cb; }

void BLETextServer::setDeviceName(const char* name, bool persist) {
    _deviceName = name;
    if (persist) {
        prefs.putString("devicename", name);
    }
}

const char* BLETextServer::getDeviceName() const {
    return _deviceName.c_str();
}

void BLETextServer::start() {
    NimBLEDevice::init(_deviceName.c_str());
    _server = NimBLEDevice::createServer();
    _callbacks = new BLETextCallbacks(this);
    _server->setCallbacks(_callbacks);

    NimBLEService* service = _server->createService(BLE_SERVICE_UUID);

    _writeChar = service->createCharacteristic(
        BLE_CHAR_WRITE_UUID,
        NIMBLE_PROPERTY::WRITE
    );

    _readChar = service->createCharacteristic(
        BLE_CHAR_READ_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    _writeChar->setCallbacks(_callbacks);
    _readChar->setCallbacks(_callbacks);

    service->start();
    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(BLE_SERVICE_UUID);
    advertising->start();

    memset(_bufferA, 0, BUFFER_SIZE);
    memset(_bufferB, 0, BUFFER_SIZE);
    _activeBuffer = _bufferA;
    _inactiveBuffer = _bufferB;
}

void BLETextServer::stop() {
    NimBLEDevice::deinit(true);
}

void BLETextServer::notify(const char* text) {
    if (_readChar && text) {
        _readChar->setValue(text);
        _readChar->notify();
    }
}

void BLETextServer::notifyFormatted(const char* format, ...) {
    static char buf[BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    notify(buf);
}

void BLETextServer::notifyString(const char* prefix, String str) {
  notifyFormatted("%s=%s", prefix, str.c_str());
}

void BLETextServer::notifyValue(const char* prefix, int value) {
  notifyFormatted("%s=%d", prefix, value);
}

void BLETextServer::notifyValue(const char* prefix, float value) {
  notifyFormatted("%s=%.2f", prefix, value);
}

void BLETextServer::notifyIndexedValue(const char* prefix, int index, int value) {
  notifyFormatted("%s%d=%d", prefix, index, value);
}

void BLETextServer::notifyIndexedValue(const char* prefix, int index, float value) {
  notifyFormatted("%s%d=%.2f", prefix, index, value);
}

const char* BLETextServer::getReceived() {
    return _inactiveBuffer[0] ? _inactiveBuffer : nullptr;
}

void BLETextServer::handleWrite(const char* data, size_t len) {
    if (len >= BUFFER_SIZE) len = BUFFER_SIZE - 1;
    memcpy(_inactiveBuffer, data, len);
    _inactiveBuffer[len] = '\0';
    swapBuffers();
    if (_writeCb) _writeCb(_activeBuffer, len);
}

void BLETextServer::swapBuffers() {
    char* temp = _activeBuffer;
    _activeBuffer = _inactiveBuffer;
    _inactiveBuffer = temp;
}
