/**
 * @file hal_rpi.c
 * @brief Hardware Abstraction Layer for Raspberry Pi (bare metal)
 */
#include "hal_rpi.h"
#include "../../logging.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

// Memory-mapped I/O
static volatile uint32_t* gpio_base = NULL;
static volatile uint32_t* uart0_base = NULL;
static volatile uint32_t* spi0_base = NULL;
static volatile uint32_t* i2c0_base = NULL;
static volatile uint32_t* pwm_base = NULL;
static volatile uint32_t* timer_base = NULL;

// File descriptor for /dev/mem
static int mem_fd = -1;

// GPIO registers offsets
#define GPFSEL_OFFSET(pin)      (pin / 10)
#define GPSET_OFFSET(pin)       (pin < 32 ? 0 : 1)
#define GPCLR_OFFSET(pin)       (pin < 32 ? 0 : 1)
#define GPLEV_OFFSET(pin)       (pin < 32 ? 0 : 1)

// UART registers offsets
#define UART_DR                 0x00 // Data Register
#define UART_FR                 0x18 // Flag Register
#define UART_IBRD               0x24 // Integer Baud Rate Divisor
#define UART_FBRD               0x28 // Fractional Baud Rate Divisor
#define UART_LCRH               0x2C // Line Control Register
#define UART_CR                 0x30 // Control Register
#define UART_IMSC               0x38 // Interrupt Mask Set/Clear Register
#define UART_ICR                0x44 // Interrupt Clear Register

// SPI registers offsets
#define SPI_CS                  0x00 // Control and Status
#define SPI_FIFO                0x04 // TX and RX FIFOs
#define SPI_CLK                 0x08 // Clock Divider
#define SPI_DLEN                0x0C // Data Length
#define SPI_LTOH                0x10 // LOSSI TOH
#define SPI_DC                  0x14 // DMA DREQ Controls

// I2C registers offsets
#define I2C_C                   0x00 // Control
#define I2C_S                   0x04 // Status
#define I2C_DLEN                0x08 // Data Length
#define I2C_A                   0x0C // Slave Address
#define I2C_FIFO                0x10 // Data FIFO
#define I2C_DIV                 0x14 // Clock Divider
#define I2C_DEL                 0x18 // Data Delay
#define I2C_CLKT                0x1C // Clock Stretch Timeout

// PWM registers offsets
#define PWM_CTL                 0x00 // Control
#define PWM_STA                 0x04 // Status
#define PWM_DMAC                0x08 // DMA Configuration
#define PWM_RNG1                0x10 // Channel 1 Range
#define PWM_DAT1                0x14 // Channel 1 Data
#define PWM_FIF1                0x18 // FIFO Input
#define PWM_RNG2                0x20 // Channel 2 Range
#define PWM_DAT2                0x24 // Channel 2 Data

// Timer registers offsets
#define TIMER_CS                0x00 // Control/Status
#define TIMER_CLO               0x04 // Counter Lower 32 bits
#define TIMER_CHI               0x08 // Counter Higher 32 bits
#define TIMER_C0                0x0C // Compare 0
#define TIMER_C1                0x10 // Compare 1
#define TIMER_C2                0x14 // Compare 2
#define TIMER_C3                0x18 // Compare 3

// Timer structures
#define MAX_TIMERS              8
typedef struct {
    bool active;
    uint32_t interval_ms;
    uint32_t next_expiry;
    void (*callback)(void*);
    void* context;
} RPiTimer;

static RPiTimer s_timers[MAX_TIMERS];
static bool s_timer_initialized = false;

// Memory mapping helpers
static void* mmio_map(uint64_t phys_addr, size_t size) {
    void *mapped_addr;
    
    // Open /dev/mem if not already open
    if (mem_fd < 0) {
        mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (mem_fd < 0) {
            log_error("Failed to open /dev/mem: %s", strerror(errno));
            return NULL;
        }
    }
    
    // Map physical memory to virtual address space
    mapped_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr);
    
    if (mapped_addr == MAP_FAILED) {
        log_error("Failed to map memory at 0x%lx: %s", phys_addr, strerror(errno));
        return NULL;
    }
    
    return mapped_addr;
}

