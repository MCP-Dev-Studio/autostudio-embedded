#include "../../src/core/device/driver_bridge.h"
#include "../../src/driver/actuators/led/led.h"
#include "../../src/driver/sensors/temperature/ds18b20.h"
#include <stdio.h>
#include <string.h>

// Example of using the driver bridge system

int main() {
    printf("MCP Driver Bridge Example\n");
    
    // 1. Initialize the driver bridge system
    printf("Initializing driver bridge system...\n");
    if (MCP_DriverBridgeInit() != 0) {
        printf("Failed to initialize driver bridge\n");
        return -1;
    }
    
    // 2. Initialize the driver bridge tool handlers
    printf("Initializing driver bridge tool handlers...\n");
    if (MCP_DriverBridgeToolsInit() != 0) {
        printf("Failed to initialize driver bridge tools\n");
        return -1;
    }
    
    // 3. Register an LED driver
    printf("Registering RGB LED driver...\n");
    if (MCP_CreateLEDDriver("led1", "RGB Status LED", LED_TYPE_RGB, 13) != 0) {
        printf("Failed to register LED driver\n");
        return -1;
    }
    
    // 4. Register a temperature sensor driver
    printf("Registering DS18B20 temperature sensor...\n");
    if (MCP_CreateDS18B20Driver("tempSensor1", "Water Temperature", 5) != 0) {
        printf("Failed to register temperature sensor driver\n");
        return -1;
    }
    
    // 5. Example of manually registering a driver with more control
    printf("Manually registering a custom LED driver...\n");
    
    // Create config schema
    const char* ledConfigSchema = 
        "{\"type\":\"object\","
        "\"properties\":{"
        "\"pin\":{\"type\":\"number\",\"description\":\"GPIO pin number\"},"
        "\"activeHigh\":{\"type\":\"boolean\",\"description\":\"Active high (true) or active low (false)\"},"
        "\"initialState\":{\"type\":\"boolean\",\"description\":\"Initial state (on/off)\"}"
        "},"
        "\"required\":[\"pin\"]"
        "}";
    
    // Register with bridge
    if (MCP_DriverBridgeRegister("led2", "Status Indicator", MCP_DRIVER_TYPE_ACTUATOR, 
                                DEVICE_TYPE_LED_SIMPLE, ledConfigSchema) != 0) {
        printf("Failed to manually register LED driver\n");
        return -1;
    }
    
    // 6. Map specific functions for the LED driver
    printf("Mapping LED driver functions...\n");
    MCP_DriverBridgeMapFunction("led2", "init", LED_Init);
    MCP_DriverBridgeMapFunction("led2", "deinit", LED_Deinit);
    MCP_DriverBridgeMapFunction("led2", "on", LED_On);
    MCP_DriverBridgeMapFunction("led2", "off", LED_Off);
    MCP_DriverBridgeMapFunction("led2", "toggle", LED_Toggle);
    
    // 7. Simulate MCP calls through the bridge
    printf("\nSimulating MCP operations:\n");
    
    // Simulate initializing the temperature sensor
    printf("- Initializing temperature sensor...\n");
    DS18B20Config sensorConfig = {
        .pin = 5,
        .address = 0,
        .resolution = DS18B20_RESOLUTION_12BIT,
        .useCRC = true,
        .conversionTimeout = 750
    };
    
    // Direct call to native driver function
    DS18B20_Init(&sensorConfig);
    
    // Simulate reading the temperature
    printf("- Reading temperature...\n");
    float temperature;
    if (DS18B20_ReadTemperature(&temperature) == 0) {
        printf("  Temperature: %.2f°C\n", temperature);
    } else {
        printf("  Failed to read temperature\n");
    }
    
    // Simulate configuring an RGB LED
    printf("- Configuring RGB LED...\n");
    LEDConfig ledConfig = {
        .type = LED_TYPE_RGB,
        .redPin = 13,
        .greenPin = 14,
        .bluePin = 15,
        .activeHigh = true,
        .initialBrightness = 128
    };
    
    // Direct call to native driver function
    LED_Init(&ledConfig);
    
    // Set LED color
    printf("- Setting LED color to red...\n");
    RGBColor redColor = LED_RGB(255, 0, 0);
    LED_SetColor(&redColor);
    
    // Turn on LED
    printf("- Turning on LED...\n");
    LED_On();
    
    // 8. Clean up
    printf("\nCleaning up...\n");
    LED_Deinit();
    DS18B20_Deinit();
    
    // 9. Unregister drivers
    printf("Unregistering drivers...\n");
    MCP_DriverBridgeUnregister("led1");
    MCP_DriverBridgeUnregister("led2");
    MCP_DriverBridgeUnregister("tempSensor1");
    
    printf("\nDriver bridge example completed successfully\n");
    return 0;
}