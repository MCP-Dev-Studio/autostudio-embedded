/**
 * @file hal_rpi.h
 * @brief Hardware Abstraction Layer for Raspberry Pi (bare metal)
 */
#ifndef HAL_RPI_H
#define HAL_RPI_H

#include <stdint.h>
#include <stdbool.h>
#include "../../mcp_os_core.h"

// Detect Raspberry Pi model
#if defined(RPI_MODEL_1)
    #define RPI_PERI_BASE       0x20000000  // Pi 1 Model A, A+, B, B+
    #define RPI_PERI_SIZE       0x01000000  // 16MB
    #define RPI_GPFSEL0         (RPI_PERI_BASE + 0x00200000)
    #define RPI_GPCLR0          (RPI_PERI_BASE + 0x00200028)
    #define RPI_GPSET0          (RPI_PERI_BASE + 0x0020001C)
    #define RPI_GPLEV0          (RPI_PERI_BASE + 0x00200034)
    #define RPI_GPEDS0          (RPI_PERI_BASE + 0x00200040)
    #define RPI_GPREN0          (RPI_PERI_BASE + 0x0020004C)
    #define RPI_GPFEN0          (RPI_PERI_BASE + 0x00200058)
    #define RPI_GPHEN0          (RPI_PERI_BASE + 0x00200064)
    #define RPI_GPLEN0          (RPI_PERI_BASE + 0x00200070)
    #define RPI_GPAREN0         (RPI_PERI_BASE + 0x0020007C)
    #define RPI_GPAFEN0         (RPI_PERI_BASE + 0x00200088)
    #define RPI_GPPUD           (RPI_PERI_BASE + 0x00200094)
    #define RPI_GPPUDCLK0       (RPI_PERI_BASE + 0x00200098)
#elif defined(RPI_MODEL_2) || defined(RPI_MODEL_3)
    #define RPI_PERI_BASE       0x3F000000  // Pi 2 Model B, Pi 3 Model B, B+
    #define RPI_PERI_SIZE       0x01000000  // 16MB
    #define RPI_GPFSEL0         (RPI_PERI_BASE + 0x00200000)
    #define RPI_GPCLR0          (RPI_PERI_BASE + 0x00200028)
    #define RPI_GPSET0          (RPI_PERI_BASE + 0x0020001C)
    #define RPI_GPLEV0          (RPI_PERI_BASE + 0x00200034)
    #define RPI_GPEDS0          (RPI_PERI_BASE + 0x00200040)
    #define RPI_GPREN0          (RPI_PERI_BASE + 0x0020004C)
    #define RPI_GPFEN0          (RPI_PERI_BASE + 0x00200058)
    #define RPI_GPHEN0          (RPI_PERI_BASE + 0x00200064)
    #define RPI_GPLEN0          (RPI_PERI_BASE + 0x00200070)
    #define RPI_GPAREN0         (RPI_PERI_BASE + 0x0020007C)
    #define RPI_GPAFEN0         (RPI_PERI_BASE + 0x00200088)
    #define RPI_GPPUD           (RPI_PERI_BASE + 0x00200094)
    #define RPI_GPPUDCLK0       (RPI_PERI_BASE + 0x00200098)
#elif defined(RPI_MODEL_4) || defined(RPI_MODEL_400)
    #define RPI_PERI_BASE       0xFE000000  // Pi 4 Model B, Pi 400
    #define RPI_PERI_SIZE       0x01000000  // 16MB
    #define RPI_GPFSEL0         (RPI_PERI_BASE + 0x00200000)
    #define RPI_GPCLR0          (RPI_PERI_BASE + 0x00200028)
    #define RPI_GPSET0          (RPI_PERI_BASE + 0x0020001C)
    #define RPI_GPLEV0          (RPI_PERI_BASE + 0x00200034)
    #define RPI_GPEDS0          (RPI_PERI_BASE + 0x00200040)
    #define RPI_GPREN0          (RPI_PERI_BASE + 0x0020004C)
    #define RPI_GPFEN0          (RPI_PERI_BASE + 0x00200058)
    #define RPI_GPHEN0          (RPI_PERI_BASE + 0x00200064)
    #define RPI_GPLEN0          (RPI_PERI_BASE + 0x00200070)
    #define RPI_GPAREN0         (RPI_PERI_BASE + 0x0020007C)
    #define RPI_GPAFEN0         (RPI_PERI_BASE + 0x00200088)
    #define RPI_GPIO_PUP_PDN_CNTRL_REG0 (RPI_PERI_BASE + 0x002000E4)