static void mmio_unmap(void* addr, size_t size) {
    if (addr != NULL && addr != MAP_FAILED) {
        if (munmap(addr, size) < 0) {
            log_error("Failed to unmap memory: %s", strerror(errno));
        }
    }
}

/**
 * @brief Initialize the Raspberry Pi hardware
 */
int hal_rpi_init(void) {
    // Map peripheral registers
    gpio_base = (volatile uint32_t*)mmio_map(RPI_GPFSEL0, 0x1000);
    if (gpio_base == NULL) {
        log_error("Failed to map GPIO registers");
        return -1;
    }

    uart0_base = (volatile uint32_t*)mmio_map(RPI_UART0_BASE, 0x1000);
    if (uart0_base == NULL) {
        log_error("Failed to map UART0 registers");
        hal_rpi_deinit();
        return -2;
    }

    spi0_base = (volatile uint32_t*)mmio_map(RPI_SPI0_BASE, 0x1000);
    if (spi0_base == NULL) {
        log_error("Failed to map SPI0 registers");
        hal_rpi_deinit();
        return -3;
    }

    i2c0_base = (volatile uint32_t*)mmio_map(RPI_I2C0_BASE, 0x1000);
    if (i2c0_base == NULL) {
        log_error("Failed to map I2C0 registers");
        hal_rpi_deinit();
        return -4;
    }

    pwm_base = (volatile uint32_t*)mmio_map(RPI_PWM_BASE, 0x1000);
    if (pwm_base == NULL) {
        log_error("Failed to map PWM registers");
        hal_rpi_deinit();
        return -5;
    }

    timer_base = (volatile uint32_t*)mmio_map(RPI_TIMER_BASE, 0x1000);
    if (timer_base == NULL) {
        log_error("Failed to map Timer registers");
        hal_rpi_deinit();
        return -6;
    }

    // Initialize timer subsystem
    memset(s_timers, 0, sizeof(s_timers));
    s_timer_initialized = true;

    log_info("Raspberry Pi hardware initialized");
    return 0;
}

/**
 * @brief Clean up and shutdown hardware
 */
int hal_rpi_deinit(void) {
    // Unmap peripheral registers
    if (gpio_base != NULL) {
        mmio_unmap((void*)gpio_base, 0x1000);
        gpio_base = NULL;
    }

    if (uart0_base != NULL) {
        mmio_unmap((void*)uart0_base, 0x1000);
        uart0_base = NULL;
    }

    if (spi0_base != NULL) {
        mmio_unmap((void*)spi0_base, 0x1000);
        spi0_base = NULL;
    }

    if (i2c0_base != NULL) {
        mmio_unmap((void*)i2c0_base, 0x1000);
        i2c0_base = NULL;
    }

    if (pwm_base != NULL) {
        mmio_unmap((void*)pwm_base, 0x1000);
        pwm_base = NULL;
    }

    if (timer_base != NULL) {
        mmio_unmap((void*)timer_base, 0x1000);
        timer_base = NULL;
    }
    
    // Close /dev/mem file descriptor
    if (mem_fd >= 0) {
        close(mem_fd);
        mem_fd = -1;
    }

    s_timer_initialized = false;
    log_info("Raspberry Pi hardware deinitialized");
    return 0;
}

/**
 * @brief Configure GPIO pin function
 */
int hal_rpi_gpio_function(int pin, int function) {
    if (gpio_base == NULL || pin < 0 || pin > 53) {
        return -1;
    }

    // Calculate GPFSEL register and bit position
    int reg_offset = GPFSEL_OFFSET(pin);
    int bit_offset = (pin % 10) * 3;

    // Read current value
    uint32_t value = gpio_base[reg_offset];

    // Clear function bits
    value &= ~(7 << bit_offset);

    // Set new function
    value |= (function & 7) << bit_offset;

    // Write back
    gpio_base[reg_offset] = value;

    return 0;
}

