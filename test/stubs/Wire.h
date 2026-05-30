#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

class MockWire {
public:
    void begin() {}
    void begin(int sda, int scl) {
        (void)sda;
        (void)scl;
    }
    void end() {}
    void setClock(uint32_t hz) { (void)hz; }
    void setTimeOut(uint16_t timeoutMs) { (void)timeoutMs; }
    void beginTransmission(int address) { (void)address; }
    size_t write(uint8_t value) {
        (void)value;
        return 1;
    }
    size_t write(const uint8_t* buffer, size_t size) {
        (void)buffer;
        return size;
    }
    int endTransmission(bool stopBit = true) {
        (void)stopBit;
        return 0;
    }
    int requestFrom(int address, int quantity) {
        (void)address;
        return quantity;
    }
    size_t readBytes(uint8_t* buffer, size_t length) {
        if (buffer) {
            std::memset(buffer, 0, length);
        }
        return length;
    }
    int available() const { return 0; }
    int read() { return -1; }
};

inline MockWire Wire;
inline MockWire Wire1;