#elif defined(RPI_MODEL_5)
    #define RPI_PERI_BASE       0x1000000000ULL  // Pi 5
    #define RPI_PERI_SIZE       0x1000000        // 16MB
    #define RPI_GPFSEL0         (RPI_PERI_BASE + 0x00200000)
    #define RPI_GPCLR0          (RPI_PERI_BASE + 0x00200028)
    #define RPI_GPSET0          (RPI_PERI_BASE + 0x0020001C)
    #define RPI_GPLEV0          (RPI_PERI_BASE + 0x00200034)
    #define RPI_GPEDS0          (RPI_PERI_BASE + 0x00200040)
    #define RPI_GPREN0          (RPI_PERI_BASE + 0x0020004C)
    #define RPI_GPFEN0          (RPI_PERI_BASE + 0x00200058)
    #define RPI_GPHEN0          (RPI_PERI_BASE + 0x00200064)
    #define RPI_GPLEN0          (RPI_PERI_BASE + 0x00200070)
    #define RPI_GPAREN0         (RPI_PERI_BASE + 0x0020007C)
    #define RPI_GPAFEN0         (RPI_PERI_BASE + 0x00200088)
    #define RPI_GPIO_PUP_PDN_CNTRL_REG0 (RPI_PERI_BASE + 0x002000E4)
#else
    #error "Unknown Raspberry Pi model"
#endif

// UART addresses
#if defined(RPI_MODEL_1) || defined(RPI_MODEL_2) || defined(RPI_MODEL_3)
    #define RPI_UART0_BASE      (RPI_PERI_BASE + 0x00201000)
#elif defined(RPI_MODEL_4) || defined(RPI_MODEL_400) || defined(RPI_MODEL_5)
    #define RPI_UART0_BASE      (RPI_PERI_BASE + 0x00201000)
    #define RPI_UART2_BASE      (RPI_PERI_BASE + 0x00201400)
    #define RPI_UART3_BASE      (RPI_PERI_BASE + 0x00201600)
    #define RPI_UART4_BASE      (RPI_PERI_BASE + 0x00201800)
    #define RPI_UART5_BASE      (RPI_PERI_BASE + 0x00201A00)
#endif

// SPI addresses
#define RPI_SPI0_BASE           (RPI_PERI_BASE + 0x00204000)
#define RPI_SPI1_BASE           (RPI_PERI_BASE + 0x00204600)
#define RPI_SPI2_BASE           (RPI_PERI_BASE + 0x00204800)

// I2C addresses
#define RPI_I2C0_BASE           (RPI_PERI_BASE + 0x00205000)
#define RPI_I2C1_BASE           (RPI_PERI_BASE + 0x00804000)
#define RPI_I2C2_BASE           (RPI_PERI_BASE + 0x00805000)

// PWM addresses
#define RPI_PWM_BASE            (RPI_PERI_BASE + 0x0020C000)

// Timer addresses
#define RPI_TIMER_BASE          (RPI_PERI_BASE + 0x00003000)

// GPIO functions
#define RPI_GPIO_FUNC_INPUT     0
#define RPI_GPIO_FUNC_OUTPUT    1
#define RPI_GPIO_FUNC_ALT0      4
#define RPI_GPIO_FUNC_ALT1      5
#define RPI_GPIO_FUNC_ALT2      6
#define RPI_GPIO_FUNC_ALT3      7
#define RPI_GPIO_FUNC_ALT4      3
#define RPI_GPIO_FUNC_ALT5      2

// Pin pull-up/down states
#define RPI_GPIO_PULL_NONE      0
#define RPI_GPIO_PULL_DOWN      1
#define RPI_GPIO_PULL_UP        2

/**
 * @brief Initialize the Raspberry Pi hardware
 * 
 * @return int 0 on success, negative on failure
 */
int hal_rpi_init(void);

/**
 * @brief Clean up and shutdown hardware
 * 
 * @return int 0 on success, negative on failure
 */
int hal_rpi_deinit(void);

/**
 * @brief Configure GPIO pin function
 * 
 * @param pin GPIO pin number (0-53)
 * @param function Function selection (INPUT, OUTPUT, ALT0-ALT5)
 * @return int 0 on success, negative on failure
 */
int hal_rpi_gpio_function(int pin, int function);

/**
 * @brief Set GPIO pin output value
 * 
 * @param pin GPIO pin number (0-53)
 * @param value 0 for low, non-zero for high
 * @return int 0 on success, negative on failure
 */
int hal_rpi_gpio_write(int pin, int value);

/**
 * @brief Read GPIO pin input value
 * 
 * @param pin GPIO pin number (0-53)
 * @return int Pin value (0 or 1), negative on failure
 */
int hal_rpi_gpio_read(int pin);

/**
 * @brief Set GPIO pin pull-up/down state
 * 
 * @param pin GPIO pin number (0-53)
 * @param pud Pull-up/down state (NONE, DOWN, UP)
 * @return int 0 on success, negative on failure
 */
int hal_rpi_gpio_pull(int pin, int pud);