/**
 * @brief Set GPIO pin output value
 */
int hal_rpi_gpio_write(int pin, int value) {
    if (gpio_base == NULL || pin < 0 || pin > 53) {
        return -1;
    }

    // Determine register offset based on pin
    int reg_offset = value ? 7 : 10; // GPSET0/GPCLR0
    if (pin >= 32) {
        reg_offset++; // GPSET1/GPCLR1
        pin -= 32;
    }

    // Set the pin
    gpio_base[reg_offset] = 1 << pin;

    return 0;
}

/**
 * @brief Read GPIO pin input value
 */
int hal_rpi_gpio_read(int pin) {
    if (gpio_base == NULL || pin < 0 || pin > 53) {
        return -1;
    }

    // Determine register offset based on pin
    int reg_offset = 13; // GPLEV0
    if (pin >= 32) {
        reg_offset++; // GPLEV1
        pin -= 32;
    }

    // Read the pin
    uint32_t value = gpio_base[reg_offset];
    return (value & (1 << pin)) ? 1 : 0;
}

/**
 * @brief Set GPIO pin pull-up/down state
 */
int hal_rpi_gpio_pull(int pin, int pud) {
    if (gpio_base == NULL || pin < 0 || pin > 53) {
        return -1;
    }

#if defined(RPI_MODEL_1) || defined(RPI_MODEL_2) || defined(RPI_MODEL_3)
    // Old pull-up/down method for Pi 1, 2, 3
    // Set the pull-up/down mode
    gpio_base[37] = pud & 3; // GPPUD
    
    // Wait 150 cycles
    for (volatile int i = 0; i < 150; i++);
    
    // Clock it in
    int reg_offset = 38; // GPPUDCLK0
    if (pin >= 32) {
        reg_offset++; // GPPUDCLK1
        pin -= 32;
    }
    
    gpio_base[reg_offset] = 1 << pin;
    
    // Wait 150 cycles
    for (volatile int i = 0; i < 150; i++);
    
    // Remove the clock
    gpio_base[37] = 0; // GPPUD
    gpio_base[reg_offset] = 0;
#else
    // New pull-up/down method for Pi 4 and newer
    int reg_offset = 57; // GPIO_PUP_PDN_CNTRL_REG0
    if (pin >= 16) {
        reg_offset++; // GPIO_PUP_PDN_CNTRL_REG1
        pin -= 16;
    }
    if (pin >= 32) {
        reg_offset++; // GPIO_PUP_PDN_CNTRL_REG2
        pin -= 16;
    }
    if (pin >= 48) {
        reg_offset++; // GPIO_PUP_PDN_CNTRL_REG3
        pin -= 16;
    }
    
    int bit_offset = pin * 2;
    uint32_t value = gpio_base[reg_offset];
    
    // Clear old value
    value &= ~(3 << bit_offset);
    
    // Set new value
    value |= (pud & 3) << bit_offset;
    
    // Write back
    gpio_base[reg_offset] = value;
#endif

    return 0;
}

/**
 * @brief Initialize UART with given baudrate
 */
