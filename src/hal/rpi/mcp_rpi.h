/**
 * @file mcp_rpi.h
 * @brief MCP implementation for bare metal Raspberry Pi
 */
#ifndef MCP_RPI_H
#define MCP_RPI_H

#include "hal_rpi.h"
#include "../../mcp_os_core.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Raspberry Pi platform configuration
 */
typedef struct MCP_RPiConfig {
    const char* deviceName;     // Maps to common.device_name
    const char* version;        // Maps to common.firmware_version
    bool enableDebug;           // Maps to common.debug_enabled
    bool enablePersistence;     // Maps to common.enable_persistence
    uint32_t heapSize;          // Maps to common.heap_size
    const char* configFile;     // Maps to common.config_file_path
    bool enableServer;          // Maps to common.server_enabled
    uint16_t serverPort;        // Maps to common.server_port
    bool autoStartServer;       // Maps to common.auto_start_server
    bool enableI2C;             // Maps to common.interfaces.i2c.enabled
    int i2cBusNumber;           // Maps to common.interfaces.i2c.bus_number
    bool enableSPI;             // Maps to common.interfaces.spi.enabled
    int spiBusNumber;           // Maps to common.interfaces.spi.bus_number
    bool enableUART;            // Maps to common.interfaces.uart.enabled
    int uartNumber;             // Maps to common.interfaces.uart.number
    uint32_t uartBaudRate;      // Maps to common.interfaces.uart.baud_rate
    bool enableGPIO;            // Maps to common.interfaces.gpio.enabled
    bool enableWifi;            // Maps to common.network.enabled
    const char* ssid;           // Maps to common.network.ssid
    const char* password;       // Maps to common.network.password
    bool enableBLE;             // Maps to common.network.ble.enabled
    bool enableEthernet;        // Maps to common.network.ethernet.enabled
    const char* ethernetInterface; // Maps to common.network.ethernet.interface
    bool enableHotspot;         // Maps to common.network.ap.enabled
    const char* hotspotSsid;    // Maps to common.network.ap.ssid
    const char* hotspotPassword; // Maps to common.network.ap.password
    uint16_t hotspotChannel;    // Maps to common.network.ap.channel
    bool enableCamera;          // Maps to platform.rpi.enable_camera
    int cameraResolution;       // Maps to platform.rpi.camera_resolution
} MCP_RPiConfig;

// Platform-specific MCP initialization
int MCP_PlatformInit(void);

// Platform-specific MCP deinitialization
int MCP_PlatformDeinit(void);

/**
 * @brief Initialize the MCP system for Raspberry Pi platform
 * 
 * @param config Platform configuration (can be NULL for defaults)
 * @return int 0 on success, negative error code on failure
 */
int MCP_SystemInit(const MCP_RPiConfig* config);

/**
 * @brief Start the MCP server
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_ServerStart(void);

/**
 * @brief Load persistent state from storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoadPersistentState(void);

/**
 * @brief Save persistent state to storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_SavePersistentState(void);

/**
 * @brief Process system tasks
 * 
 * @param timeout_ms Maximum time to spend processing tasks (0 for no timeout)
 * @return int Number of tasks processed or negative error code
 */
int MCP_SystemProcess(uint32_t timeout_ms);

/**
 * @brief Deinitialize the MCP system
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_SystemDeinit(void);

/**
 * @brief Get system status as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_SystemGetStatus(char* buffer, size_t bufferSize);

/**
 * @brief Set debug output enable state
 * 
 * @param enable Enable debug output
 * @return int Previous enable state
 */
int MCP_SystemSetDebug(bool enable);

/**
 * @brief Print debug message (if debug enabled)
 * 
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void MCP_SystemDebugPrint(const char* format, ...);

/**
 * @brief Restart the Raspberry Pi
 */
void MCP_Restart(void);

// --- Memory management ---

// Allocate memory
void* MCP_Malloc(size_t size);

// Free memory
void MCP_Free(void* ptr);

// Reallocate memory
void* MCP_Realloc(void* ptr, size_t size);

// Get memory usage statistics
void MCP_GetMemoryStats(size_t* total, size_t* used, size_t* peak);

// --- Time and delay functions ---

// Get system time in milliseconds
uint64_t MCP_GetTimeMs(void);

// Delay for specified milliseconds
void MCP_DelayMs(uint32_t ms);

// --- Task scheduling ---

// Task handle type
typedef int MCP_TaskHandle;

// Task function type
typedef void (*MCP_TaskFunction)(void* arg);

// Task priority levels
typedef enum {
    MCP_TASK_PRIORITY_LOW,
    MCP_TASK_PRIORITY_NORMAL,
    MCP_TASK_PRIORITY_HIGH,
    MCP_TASK_PRIORITY_CRITICAL
} MCP_TaskPriority;

