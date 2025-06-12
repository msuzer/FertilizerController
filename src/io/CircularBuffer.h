// ============================================
// File: CircularBuffer.h
// Purpose: 
// Part of: Hardware Abstraction Layer (HAL)
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once
#include <stdint.h>
#include <stddef.h>

class CircularBuffer {
public:
    CircularBuffer(int16_t* buffer, size_t capacity)
    : _buffer(buffer), _capacity(capacity), _head(0), _count(0), _sum(0) {}

    void push(int16_t value) {
        if (_count < _capacity) {
            _sum += value;
            _buffer[_head] = value;
            _head = (_head + 1) % _capacity;
            _count++;
        } else {
            _sum -= _buffer[_head];
            _sum += value;
            _buffer[_head] = value;
            _head = (_head + 1) % _capacity;
        }
    }

    int16_t average() const {
        return (_count == 0) ? 0 : static_cast<int16_t>(_sum / _count);
    }

    int16_t get(size_t index) const {
        if (index >= _count) return 0;
        size_t pos = (_head + _capacity - _count + index) % _capacity;
        return _buffer[pos];
    }

    size_t size() const {
        return _count;
    }

    void clear() {
        _head = 0;
        _count = 0;
        _sum = 0;
        for (size_t i = 0; i < _capacity; ++i) _buffer[i] = 0;
    }

private:
    int16_t* _buffer;
    size_t _capacity;
    size_t _head;
    size_t _count;
    int32_t _sum;
};