int hal_rpi_uart_init(int uart_num, int baudrate) {
    volatile uint32_t* uart_base = NULL;
    
    // Get UART base address
    switch (uart_num) {
        case 0:
            uart_base = uart0_base;
            break;
#if defined(RPI_MODEL_4) || defined(RPI_MODEL_400) || defined(RPI_MODEL_5)
        case 2:
            uart_base = (volatile uint32_t*)mmio_map(RPI_UART2_BASE, 0x1000);
            break;
        case 3:
            uart_base = (volatile uint32_t*)mmio_map(RPI_UART3_BASE, 0x1000);
            break;
        case 4:
            uart_base = (volatile uint32_t*)mmio_map(RPI_UART4_BASE, 0x1000);
            break;
        case 5:
            uart_base = (volatile uint32_t*)mmio_map(RPI_UART5_BASE, 0x1000);
            break;
#endif
        default:
            return -1; // Invalid UART number
    }
    
    if (uart_base == NULL) {
        return -2; // UART not mapped
    }

    // Disable UART
    uart_base[UART_CR / 4] = 0;

    // Wait for end of transmission
    while (uart_base[UART_FR / 4] & (1 << 3)); // Wait until BUSY bit is clear

    // Calculate baud rate divisors
    // UART clock is 48MHz
    uint32_t baud_divisor = 48000000 / (16 * baudrate);
    uint32_t baud_integer = baud_divisor;
    uint32_t baud_fractional = (uint32_t)((48000000.0 / (16.0 * baudrate) - baud_integer) * 64.0 + 0.5);

    // Set baud rate
    uart_base[UART_IBRD / 4] = baud_integer;
    uart_base[UART_FBRD / 4] = baud_fractional;

    // Set line control (8 bits, 1 stop bit, no parity, FIFO enable)
    uart_base[UART_LCRH / 4] = (1 << 4) | (1 << 5) | (1 << 6);

    // Enable UART, transmit and receive
    uart_base[UART_CR / 4] = (1 << 0) | (1 << 8) | (1 << 9);

    // Clear interrupts
    uart_base[UART_ICR / 4] = 0x7FF;

    return 0;
}

/**
 * @brief Send a character over UART
 */
int hal_rpi_uart_putc(int uart_num, char c) {
    volatile uint32_t* uart_base = NULL;
    
    // Get UART base address
    switch (uart_num) {
        case 0:
            uart_base = uart0_base;
            break;
#if defined(RPI_MODEL_4) || defined(RPI_MODEL_400) || defined(RPI_MODEL_5)
        case 2:
            uart_base = (volatile uint32_t*)RPI_UART2_BASE;
            break;
        case 3:
            uart_base = (volatile uint32_t*)RPI_UART3_BASE;
            break;
        case 4:
            uart_base = (volatile uint32_t*)RPI_UART4_BASE;
            break;
        case 5:
            uart_base = (volatile uint32_t*)RPI_UART5_BASE;
            break;
#endif
        default:
            return -1; // Invalid UART number
    }
    
    if (uart_base == NULL) {
        return -2; // UART not mapped
    }
    
    // Wait until FIFO has space
    while (uart_base[UART_FR / 4] & (1 << 5)); // Wait until TXFF bit is clear
    
    // Write the character
    uart_base[UART_DR / 4] = c;
    
    return 0;
}

/**
 * @brief Receive a character from UART
 */
int hal_rpi_uart_getc(int uart_num) {
    volatile uint32_t* uart_base = NULL;
    
    // Get UART base address
    switch (uart_num) {
        case 0:
            uart_base = uart0_base;
            break;
#if defined(RPI_MODEL_4) || defined(RPI_MODEL_400) || defined(RPI_MODEL_5)
        case 2:
            uart_base = (volatile uint32_t*)RPI_UART2_BASE;
            break;
        case 3:
            uart_base = (volatile uint32_t*)RPI_UART3_BASE;
            break;
        case 4:
            uart_base = (volatile uint32_t*)RPI_UART4_BASE;
            break;
        case 5:
            uart_base = (volatile uint32_t*)RPI_UART5_BASE;
            break;
#endif
        default:
            return -1; // Invalid UART number
    }
    
    if (uart_base == NULL) {
        return -2; // UART not mapped
    }
    
    // Check if FIFO has data
    if (uart_base[UART_FR / 4] & (1 << 4)) {
        return -3; // No data available
    }
    
    // Read the character
    return uart_base[UART_DR / 4] & 0xFF;
}

/**
 * @brief Check if UART has received data available
 */
