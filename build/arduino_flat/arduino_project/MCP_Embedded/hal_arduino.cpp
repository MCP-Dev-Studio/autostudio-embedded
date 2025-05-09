#include "arduino_compat.h"
#include "hal_arduino.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Arduino HAL implementation that will be conditionally compiled by the Arduino compiler

/**
 * @brief Initialize the Arduino HAL
 */
int HAL_ArduinoInit(void) {
    // On actual Arduino, this would initialize hardware
    // For non-Arduino platforms, this is a placeholder
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // Serial.begin(115200);
    // Serial.println("Arduino HAL initialized");
#else
    printf("Arduino HAL initialized\n");
#endif
    return 0;
}

/**
 * @brief Deinitialize the Arduino HAL
 */
int HAL_ArduinoDeinit(void) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // Serial.println("Arduino HAL deinitialized");
#else
    printf("Arduino HAL deinitialized\n");
#endif
    return 0;
}

/**
 * @brief Configure a digital pin
 */
int HAL_ArduinoPinMode(uint8_t pin, uint8_t mode) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // pinMode(pin, mode);
#else
    printf("Set pin %u mode to %u\n", pin, mode);
#endif
    return 0;
}

/**
 * @brief Write to a digital pin
 */
int HAL_ArduinoDigitalWrite(uint8_t pin, uint8_t value) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // digitalWrite(pin, value);
#else
    printf("Digital write pin %u = %u\n", pin, value);
#endif
    return 0;
}

/**
 * @brief Read from a digital pin
 */
int HAL_ArduinoDigitalRead(uint8_t pin) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // return digitalRead(pin);
    return 0;
#else
    // For testing, return high for even pins, low for odd pins
    return (pin % 2 == 0) ? 1 : 0;
#endif
}

/**
 * @brief Write analog value (PWM) to a pin
 */
int HAL_ArduinoAnalogWrite(uint8_t pin, uint8_t value) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // analogWrite(pin, value);
#else
    printf("Analog write pin %u = %u\n", pin, value);
#endif
    return 0;
}

/**
 * @brief Read analog value from a pin
 */
int HAL_ArduinoAnalogRead(uint8_t pin) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // return analogRead(pin);
    return 0;
#else
    // For testing, return pin number * 10
    return pin * 10;
#endif
}

/**
 * @brief Attach interrupt to a pin
 */
int HAL_ArduinoAttachInterrupt(uint8_t pin, uint8_t mode, void (*callback)(void*), void* arg) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // attachInterrupt(digitalPinToInterrupt(pin), (void (*)())callback, mode);
#else
    printf("Attach interrupt to pin %u, mode %u\n", pin, mode);
#endif
    return 0;
}

/**
 * @brief Detach interrupt from a pin
 */
int HAL_ArduinoDetachInterrupt(uint8_t pin) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // detachInterrupt(digitalPinToInterrupt(pin));
#else
    printf("Detach interrupt from pin %u\n", pin);
#endif
    return 0;
}

/**
 * @brief Configure a software serial port
 */
int HAL_ArduinoSerialConfig(uint8_t instance, uint8_t rxPin, uint8_t txPin, uint32_t baudRate) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code using SoftwareSerial
    // Would need to instantiate the appropriate SoftwareSerial objects
#else
    printf("Serial %u config: RX=%u, TX=%u, baud=%u\n", 
           instance, rxPin, txPin, baudRate);
#endif
    return 0;
}

/**
 * @brief Read from a serial port
 */
int HAL_ArduinoSerialRead(uint8_t instance, uint8_t* buffer, size_t size, uint32_t timeout) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // Would need to select the correct Serial instance and read
    return 0;
#else
    // For testing, fill buffer with test data
    if (buffer != NULL && size > 0) {
        memset(buffer, 'A', size);
        return size;
    }
    return 0;
#endif
}

/**
 * @brief Write to a serial port
 */
int HAL_ArduinoSerialWrite(uint8_t instance, const uint8_t* data, size_t size) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // Would need to select the correct Serial instance and write
    return 0;
#else
    printf("Serial %u write %zu bytes\n", instance, size);
    return size;
#endif
}

/**
 * @brief Configure an I2C interface
 */
int HAL_ArduinoI2CConfig(uint8_t instance, uint8_t sdaPin, uint8_t sclPin, uint32_t frequency) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // Wire.begin(sdaPin, sclPin);
    // Wire.setClock(frequency);
