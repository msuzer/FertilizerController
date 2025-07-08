// SerialHandler.cpp

#include "SerialHandler.h"

SerialHandler::SerialHandler(char* bufferA, char* bufferB, size_t bufferSize)
    : rxBuffer_(bufferA), processBuffer_(bufferB), bufferSize_(bufferSize),
      rxIndex_(0), messageLength_(0), truncationFlag_(false), callback_(nullptr) {
    if (rxBuffer_) rxBuffer_[0] = '\0';
    if (processBuffer_) processBuffer_[0] = '\0';
}

void SerialHandler::setCallback(SerialMessageCallback callback) {
    callback_ = callback;
}

void SerialHandler::onReceiveChar(char c) {
    if (c == '\n') {
        // Null-terminate the message
        if (rxIndex_ < bufferSize_) {
            rxBuffer_[rxIndex_] = '\0';
        } else {
            rxBuffer_[bufferSize_ - 1] = '\0';
        }

        // Swap buffers
        char* temp = rxBuffer_;
        rxBuffer_ = processBuffer_;
        processBuffer_ = temp;

        // Store message length
        messageLength_ = rxIndex_;

        // Reset index for next message
        rxIndex_ = 0;
    } else {
        if (rxIndex_ < bufferSize_ - 1) { // Leave space for null terminator
            rxBuffer_[rxIndex_++] = c;
        } else {
            // Set truncation flag on overflow
            truncationFlag_ = true;
        }
    }
}

void SerialHandler::process() {
    if (messageLength_ > 0 && callback_) {
        callback_(processBuffer_, messageLength_);
        messageLength_ = 0;
        truncationFlag_ = false; // Reset after delivering the message
    }
}

bool SerialHandler::hasMessage() const {
    return messageLength_ > 0;
}

bool SerialHandler::isMessageTruncated() const {
    return truncationFlag_;
}