int hal_rpi_uart_available(int uart_num) {
    volatile uint32_t* uart_base = NULL;
    
    // Get UART base address
    switch (uart_num) {
        case 0:
            uart_base = uart0_base;
            break;
#if defined(RPI_MODEL_4) || defined(RPI_MODEL_400) || defined(RPI_MODEL_5)
        case 2:
            uart_base = (volatile uint32_t*)RPI_UART2_BASE;
            break;
        case 3:
            uart_base = (volatile uint32_t*)RPI_UART3_BASE;
            break;
        case 4:
            uart_base = (volatile uint32_t*)RPI_UART4_BASE;
            break;
        case 5:
            uart_base = (volatile uint32_t*)RPI_UART5_BASE;
            break;
#endif
        default:
            return -1; // Invalid UART number
    }
    
    if (uart_base == NULL) {
        return -2; // UART not mapped
    }
    
    // Check if FIFO has data
    return !(uart_base[UART_FR / 4] & (1 << 4));
}

/**
 * @brief Initialize SPI with given settings
 */
int hal_rpi_spi_init(int spi_num, int clock_div, int mode) {
    volatile uint32_t* spi_base = NULL;
    
    // Get SPI base address
    switch (spi_num) {
        case 0:
            spi_base = spi0_base;
            break;
        case 1:
            spi_base = (volatile uint32_t*)mmio_map(RPI_SPI1_BASE, 0x1000);
            break;
        case 2:
            spi_base = (volatile uint32_t*)mmio_map(RPI_SPI2_BASE, 0x1000);
            break;
        default:
            return -1; // Invalid SPI number
    }
    
    if (spi_base == NULL) {
        return -2; // SPI not mapped
    }
    
    // Clear control and status register
    spi_base[SPI_CS / 4] = 0;
    
    // Set clock divider
    spi_base[SPI_CLK / 4] = clock_div;
    
    // Set control and status register based on mode
    uint32_t cs = 0;
    
    // Configure based on SPI mode
    switch (mode) {
        case 0: // CPOL=0, CPHA=0
            // No flags needed
            break;
        case 1: // CPOL=0, CPHA=1
            cs |= (1 << 2); // CPHA
            break;
        case 2: // CPOL=1, CPHA=0
            cs |= (1 << 3); // CPOL
            break;
        case 3: // CPOL=1, CPHA=1
            cs |= (1 << 2) | (1 << 3); // CPHA and CPOL
            break;
        default:
            return -3; // Invalid mode
    }
    
    // Enable SPI
    cs |= (1 << 6) | (1 << 7) | (1 << 11); // CLEAR_RX, CLEAR_TX, and SPI_EN
    spi_base[SPI_CS / 4] = cs;
    
    return 0;
}

/**
 * @brief Transfer data over SPI
 */
int hal_rpi_spi_transfer(int spi_num, uint8_t tx_data) {
    volatile uint32_t* spi_base = NULL;
    
    // Get SPI base address
    switch (spi_num) {
        case 0:
            spi_base = spi0_base;
            break;
        case 1:
            spi_base = (volatile uint32_t*)RPI_SPI1_BASE;
            break;
        case 2:
            spi_base = (volatile uint32_t*)RPI_SPI2_BASE;
            break;
        default:
            return -1; // Invalid SPI number
    }
    
    if (spi_base == NULL) {
        return -2; // SPI not mapped
    }
    
    // Clear FIFO
    spi_base[SPI_CS / 4] |= (1 << 6) | (1 << 7); // CLEAR_RX and CLEAR_TX
    
    // Start transfer
    spi_base[SPI_CS / 4] |= (1 << 0); // TA
    
    // Wait until TX FIFO can accept data
    while (!(spi_base[SPI_CS / 4] & (1 << 1))); // Wait until TXD
    
    // Write data
    spi_base[SPI_FIFO / 4] = tx_data;
    
    // Wait until transfer complete
    while (spi_base[SPI_CS / 4] & (1 << 0)); // Wait until TA is clear
    
    // Read data
    return spi_base[SPI_FIFO / 4] & 0xFF;
}

/**
 * @brief Initialize I2C with given clock speed
 */
