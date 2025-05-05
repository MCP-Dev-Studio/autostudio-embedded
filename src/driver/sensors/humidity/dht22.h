#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief DHT22 sensor type enumeration
 */
typedef enum {
    DHT_TYPE_DHT11 = 0,     // DHT11 sensor (blue)
    DHT_TYPE_DHT22 = 1,     // DHT22 sensor (white) / AM2302
    DHT_TYPE_AM2320 = 2,    // AM2320 sensor
    DHT_TYPE_AM2321 = 3     // AM2321 sensor
} DHTType;

/**
 * @brief DHT22 configuration
 */
typedef struct {
    uint8_t pin;            // Data pin number
    DHTType type;           // Sensor type
    uint32_t readTimeout;   // Read timeout in milliseconds
    uint16_t minInterval;   // Minimum interval between reads in milliseconds
} DHT22Config;

/**
 * @brief DHT22 reading structure
 */
typedef struct {
    float temperature;      // Temperature in Celsius
    float humidity;         // Relative humidity in percentage
    uint32_t timestamp;     // Reading timestamp
    bool valid;             // Reading validity flag
} DHT22Reading;

/**
 * @brief DHT22 initialization
 * 
 * @param config DHT22 configuration
 * @return int 0 on success, negative error code on failure
 */
int DHT22_Init(const DHT22Config* config);

/**
 * @brief DHT22 deinitialization
 * 
 * @return int 0 on success, negative error code on failure
 */
int DHT22_Deinit(void);

/**
 * @brief Read temperature and humidity
 * 
 * @param reading Pointer to store sensor reading
 * @return int 0 on success, negative error code on failure
 */
int DHT22_Read(DHT22Reading* reading);

/**
 * @brief Read temperature only
 * 
 * @param temperature Pointer to store temperature in Celsius
 * @return int 0 on success, negative error code on failure
 */
int DHT22_ReadTemperature(float* temperature);

/**
 * @brief Read humidity only
 * 
 * @param humidity Pointer to store relative humidity in percentage
 * @return int 0 on success, negative error code on failure
 */
int DHT22_ReadHumidity(float* humidity);

/**
 * @brief Calculate heat index
 * 
 * @param temperature Temperature in Celsius
 * @param humidity Relative humidity in percentage
 * @param isFahrenheit Flag to return result in Fahrenheit
 * @return float Heat index in Celsius or Fahrenheit
 */
float DHT22_CalculateHeatIndex(float temperature, float humidity, bool isFahrenheit);

/**
 * @brief Calculate dew point
 * 
 * @param temperature Temperature in Celsius
 * @param humidity Relative humidity in percentage
 * @return float Dew point in Celsius
 */
float DHT22_CalculateDewPoint(float temperature, float humidity);

/**
 * @brief Convert Celsius to Fahrenheit
 * 
 * @param celsius Temperature in Celsius
 * @return float Temperature in Fahrenheit
 */
float DHT22_CelsiusToFahrenheit(float celsius);

/**
 * @brief Convert Fahrenheit to Celsius
 * 
 * @param fahrenheit Temperature in Fahrenheit
 * @return float Temperature in Celsius
 */
float DHT22_FahrenheitToCelsius(float fahrenheit);

/**
 * @brief Get minimum reading interval
 * 
 * @return uint16_t Minimum interval in milliseconds
 */
uint16_t DHT22_GetMinInterval(void);

/**
 * @brief Get last reading timestamp
 * 
 * @return uint32_t Last reading timestamp
 */
uint32_t DHT22_GetLastReadTimestamp(void);

/**
 * @brief Force a new reading (ignoring minimum interval)
 * 
 * @param reading Pointer to store sensor reading
 * @return int 0 on success, negative error code on failure
 */
int DHT22_ForceRead(DHT22Reading* reading);

#endif /* DHT22_H */