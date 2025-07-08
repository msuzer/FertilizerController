// SerialHandler.h

#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include <stddef.h> // for size_t

// Typedef for the callback function
typedef void (*SerialMessageCallback)(const char* message, size_t length);

class SerialHandler {
public:
    SerialHandler(char* bufferA, char* bufferB, size_t bufferSize);

    void setCallback(SerialMessageCallback callback);

    void onReceiveChar(char c); // Call from ISR or data-receiving routine
    void process();             // Call from main loop

    bool hasMessage() const;        // Check if a message is pending
    bool isMessageTruncated() const; // True if the last message was truncated

private:
    char* rxBuffer_;
    char* processBuffer_;
    size_t bufferSize_;

    size_t rxIndex_;              // Index into rxBuffer_
    size_t messageLength_;        // Non-zero = message ready, stores length
    bool truncationFlag_;         // True if message was truncated

    SerialMessageCallback callback_;
};

#endif // SERIAL_HANDLER_H