int hal_rpi_i2c_init(int i2c_num, int clock_speed) {
    volatile uint32_t* i2c_base = NULL;
    
    // Get I2C base address
    switch (i2c_num) {
        case 0:
            i2c_base = i2c0_base;
            break;
        case 1:
            i2c_base = (volatile uint32_t*)mmio_map(RPI_I2C1_BASE, 0x1000);
            break;
        case 2:
            i2c_base = (volatile uint32_t*)mmio_map(RPI_I2C2_BASE, 0x1000);
            break;
        default:
            return -1; // Invalid I2C number
    }
    
    if (i2c_base == NULL) {
        return -2; // I2C not mapped
    }
    
    // Disable I2C
    i2c_base[I2C_C / 4] = 0;
    
    // Set clock divider
    // I2C clock is 150MHz
    uint32_t divider = 150000000 / clock_speed;
    i2c_base[I2C_DIV / 4] = divider;
    
    // Enable I2C
    i2c_base[I2C_C / 4] = 1 << 15; // I2CEN
    
    return 0;
}

/**
 * @brief Write data to I2C device
 */
int hal_rpi_i2c_write(int i2c_num, uint8_t address, const uint8_t* data, size_t length) {
    volatile uint32_t* i2c_base = NULL;
    
    // Get I2C base address
    switch (i2c_num) {
        case 0:
            i2c_base = i2c0_base;
            break;
        case 1:
            i2c_base = (volatile uint32_t*)RPI_I2C1_BASE;
            break;
        case 2:
            i2c_base = (volatile uint32_t*)RPI_I2C2_BASE;
            break;
        default:
            return -1; // Invalid I2C number
    }
    
    if (i2c_base == NULL || data == NULL) {
        return -2; // I2C not mapped or invalid data
    }
    
    // Clear FIFO
    i2c_base[I2C_C / 4] |= (1 << 4) | (1 << 5); // CLEAR
    
    // Set slave address
    i2c_base[I2C_A / 4] = address;
    
    // Set data length
    i2c_base[I2C_DLEN / 4] = length;
    
    // Fill FIFO with data
    for (size_t i = 0; i < length; i++) {
        i2c_base[I2C_FIFO / 4] = data[i];
    }
    
    // Start write
    i2c_base[I2C_C / 4] = (1 << 15) | (1 << 7); // I2CEN and ST
    
    // Wait for transfer to complete
    while (!(i2c_base[I2C_S / 4] & (1 << 1))); // Wait until DONE
    
    // Check for errors
    if (i2c_base[I2C_S / 4] & (1 << 8)) {
        return -3; // Error (CLKT)
    }
    if (i2c_base[I2C_S / 4] & (1 << 9)) {
        return -4; // Error (ERR)
    }
    
    return 0;
}

/**
 * @brief Read data from I2C device
 */
int hal_rpi_i2c_read(int i2c_num, uint8_t address, uint8_t* data, size_t length) {
    volatile uint32_t* i2c_base = NULL;
    
    // Get I2C base address
    switch (i2c_num) {
        case 0:
            i2c_base = i2c0_base;
            break;
        case 1:
            i2c_base = (volatile uint32_t*)RPI_I2C1_BASE;
            break;
        case 2:
            i2c_base = (volatile uint32_t*)RPI_I2C2_BASE;
            break;
        default:
            return -1; // Invalid I2C number
    }
    
    if (i2c_base == NULL || data == NULL) {
        return -2; // I2C not mapped or invalid data
    }
    
    // Clear FIFO
    i2c_base[I2C_C / 4] |= (1 << 4) | (1 << 5); // CLEAR
    
    // Set slave address
    i2c_base[I2C_A / 4] = address;
    
    // Set data length
    i2c_base[I2C_DLEN / 4] = length;
    
    // Start read
    i2c_base[I2C_C / 4] = (1 << 15) | (1 << 7) | (1 << 0); // I2CEN, ST, and READ
    
    // Wait for transfer to complete
    while (!(i2c_base[I2C_S / 4] & (1 << 1))); // Wait until DONE
    
    // Check for errors
    if (i2c_base[I2C_S / 4] & (1 << 8)) {
        return -3; // Error (CLKT)
    }
    if (i2c_base[I2C_S / 4] & (1 << 9)) {
        return -4; // Error (ERR)
    }
    
    // Read data from FIFO
    for (size_t i = 0; i < length; i++) {
        data[i] = i2c_base[I2C_FIFO / 4] & 0xFF;
    }
    
    return 0;
}