#else
    printf("I2C %u config: SDA=%u, SCL=%u, freq=%u Hz\n", 
           instance, sdaPin, sclPin, frequency);
#endif
    return 0;
}

/**
 * @brief Write to an I2C device
 */
int HAL_ArduinoI2CWrite(uint8_t instance, uint8_t address, const uint8_t* data, size_t size, bool stop) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // Wire.beginTransmission(address);
    // Wire.write(data, size);
    // return Wire.endTransmission(stop) ? 0 : size;
    return 0;
#else
    printf("I2C %u write to address 0x%02X, %zu bytes, stop=%d\n", 
           instance, address, size, stop);
    return size;
#endif
}

/**
 * @brief Read from an I2C device
 */
int HAL_ArduinoI2CRead(uint8_t instance, uint8_t address, uint8_t* data, size_t size) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // Wire.requestFrom(address, size);
    // size_t read = 0;
    // while (Wire.available() && read < size) {
    //     data[read++] = Wire.read();
    // }
    // return read;
    return 0;
#else
    printf("I2C %u read from address 0x%02X, %zu bytes\n", 
           instance, address, size);
    // For testing, fill buffer with test data
    if (data != NULL && size > 0) {
        memset(data, 0xAA, size);
        return size;
    }
    return 0;
#endif
}

/**
 * @brief Configure an SPI interface
 */
int HAL_ArduinoSPIConfig(uint8_t instance, uint8_t mosiPin, uint8_t misoPin, uint8_t sckPin, 
                         uint32_t frequency, uint8_t mode) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // SPI.begin(sckPin, misoPin, mosiPin);
    // SPI.setFrequency(frequency);
    // SPI.setDataMode(mode);
#else
    printf("SPI %u config: MOSI=%u, MISO=%u, SCK=%u, freq=%u Hz, mode=%u\n", 
           instance, mosiPin, misoPin, sckPin, frequency, mode);
#endif
    return 0;
}

/**
 * @brief Transfer data over SPI
 */
int HAL_ArduinoSPITransfer(uint8_t instance, const uint8_t* txData, uint8_t* rxData, size_t size) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // for (size_t i = 0; i < size; i++) {
    //     if (rxData) {
    //         rxData[i] = SPI.transfer(txData ? txData[i] : 0xFF);
    //     } else {
    //         SPI.transfer(txData ? txData[i] : 0xFF);
    //     }
    // }
    return 0;
#else
    printf("SPI %u transfer %zu bytes\n", instance, size);
    // For testing, copy TX data to RX buffer
    if (txData != NULL && rxData != NULL && size > 0) {
        memcpy(rxData, txData, size);
        return size;
    }
    return 0;
#endif
}

/**
 * @brief Read from EEPROM
 */
int HAL_ArduinoEEPROMRead(uint16_t address, uint8_t* data, size_t size) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // for (size_t i = 0; i < size; i++) {
    //     if (address + i < EEPROM.length()) {
    //         data[i] = EEPROM.read(address + i);
    //     } else {
    //         return i;
    //     }
    // }
    return 0;
#else
    printf("EEPROM read address 0x%04X, %zu bytes\n", address, size);
    // For testing, fill buffer with test data
    if (data != NULL && size > 0) {
        memset(data, 0xBB, size);
        return size;
    }
    return 0;
#endif
}

/**
 * @brief Write to EEPROM
 */
int HAL_ArduinoEEPROMWrite(uint16_t address, const uint8_t* data, size_t size) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // for (size_t i = 0; i < size; i++) {
    //     if (address + i < EEPROM.length()) {
    //         EEPROM.write(address + i, data[i]);
    //     } else {
    //         return i;
    //     }
    // }
    // EEPROM.commit();
    return 0;
#else
    printf("EEPROM write address 0x%04X, %zu bytes\n", address, size);
    return size;
#endif
}

/**
 * @brief Get system time in milliseconds
 */
uint32_t HAL_ArduinoMillis(void) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // return millis();
    return 0;
#else
    // Simplified implementation for testing
    static uint32_t time = 0;
    return time++;
#endif
}

/**
 * @brief Delay for specified milliseconds
 */
void HAL_ArduinoDelay(uint32_t ms) {
#ifdef MCP_PLATFORM_ARDUINO
    // Arduino-specific code
    // delay(ms);
#else
    printf("Delay %u ms\n", ms);
    // No actual delay in testing implementation
#endif
}
