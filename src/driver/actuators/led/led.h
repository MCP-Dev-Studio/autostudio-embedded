#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief LED type enumeration
 */
typedef enum {
    LED_TYPE_SIMPLE,     // Simple on/off LED
    LED_TYPE_PWM,        // PWM-controlled LED
    LED_TYPE_RGB,        // RGB LED
    LED_TYPE_RGBW,       // RGBW LED
    LED_TYPE_ADDRESSABLE // Addressable LED strip (WS2812, etc.)
} LEDType;

/**
 * @brief LED pattern type
 */
typedef enum {
    LED_PATTERN_NONE,        // No pattern (steady)
    LED_PATTERN_BLINK,       // Simple blink
    LED_PATTERN_PULSE,       // Pulse (fade in/out)
    LED_PATTERN_RAINBOW,     // Rainbow effect (RGB/RGBW)
    LED_PATTERN_CHASE,       // Chase effect (addressable)
    LED_PATTERN_FLASH,       // Fast flash
    LED_PATTERN_BREATHE,     // Breathing effect
    LED_PATTERN_CUSTOM       // Custom pattern
} LEDPattern;

/**
 * @brief RGB color structure
 */
typedef struct {
    uint8_t r;  // Red (0-255)
    uint8_t g;  // Green (0-255)
    uint8_t b;  // Blue (0-255)
    uint8_t w;  // White (0-255, for RGBW LEDs)
} RGBColor;

/**
 * @brief LED configuration
 */
typedef struct {
    LEDType type;             // LED type
    uint8_t pin;              // Control pin for simple/PWM LED
    uint8_t redPin;           // Red pin for RGB/RGBW LED
    uint8_t greenPin;         // Green pin for RGB/RGBW LED
    uint8_t bluePin;          // Blue pin for RGB/RGBW LED
    uint8_t whitePin;         // White pin for RGBW LED
    uint8_t dataPin;          // Data pin for addressable LEDs
    uint16_t count;           // Number of LEDs in addressable strip
    bool activeHigh;          // Active high (true) or active low (false)
    bool commonAnode;         // Common anode RGB LED (true) or common cathode (false)
    uint16_t pwmFrequency;    // PWM frequency in Hz
    uint8_t pwmResolution;    // PWM resolution in bits
    uint8_t initialBrightness; // Initial brightness (0-255)
    RGBColor initialColor;    // Initial color for RGB/RGBW LEDs
} LEDConfig;

/**
 * @brief LED pattern configuration
 */
typedef struct {
    LEDPattern pattern;        // Pattern type
    uint32_t speed;            // Pattern speed in milliseconds
    uint8_t intensity;         // Pattern intensity (0-255)
    uint32_t duration;         // Pattern duration in milliseconds (0 for infinite)
    uint8_t repeat;            // Repeat count (0 for infinite)
    RGBColor colors[8];        // Colors to use in pattern
    uint8_t colorCount;        // Number of colors to use
} LEDPatternConfig;

/**
 * @brief LED initialization
 * 
 * @param config LED configuration
 * @return int 0 on success, negative error code on failure
 */
int LED_Init(const LEDConfig* config);

/**
 * @brief LED deinitialization
 * 
 * @return int 0 on success, negative error code on failure
 */
int LED_Deinit(void);

/**
 * @brief Turn LED on
 * 
 * @return int 0 on success, negative error code on failure
 */
int LED_On(void);

/**
 * @brief Turn LED off
 * 
 * @return int 0 on success, negative error code on failure
 */
int LED_Off(void);

/**
 * @brief Toggle LED state
 * 
 * @return int 0 on success, negative error code on failure
 */
int LED_Toggle(void);

/**
 * @brief Set LED state
 * 
 * @param state Desired state (true = on, false = off)
 * @return int 0 on success, negative error code on failure
 */
int LED_SetState(bool state);

/**
 * @brief Get LED state
 * 
 * @return bool True if LED is on, false if off
 */
bool LED_GetState(void);

/**
 * @brief Set LED brightness
 * 
 * @param brightness Brightness level (0-255)
 * @return int 0 on success, negative error code on failure
 */
int LED_SetBrightness(uint8_t brightness);

/**
 * @brief Get LED brightness
 * 
 * @return uint8_t Brightness level (0-255)
 */
uint8_t LED_GetBrightness(void);

/**
 * @brief Set RGB/RGBW LED color
 * 
 * @param color RGB/RGBW color
 * @return int 0 on success, negative error code on failure
 */
int LED_SetColor(const RGBColor* color);

/**
 * @brief Get RGB/RGBW LED color
 * 
 * @param color Pointer to store color
 * @return int 0 on success, negative error code on failure
 */
int LED_GetColor(RGBColor* color);

/**
 * @brief Set LED pattern
 * 
 * @param pattern Pattern configuration
 * @return int 0 on success, negative error code on failure
 */
int LED_SetPattern(const LEDPatternConfig* pattern);

/**
 * @brief Stop LED pattern
 * 
 * @return int 0 on success, negative error code on failure
 */
int LED_StopPattern(void);

/**
 * @brief Process LED (call in main loop)
 * 
 * @return int 0 on success, negative error code on failure
 */
int LED_Process(void);

/**
 * @brief Set specific LED in addressable strip
 * 
 * @param index LED index
 * @param color RGB/RGBW color
 * @return int 0 on success, negative error code on failure
 */
int LED_SetPixel(uint16_t index, const RGBColor* color);

/**
 * @brief Update addressable LED strip (send data to LEDs)
 * 
 * @return int 0 on success, negative error code on failure
 */
int LED_Update(void);

/**
 * @brief Fill all addressable LEDs with same color
 * 
 * @param color RGB/RGBW color
 * @return int 0 on success, negative error code on failure
 */
int LED_Fill(const RGBColor* color);

/**
 * @brief Create RGB color
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return RGBColor RGB color
 */
RGBColor LED_RGB(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Create RGBW color
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param w White component (0-255)
 * @return RGBColor RGBW color
 */
RGBColor LED_RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);

#endif /* LED_H */