/**
 * @brief Initialize PWM channel
 */
int hal_rpi_pwm_init(int channel, int range, int clock_div) {
    if (pwm_base == NULL || channel < 0 || channel > 1) {
        return -1;
    }
    
    // Set PWM control register
    uint32_t control = pwm_base[PWM_CTL / 4];
    
    // Clear channel bits
    if (channel == 0) {
        control &= ~0xFF; // Clear bits 0-7
    } else {
        control &= ~0xFF00; // Clear bits 8-15
    }
    
    // Set new values
    if (channel == 0) {
        control |= (1 << 0); // PWEN1 (enable)
        pwm_base[PWM_RNG1 / 4] = range;
        pwm_base[PWM_DAT1 / 4] = 0; // Start with 0% duty cycle
    } else {
        control |= (1 << 8); // PWEN2 (enable)
        pwm_base[PWM_RNG2 / 4] = range;
        pwm_base[PWM_DAT2 / 4] = 0; // Start with 0% duty cycle
    }
    
    // Write back control register
    pwm_base[PWM_CTL / 4] = control;
    
    return 0;
}

/**
 * @brief Set PWM duty cycle
 */
int hal_rpi_pwm_set_duty(int channel, int duty) {
    if (pwm_base == NULL || channel < 0 || channel > 1) {
        return -1;
    }
    
    // Set duty cycle
    if (channel == 0) {
        pwm_base[PWM_DAT1 / 4] = duty;
    } else {
        pwm_base[PWM_DAT2 / 4] = duty;
    }
    
    return 0;
}

/**
 * @brief Get current system time in milliseconds
 */
uint64_t hal_rpi_get_time_ms(void) {
    if (timer_base == NULL) {
        return 0;
    }
    
    // Read timer counter (64-bit value)
    uint32_t lo = timer_base[TIMER_CLO / 4];
    uint32_t hi = timer_base[TIMER_CHI / 4];
    
    // Combine into 64-bit value
    uint64_t counter = ((uint64_t)hi << 32) | lo;
    
    // Convert to milliseconds (timer runs at 1 MHz)
    return counter / 1000;
}

/**
 * @brief Delay execution for specified milliseconds
 */
void hal_rpi_delay_ms(uint32_t ms) {
    if (timer_base == NULL) {
        // Fallback to busy-wait if timer not initialized
        for (volatile uint32_t i = 0; i < ms * 10000; i++);
        return;
    }
    
    uint64_t target_time = hal_rpi_get_time_ms() + ms;
    while (hal_rpi_get_time_ms() < target_time);
}

/**
 * @brief Get CPU temperature
 */
float hal_rpi_get_temperature(void) {
    float temperature = 0.0f;
    FILE *thermal_file;
    char buffer[100];
    
    // Open the temperature file from the thermal zone
    thermal_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (thermal_file == NULL) {
        log_error("Failed to open thermal zone file");
        return -1.0f;
    }
    
    // Read temperature value (in millidegrees Celsius)
    if (fgets(buffer, sizeof(buffer), thermal_file) != NULL) {
        int temp_milliC = atoi(buffer);
        temperature = (float)temp_milliC / 1000.0f;
    } else {
        log_error("Failed to read temperature");
        temperature = -1.0f;
    }
    
    fclose(thermal_file);
    return temperature;
}

/**
 * @brief Get memory information
 */