/**
 * @brief Initialize UART with given baudrate
 * 
 * @param uart_num UART number (0, 2-5 depending on Pi model)
 * @param baudrate Baudrate (e.g., 115200)
 * @return int 0 on success, negative on failure
 */
int hal_rpi_uart_init(int uart_num, int baudrate);

/**
 * @brief Send a character over UART
 * 
 * @param uart_num UART number
 * @param c Character to send
 * @return int 0 on success, negative on failure
 */
int hal_rpi_uart_putc(int uart_num, char c);

/**
 * @brief Receive a character from UART
 * 
 * @param uart_num UART number
 * @return int Character received, negative on failure
 */
int hal_rpi_uart_getc(int uart_num);

/**
 * @brief Check if UART has received data available
 * 
 * @param uart_num UART number
 * @return int 1 if data available, 0 if not, negative on failure
 */
int hal_rpi_uart_available(int uart_num);

/**
 * @brief Initialize SPI with given settings
 * 
 * @param spi_num SPI number (0-2)
 * @param clock_div Clock divider (determines speed)
 * @param mode SPI mode (0-3)
 * @return int 0 on success, negative on failure
 */
int hal_rpi_spi_init(int spi_num, int clock_div, int mode);

/**
 * @brief Transfer data over SPI
 * 
 * @param spi_num SPI number
 * @param tx_data Data to transmit
 * @return int Received data, negative on failure
 */
int hal_rpi_spi_transfer(int spi_num, uint8_t tx_data);

/**
 * @brief Initialize I2C with given clock speed
 * 
 * @param i2c_num I2C number (0-2)
 * @param clock_speed Clock speed in Hz
 * @return int 0 on success, negative on failure
 */
int hal_rpi_i2c_init(int i2c_num, int clock_speed);

/**
 * @brief Write data to I2C device
 * 
 * @param i2c_num I2C number
 * @param address 7-bit device address
 * @param data Data buffer to write
 * @param length Length of data to write
 * @return int 0 on success, negative on failure
 */
int hal_rpi_i2c_write(int i2c_num, uint8_t address, const uint8_t* data, size_t length);

/**
 * @brief Read data from I2C device
 * 
 * @param i2c_num I2C number
 * @param address 7-bit device address
 * @param data Buffer to store read data
 * @param length Number of bytes to read
 * @return int 0 on success, negative on failure
 */
int hal_rpi_i2c_read(int i2c_num, uint8_t address, uint8_t* data, size_t length);

/**
 * @brief Initialize PWM channel
 * 
 * @param channel PWM channel (0-1)
 * @param range PWM range value
 * @param clock_div Clock divider
 * @return int 0 on success, negative on failure
 */
int hal_rpi_pwm_init(int channel, int range, int clock_div);

/**
 * @brief Set PWM duty cycle
 * 
 * @param channel PWM channel (0-1)
 * @param duty Duty cycle value
 * @return int 0 on success, negative on failure
 */
int hal_rpi_pwm_set_duty(int channel, int duty);

/**
 * @brief Get current system time in milliseconds
 * 
 * @return uint64_t Current time in milliseconds
 */
uint64_t hal_rpi_get_time_ms(void);

/**
 * @brief Delay execution for specified milliseconds
 * 
 * @param ms Milliseconds to delay
 */
void hal_rpi_delay_ms(uint32_t ms);

/**
 * @brief Get CPU temperature
 * 
 * @return float Temperature in Celsius, negative on error
 */
float hal_rpi_get_temperature(void);

/**
 * @brief Get memory information
 * 
 * @param total Pointer to store total memory (in bytes)
 * @param free Pointer to store free memory (in bytes)
 * @return int 0 on success, negative on failure
 */
int hal_rpi_get_memory_info(uint32_t* total, uint32_t* free);

/**
 * @brief Initialize timer with callback
 * 
 * @param interval_ms Interval in milliseconds
 * @param callback Function to call when timer expires
 * @param context User context to pass to callback
 * @return int Timer ID on success, negative on failure
 */
int hal_rpi_timer_init(uint32_t interval_ms, void (*callback)(void*), void* context);

/**
 * @brief Stop and remove timer
 * 
 * @param timer_id Timer ID returned by hal_rpi_timer_init
 * @return int 0 on success, negative on failure
 */
int hal_rpi_timer_stop(int timer_id);

/**
 * @brief Map a physical memory address to be accessible from user space
 * 
 * @param phys_addr Physical address to map
 * @param size Size of memory region to map
 * @return void* Mapped virtual address, NULL on failure
 */
void* hal_rpi_mmap(uint64_t phys_addr, size_t size);

/**
 * @brief Unmap a previously mapped memory region
 * 
 * @param addr Virtual address returned by hal_rpi_mmap
 * @param size Size of mapped region
 * @return int 0 on success, negative on failure
 */
int hal_rpi_munmap(void* addr, size_t size);

#endif /* HAL_RPI_H */