// Create a new task
MCP_TaskHandle MCP_TaskCreate(MCP_TaskFunction func, const char* name, size_t stack_size, 
                              void* arg, MCP_TaskPriority priority);

// Delete a task
int MCP_TaskDelete(MCP_TaskHandle task);

// Suspend a task
int MCP_TaskSuspend(MCP_TaskHandle task);

// Resume a task
int MCP_TaskResume(MCP_TaskHandle task);

// Yield execution to other tasks
void MCP_TaskYield(void);

// Get current task handle
MCP_TaskHandle MCP_TaskGetCurrent(void);

// Get task name
const char* MCP_TaskGetName(MCP_TaskHandle task);

// --- Mutex synchronization ---

// Mutex handle type
typedef int MCP_MutexHandle;

// Create a mutex
MCP_MutexHandle MCP_MutexCreate(void);

// Delete a mutex
int MCP_MutexDelete(MCP_MutexHandle mutex);

// Lock a mutex
int MCP_MutexLock(MCP_MutexHandle mutex, uint32_t timeout_ms);

// Unlock a mutex
int MCP_MutexUnlock(MCP_MutexHandle mutex);

// --- Semaphore synchronization ---

// Semaphore handle type
typedef int MCP_SemaphoreHandle;

// Create a semaphore
MCP_SemaphoreHandle MCP_SemaphoreCreate(uint32_t max_count, uint32_t initial_count);

// Delete a semaphore
int MCP_SemaphorDelete(MCP_SemaphoreHandle semaphore);

// Take a semaphore
int MCP_SemaphoreTake(MCP_SemaphoreHandle semaphore, uint32_t timeout_ms);

// Give a semaphore
int MCP_SemaphoreGive(MCP_SemaphoreHandle semaphore);

// --- Timer management ---

// Timer handle type
typedef int MCP_TimerHandle;

// Timer callback type
typedef void (*MCP_TimerCallback)(void* arg);

// Create a timer
MCP_TimerHandle MCP_TimerCreate(const char* name, uint32_t period_ms, bool auto_reload, 
                               void* arg, MCP_TimerCallback callback);

// Delete a timer
int MCP_TimerDelete(MCP_TimerHandle timer);

// Start a timer
int MCP_TimerStart(MCP_TimerHandle timer);

// Stop a timer
int MCP_TimerStop(MCP_TimerHandle timer);

// Change timer period
int MCP_TimerChangePeriod(MCP_TimerHandle timer, uint32_t period_ms);

// --- Event management ---

// Event bits type
typedef uint32_t MCP_EventBits;

// Event group handle type
typedef int MCP_EventGroupHandle;

// Create an event group
MCP_EventGroupHandle MCP_EventGroupCreate(void);

// Delete an event group
int MCP_EventGroupDelete(MCP_EventGroupHandle group);

// Set bits in an event group
MCP_EventBits MCP_EventGroupSetBits(MCP_EventGroupHandle group, MCP_EventBits bits);

// Clear bits in an event group
MCP_EventBits MCP_EventGroupClearBits(MCP_EventGroupHandle group, MCP_EventBits bits);

// Wait for bits in an event group
MCP_EventBits MCP_EventGroupWaitBits(MCP_EventGroupHandle group, MCP_EventBits bits,
                                    bool clear_on_exit, bool wait_for_all, uint32_t timeout_ms);

// --- Queue management ---

// Queue handle type
typedef int MCP_QueueHandle;

// Create a queue
MCP_QueueHandle MCP_QueueCreate(size_t item_size, size_t max_items);

// Delete a queue
int MCP_QueueDelete(MCP_QueueHandle queue);

// Send an item to the back of a queue
int MCP_QueueSend(MCP_QueueHandle queue, const void* item, uint32_t timeout_ms);

// Send an item to the front of a queue
int MCP_QueueSendToFront(MCP_QueueHandle queue, const void* item, uint32_t timeout_ms);

// Receive an item from a queue
int MCP_QueueReceive(MCP_QueueHandle queue, void* item, uint32_t timeout_ms);

// Get the number of items in a queue
size_t MCP_QueueGetCount(MCP_QueueHandle queue);

// Reset a queue
int MCP_QueueReset(MCP_QueueHandle queue);

// --- File system operations ---

// File handle type
typedef void* MCP_FileHandle;

// File open mode
typedef enum {
    MCP_FILE_MODE_READ,
    MCP_FILE_MODE_WRITE,
    MCP_FILE_MODE_APPEND,
    MCP_FILE_MODE_READ_WRITE,
    MCP_FILE_MODE_READ_WRITE_CREATE
} MCP_FileMode;

// Open a file
MCP_FileHandle MCP_FileOpen(const char* path, MCP_FileMode mode);

