#include "hal_esp32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple implementation of ESP32 HAL functions for testing

int HAL_ESP32Init(void) {
    printf("ESP32 HAL initialized\n");
    return 0;
}

int HAL_ESP32Deinit(void) {
    printf("ESP32 HAL deinitialized\n");
    return 0;
}

int HAL_ESP32PinMode(uint8_t pin, uint8_t mode) {
    printf("Set pin %u mode to %u\n", pin, mode);
    return 0;
}

int HAL_ESP32DigitalWrite(uint8_t pin, uint8_t value) {
    printf("Digital write pin %u = %u\n", pin, value);
    return 0;
}

int HAL_ESP32DigitalRead(uint8_t pin) {
    // For testing, return high for even pins, low for odd pins
    return (pin % 2 == 0) ? 1 : 0;
}

int HAL_ESP32AnalogWrite(uint8_t pin, uint8_t value) {
    printf("Analog write pin %u = %u\n", pin, value);
    return 0;
}

int HAL_ESP32AnalogRead(uint8_t pin, uint8_t attenuationDb, uint8_t width) {
    // For testing, return pin number * 100
    return pin * 100;
}

int HAL_ESP32PWMConfig(uint8_t pin, uint8_t channel, uint32_t frequency, uint8_t resolution) {
    printf("PWM config pin %u, channel %u, freq %u Hz, res %u bits\n", 
           pin, channel, frequency, resolution);
    return 0;
}

int HAL_ESP32PWMSetDutyCycle(uint8_t channel, uint32_t dutyCycle) {
    printf("PWM channel %u duty cycle = %u\n", channel, dutyCycle);
    return 0;
}

int HAL_ESP32AttachInterrupt(uint8_t pin, uint8_t mode, void (*callback)(void*), void* arg) {
    printf("Attach interrupt to pin %u, mode %u\n", pin, mode);
    return 0;
}

int HAL_ESP32DetachInterrupt(uint8_t pin) {
    printf("Detach interrupt from pin %u\n", pin);
    return 0;
}

int HAL_ESP32UARTConfig(uint8_t uart, uint8_t txPin, uint8_t rxPin, 
                        uint32_t baudRate, uint32_t config) {
    printf("UART %u config: TX=%u, RX=%u, baud=%u\n", 
           uart, txPin, rxPin, baudRate);
    return 0;
}

int HAL_ESP32UARTRead(uint8_t uart, uint8_t* buffer, size_t size, uint32_t timeout) {
    // For testing, fill buffer with test data
    if (buffer != NULL && size > 0) {
        memset(buffer, 'A', size);
        return size;
    }
    return 0;
}

int HAL_ESP32UARTWrite(uint8_t uart, const uint8_t* data, size_t size) {
    printf("UART %u write %zu bytes\n", uart, size);
    return size;
}

int HAL_ESP32I2CConfig(uint8_t i2c, uint8_t sdaPin, uint8_t sclPin, uint32_t frequency) {
    printf("I2C %u config: SDA=%u, SCL=%u, freq=%u Hz\n", 
           i2c, sdaPin, sclPin, frequency);
    return 0;
}

int HAL_ESP32I2CWrite(uint8_t i2c, uint8_t address, const uint8_t* data, size_t size, bool stop) {
    printf("I2C %u write to address 0x%02X, %zu bytes, stop=%d\n", 
           i2c, address, size, stop);
    return size;
}

int HAL_ESP32I2CRead(uint8_t i2c, uint8_t address, uint8_t* data, size_t size) {
    printf("I2C %u read from address 0x%02X, %zu bytes\n", 
           i2c, address, size);
    // For testing, fill buffer with test data
    if (data != NULL && size > 0) {
        memset(data, 0xAA, size);
        return size;
    }
    return 0;
}

int HAL_ESP32SPIConfig(uint8_t spi, int8_t mosiPin, int8_t misoPin, int8_t sckPin, 
                       int8_t csPin, uint32_t frequency, uint8_t mode) {
    printf("SPI %u config: MOSI=%d, MISO=%d, SCK=%d, CS=%d, freq=%u Hz, mode=%u\n", 
           spi, mosiPin, misoPin, sckPin, csPin, frequency, mode);
    return 0;
}

int HAL_ESP32SPITransfer(uint8_t spi, const uint8_t* txData, uint8_t* rxData, size_t size) {
    printf("SPI %u transfer %zu bytes\n", spi, size);
    // For testing, copy TX data to RX buffer
    if (txData != NULL && rxData != NULL && size > 0) {
        memcpy(rxData, txData, size);
        return size;
    }
    return 0;
}

int HAL_ESP32FlashRead(uint32_t address, uint8_t* data, size_t size) {
    printf("Flash read address 0x%08X, %zu bytes\n", address, size);
    // For testing, fill buffer with test data
    if (data != NULL && size > 0) {
        memset(data, 0xBB, size);
        return size;
    }
    return 0;
}

int HAL_ESP32FlashWrite(uint32_t address, const uint8_t* data, size_t size) {
    printf("Flash write address 0x%08X, %zu bytes\n", address, size);
    return size;
}

int HAL_ESP32FlashEraseSector(uint32_t sector) {
    printf("Flash erase sector %u\n", sector);
    return 0;
}

uint32_t HAL_ESP32GetFreeHeap(void) {
    return 262144; // 256KB for testing
}

uint32_t HAL_ESP32Millis(void) {
    // Simplified implementation for testing
    static uint32_t time = 0;
    return time++;
}

void HAL_ESP32Delay(uint32_t ms) {
    printf("Delay %u ms\n", ms);
    // No actual delay in testing implementation
}

void HAL_ESP32Restart(void) {
    printf("ESP32 restarting...\n");
    // In a real implementation, this would reset the ESP32
}

int HAL_ESP32GetChipInfo(uint32_t* model, uint32_t* revision, uint32_t* cores, uint32_t* features) {
    if (model != NULL) *model = 1; // ESP32
    if (revision != NULL) *revision = 1;
    if (cores != NULL) *cores = 2;
    if (features != NULL) *features = 0x12345678;
    return 0;
}