int hal_rpi_get_memory_info(uint32_t* total, uint32_t* free) {
    if (total == NULL || free == NULL) {
        return -1;
    }

    FILE *meminfo;
    char line[128];
    uint64_t mem_total = 0;
    uint64_t mem_free = 0;
    uint64_t mem_available = 0;
    bool found_total = false;
    bool found_free = false;
    bool found_available = false;
    
    // Open /proc/meminfo to get real memory information
    meminfo = fopen("/proc/meminfo", "r");
    if (meminfo == NULL) {
        log_error("Failed to open /proc/meminfo");
        
        // Fallback to reasonable defaults if we can't read the file
        *total = 1024 * 1024 * 1024; // 1GB (default)
        *free = 512 * 1024 * 1024;   // 512MB
        return -2;
    }
    
    // Parse the file for memory information
    while (fgets(line, sizeof(line), meminfo) != NULL) {
        // Total memory
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %llu kB", &mem_total);
            found_total = true;
        }
        // Free memory
        else if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line, "MemFree: %llu kB", &mem_free);
            found_free = true;
        }
        // Available memory (better metric than just free)
        else if (strncmp(line, "MemAvailable:", 13) == 0) {
            sscanf(line, "MemAvailable: %llu kB", &mem_available);
            found_available = true;
        }
        
        // If we've found all the values we need, break the loop
        if (found_total && found_free && found_available) {
            break;
        }
    }
    
    fclose(meminfo);
    
    // Convert from kB to bytes
    *total = (uint32_t)(mem_total * 1024);
    
    // Prefer MemAvailable over MemFree if available
    if (found_available) {
        *free = (uint32_t)(mem_available * 1024);
    } else if (found_free) {
        *free = (uint32_t)(mem_free * 1024);
    } else {
        // Fallback if we couldn't read the values
        *free = *total / 2;
    }
    
    return 0;
}

/**
 * @brief Process timers and call callbacks
 * (This would be called periodically by the main loop)
 */
static void process_timers(void) {
    if (!s_timer_initialized) {
        return;
    }

    uint64_t current_time = hal_rpi_get_time_ms();

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (s_timers[i].active && current_time >= s_timers[i].next_expiry) {
            // Update next expiry time
            s_timers[i].next_expiry = current_time + s_timers[i].interval_ms;

            // Call the callback
            if (s_timers[i].callback != NULL) {
                s_timers[i].callback(s_timers[i].context);
            }
        }
    }
}

/**
 * @brief Initialize timer with callback
 */
int hal_rpi_timer_init(uint32_t interval_ms, void (*callback)(void*), void* context) {
    if (!s_timer_initialized || callback == NULL || interval_ms == 0) {
        return -1;
    }

    // Find a free timer slot
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!s_timers[i].active) {
            s_timers[i].active = true;
            s_timers[i].interval_ms = interval_ms;
            s_timers[i].next_expiry = hal_rpi_get_time_ms() + interval_ms;
            s_timers[i].callback = callback;
            s_timers[i].context = context;
            return i; // Return timer ID
        }
    }

    return -2; // No free timer slots
}

/**
 * @brief Stop and remove timer
 */
int hal_rpi_timer_stop(int timer_id) {
    if (!s_timer_initialized || timer_id < 0 || timer_id >= MAX_TIMERS || !s_timers[timer_id].active) {
        return -1;
    }

    s_timers[timer_id].active = false;
    s_timers[timer_id].callback = NULL;
    s_timers[timer_id].context = NULL;

    return 0;
}

/**
 * @brief Map a physical memory address to be accessible from user space
 */
void* hal_rpi_mmap(uint64_t phys_addr, size_t size) {
    return mmio_map(phys_addr, size);
}

/**
 * @brief Unmap a previously mapped memory region
 */
int hal_rpi_munmap(void* addr, size_t size) {
    if (addr == NULL) {
        return -1;
    }

    mmio_unmap(addr, size);
    return 0;
}

// Function to be called periodically from the main loop to update internal state
void hal_rpi_tick(void) {
    process_timers();
}