// Close a file
int MCP_FileClose(MCP_FileHandle file);

// Read from a file
size_t MCP_FileRead(MCP_FileHandle file, void* buffer, size_t size);

// Write to a file
size_t MCP_FileWrite(MCP_FileHandle file, const void* buffer, size_t size);

// Seek within a file
int MCP_FileSeek(MCP_FileHandle file, long offset, int origin);

// Get file position
long MCP_FileTell(MCP_FileHandle file);

// Check if file exists
bool MCP_FileExists(const char* path);

// Delete a file
int MCP_FileDelete(const char* path);

// Create a directory
int MCP_DirCreate(const char* path);

// Delete a directory
int MCP_DirDelete(const char* path);

// --- GPIO operations ---

// GPIO pin configuration
typedef enum {
    MCP_GPIO_MODE_INPUT,
    MCP_GPIO_MODE_OUTPUT,
    MCP_GPIO_MODE_INPUT_PULLUP,
    MCP_GPIO_MODE_INPUT_PULLDOWN,
    MCP_GPIO_MODE_ALT0,
    MCP_GPIO_MODE_ALT1,
    MCP_GPIO_MODE_ALT2,
    MCP_GPIO_MODE_ALT3,
    MCP_GPIO_MODE_ALT4,
    MCP_GPIO_MODE_ALT5
} MCP_GPIOMode;

// Configure GPIO pin
int MCP_GPIOConfig(int pin, MCP_GPIOMode mode);

// Set GPIO output level
int MCP_GPIOWrite(int pin, int level);

// Read GPIO input level
int MCP_GPIORead(int pin);

// --- UART operations ---

// UART handle type
typedef int MCP_UARTHandle;

// UART configuration
typedef struct {
    int uart_num;
    int baudrate;
    int data_bits;
    int stop_bits;
    int parity;
    bool flow_control;
} MCP_UARTConfig;

// Initialize UART
MCP_UARTHandle MCP_UARTInit(const MCP_UARTConfig* config);

// Deinitialize UART
int MCP_UARTDeinit(MCP_UARTHandle uart);

// Write data to UART
size_t MCP_UARTWrite(MCP_UARTHandle uart, const uint8_t* data, size_t size);

// Read data from UART
size_t MCP_UARTRead(MCP_UARTHandle uart, uint8_t* data, size_t size, uint32_t timeout_ms);

// Check if UART has received data available
int MCP_UARTAvailable(MCP_UARTHandle uart);

// --- Sensor system operations ---

// Initialize the sensor system
int MCP_SensorSystemInit(void);

// Register a sensor
int MCP_SensorRegister(const char* id, const char* name, const char* type, int interface, int pin, const char* driver_id);

// Unregister a sensor
int MCP_SensorUnregister(const char* id);

// Read a sensor value
int MCP_SensorRead(const char* id, void* value, size_t size);

// Enable or disable a sensor
int MCP_SensorSetEnabled(const char* id, bool enabled);

// Get sensor information
int MCP_SensorGetInfo(const char* id, char* buffer, size_t size);

// Get list of all sensors
int MCP_SensorListAll(char* buffer, size_t size);

// --- Network operations ---

// WiFi connection status codes
typedef enum {
    MCP_WIFI_STATUS_DISCONNECTED = 0,
    MCP_WIFI_STATUS_CONNECTING = 1,
    MCP_WIFI_STATUS_CONNECTED = 2,
    MCP_WIFI_STATUS_CONNECTION_FAILED = 3,
    MCP_WIFI_STATUS_CONNECTION_LOST = 4
} MCP_WiFiStatus;

// Configure and connect to WiFi network
int MCP_WiFiConnect(const char* ssid, const char* password, uint32_t timeout_ms);

// Disconnect from WiFi network
int MCP_WiFiDisconnect(void);

// Get current WiFi status
MCP_WiFiStatus MCP_WiFiGetStatus(void);

// Get WiFi IP address as string
int MCP_WiFiGetIP(char* buffer, size_t size);

// Get WiFi SSID
int MCP_WiFiGetSSID(char* buffer, size_t size);

// Get WiFi signal strength (RSSI)
int MCP_WiFiGetRSSI(void);

// Start WiFi in AP (hotspot) mode
int MCP_WiFiStartAP(const char* ssid, const char* password, uint16_t channel);

// Stop WiFi AP mode
int MCP_WiFiStopAP(void);

// Check if WiFi hotspot is active
bool MCP_WiFiIsAPActive(void);

// Initialize BLE
int MCP_BLEInit(const char* deviceName);

// Start BLE advertising
int MCP_BLEStartAdvertising(void);

// Stop BLE advertising
int MCP_BLEStopAdvertising(void);

// Check if BLE is connected
bool MCP_BLEIsConnected(void);

// Configure Ethernet
int MCP_EthernetConfigure(const char* interface, bool useDHCP, const char* staticIP, 
                          const char* gateway, const char* subnet);

// Get current Ethernet status
int MCP_EthernetGetStatus(void);

// Get Ethernet IP address as string
int MCP_EthernetGetIP(char* buffer, size_t size);

// Scan for available WiFi networks
int MCP_WiFiScan(char* buffer, size_t size);

// --- I2C operations ---

// I2C handle type
typedef int MCP_I2CHandle;

// I2C configuration
typedef struct {
    int i2c_num;
    int scl_pin;
    int sda_pin;
    int clock_speed;
} MCP_I2CConfig;

// Initialize I2C
MCP_I2CHandle MCP_I2CInit(const MCP_I2CConfig* config);

// Deinitialize I2C
int MCP_I2CDeinit(MCP_I2CHandle i2c);

// Write data to I2C device
int MCP_I2CWrite(MCP_I2CHandle i2c, uint8_t address, const uint8_t* data, size_t size);

// Read data from I2C device
int MCP_I2CRead(MCP_I2CHandle i2c, uint8_t address, uint8_t* data, size_t size);

// --- SPI operations ---

// SPI handle type
typedef int MCP_SPIHandle;

// SPI configuration
typedef struct {
    int spi_num;
    int clock_div;
    int mode;
    int cs_pin;
    int miso_pin;
    int mosi_pin;
    int sck_pin;
} MCP_SPIConfig;

// Initialize SPI
MCP_SPIHandle MCP_SPIInit(const MCP_SPIConfig* config);

// Deinitialize SPI
int MCP_SPIDeinit(MCP_SPIHandle spi);

// Transfer data over SPI
int MCP_SPITransfer(MCP_SPIHandle spi, const uint8_t* tx_data, uint8_t* rx_data, size_t size);

// --- PWM operations ---

// PWM handle type
typedef int MCP_PWMHandle;

// PWM configuration
typedef struct {
    int channel;
    int pin;
    int frequency;
    int resolution;
} MCP_PWMConfig;

// Initialize PWM
MCP_PWMHandle MCP_PWMInit(const MCP_PWMConfig* config);

// Deinitialize PWM
int MCP_PWMDeinit(MCP_PWMHandle pwm);

// Set PWM duty cycle
int MCP_PWMSetDuty(MCP_PWMHandle pwm, int duty);

// Set PWM frequency
int MCP_PWMSetFrequency(MCP_PWMHandle pwm, int frequency);

// --- ADC operations ---

// ADC handle type
typedef int MCP_ADCHandle;

// ADC configuration
typedef struct {
    int channel;
    int pin;
    int resolution;
    int attenuation;
} MCP_ADCConfig;

// Initialize ADC
MCP_ADCHandle MCP_ADCInit(const MCP_ADCConfig* config);

// Deinitialize ADC
int MCP_ADCDeinit(MCP_ADCHandle adc);

// Read ADC value
int MCP_ADCRead(MCP_ADCHandle adc);

// --- DAC operations ---

// DAC handle type
typedef int MCP_DACHandle;

// DAC configuration
typedef struct {
    int channel;
    int pin;
} MCP_DACConfig;

// Initialize DAC
MCP_DACHandle MCP_DACInit(const MCP_DACConfig* config);

// Deinitialize DAC
int MCP_DACDeinit(MCP_DACHandle dac);

// Write DAC value
int MCP_DACWrite(MCP_DACHandle dac, int value);

// --- System functions ---

// Reboot the system
void MCP_SystemReboot(void);

// Get system uptime in seconds
uint32_t MCP_SystemGetUptime(void);

// Get system temperature in Celsius
float MCP_SystemGetTemperature(void);

// Get system memory information
int MCP_SystemGetMemoryInfo(uint32_t* total, uint32_t* free);

// Get system information string
const char* MCP_SystemGetInfo(void);

/**
 * @brief Set a configuration parameter by name
 * 
 * This function allows updating any configuration parameter by name through JSON.
 * It supports all configuration parameters including WiFi, BLE, Ethernet, etc.
 * 
 * Example JSON: {"wifi": {"ssid": "NewNetwork", "password": "NewPassword", "enabled": true}}
 * 
 * @param json_config JSON string containing the configuration to update
 * @return int 0 on success, negative error code on failure
 */
int MCP_SetConfiguration(const char* json_config);

/**
 * @brief Get the current configuration as JSON
 * 
 * This function returns the full configuration as a JSON string.
 * 
 * @param buffer Buffer to store the JSON string
 * @param size Size of the buffer
 * @return int Length of the JSON string or negative error code on failure
 */
int MCP_GetConfiguration(char* buffer, size_t size);

#endif /* MCP_RPI_H */