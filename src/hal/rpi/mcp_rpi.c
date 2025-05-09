/**
 * @file mcp_rpi.c
 * @brief MCP implementation for bare metal Raspberry Pi
 */
#include "mcp_rpi.h"
#include "hal_rpi.h"
#include "../../logging.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

// Memory management statistics
static size_t s_total_allocated = 0;
static size_t s_peak_allocated = 0;
static bool s_platform_initialized = false;

// --- Synchronization primitives ---

// Mutex type
typedef struct {
    bool locked;
    MCP_TaskHandle owner;
} RPi_Mutex;

// Semaphore type
typedef struct {
    uint32_t count;
    uint32_t max_count;
} RPi_Semaphore;

// Timer type
typedef struct {
    char name[32];
    uint32_t period_ms;
    bool auto_reload;
    void* arg;
    MCP_TimerCallback callback;
    int hal_timer_id;
    bool active;
} RPi_Timer;

// Event group type
typedef struct {
    MCP_EventBits bits;
} RPi_EventGroup;

// Queue type
typedef struct {
    void* buffer;
    size_t item_size;
    size_t max_items;
    size_t item_count;
    size_t read_index;
    size_t write_index;
    RPi_Mutex mutex;
    RPi_Semaphore not_empty;
    RPi_Semaphore not_full;
} RPi_Queue;

// File handle type
typedef struct {
    // Placeholder for file operations (would need a filesystem implementation)
    int dummy;
} RPi_File;

// --- Resource pools ---
#define MAX_MUTEXES      16
#define MAX_SEMAPHORES   16
#define MAX_TIMERS       8
#define MAX_EVENT_GROUPS 8
#define MAX_QUEUES       8
#define MAX_FILES        8
#define MAX_UARTS        6
#define MAX_I2C          3
#define MAX_SPI          3
#define MAX_PWM          2
#define MAX_ADC          8
#define MAX_DAC          2

static RPi_Mutex s_mutexes[MAX_MUTEXES];
static RPi_Semaphore s_semaphores[MAX_SEMAPHORES];
static RPi_Timer s_timers[MAX_TIMERS];
static RPi_EventGroup s_event_groups[MAX_EVENT_GROUPS];
static RPi_Queue s_queues[MAX_QUEUES];

static bool s_mutex_used[MAX_MUTEXES];
static bool s_semaphore_used[MAX_SEMAPHORES];
static bool s_timer_used[MAX_TIMERS];
static bool s_event_group_used[MAX_EVENT_GROUPS];
static bool s_queue_used[MAX_QUEUES];

// --- Forward declarations ---
static void timer_callback_wrapper(void* arg);

/**
 * @brief Platform-specific MCP initialization
 * 
 * This function only initializes the hardware and resource pools,
 * not the higher-level subsystems (which are initialized in MCP_SystemInit)
 */
int MCP_PlatformInit(void) {
    if (s_platform_initialized) {
        log_warn("Platform already initialized");
        return -1; // Already initialized
    }
    
    // Initialize HAL
    int result = hal_rpi_init();
    if (result != 0) {
        log_error("Failed to initialize Raspberry Pi HAL: %d", result);
        return result;
    }
    
    // Initialize resource pools
    memset(s_mutexes, 0, sizeof(s_mutexes));
    memset(s_semaphores, 0, sizeof(s_semaphores));
    memset(s_timers, 0, sizeof(s_timers));
    memset(s_event_groups, 0, sizeof(s_event_groups));
    memset(s_queues, 0, sizeof(s_queues));
    
    memset(s_mutex_used, 0, sizeof(s_mutex_used));
    memset(s_semaphore_used, 0, sizeof(s_semaphore_used));
    memset(s_timer_used, 0, sizeof(s_timer_used));
    memset(s_event_group_used, 0, sizeof(s_event_group_used));
    memset(s_queue_used, 0, sizeof(s_queue_used));
    
    // Initialize persistent configuration
    memset(&s_persistent_config, 0, sizeof(s_persistent_config));
    
    // Set default configuration values
    strncpy(s_persistent_config.device_name, "Raspberry Pi", sizeof(s_persistent_config.device_name) - 1);
    strncpy(s_persistent_config.firmware_version, "1.0.0", sizeof(s_persistent_config.firmware_version) - 1);
    s_persistent_config.debug_enabled = false;
    
    s_persistent_config.server_enabled = true;
    s_persistent_config.server_port = 8080;
    s_persistent_config.auto_start_server = true;
    
    s_persistent_config.wifi_enabled = false;
    s_persistent_config.wifi_auto_connect = false;
    
    s_persistent_config.wifi_ap_enabled = false;
    s_persistent_config.wifi_ap_channel = 6;
    
    s_persistent_config.ble_enabled = false;
    strncpy(s_persistent_config.ble_device_name, "RPi_MCP", sizeof(s_persistent_config.ble_device_name) - 1);
    s_persistent_config.ble_auto_advertise = false;
    
    s_persistent_config.ethernet_enabled = true;
    strncpy(s_persistent_config.ethernet_interface, "eth0", sizeof(s_persistent_config.ethernet_interface) - 1);
    s_persistent_config.ethernet_dhcp = true;
    
    s_persistent_config.i2c_enabled = false;
    s_persistent_config.i2c_bus_number = 1;
    
    s_persistent_config.spi_enabled = false;
    s_persistent_config.spi_bus_number = 0;
    
    s_persistent_config.uart_enabled = false;
    s_persistent_config.uart_number = 0;
    s_persistent_config.uart_baud_rate = 115200;
    
    s_persistent_config.gpio_enabled = true;
    
    s_persistent_config.heap_size = 1024 * 1024; // 1MB heap by default
    strncpy(s_persistent_config.config_file_path, "/etc/mcp_rpi/config.json", sizeof(s_persistent_config.config_file_path) - 1);
    
    // Create configuration mutex
    s_config_mutex = MCP_MutexCreate();
    if (s_config_mutex <= 0) {
        log_error("Failed to create configuration mutex");
        hal_rpi_deinit();
        return -2;
    }
    
    // Mark platform as initialized
    s_platform_initialized = true;
    s_config_initialized = true;
    log_info("Raspberry Pi MCP platform initialized");
    
    return 0;
}

/**
 * @brief Platform-specific MCP deinitialization
 */
int MCP_PlatformDeinit(void) {
    if (!s_platform_initialized) {
        return -1; // Not initialized
    }
    
    // Stop all timers
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (s_timer_used[i]) {
            MCP_TimerStop(i);
        }
    }
    
    // Free queue buffers
    for (int i = 0; i < MAX_QUEUES; i++) {
        if (s_queue_used[i] && s_queues[i].buffer != NULL) {
            free(s_queues[i].buffer);
        }
    }
    
    // Delete configuration mutex
    if (s_config_mutex > 0) {
        MCP_MutexDelete(s_config_mutex);
        s_config_mutex = -1;
    }
    
    // Deinitialize HAL
    hal_rpi_deinit();
    
    s_platform_initialized = false;
    s_config_initialized = false;
    log_info("Raspberry Pi MCP platform deinitialized");
    
    return 0;
}

/**
 * @brief Platform tick function to be called from main loop
 */
void MCP_PlatformTick(void) {
    // Call HAL tick function
    hal_rpi_tick();
}

// --- Memory management ---

/**
 * @brief Allocate memory
 */
void* MCP_Malloc(size_t size) {
    void* ptr = malloc(size);
    
    if (ptr != NULL) {
        s_total_allocated += size;
        if (s_total_allocated > s_peak_allocated) {
            s_peak_allocated = s_total_allocated;
        }
    }
    
    return ptr;
}

/**
 * @brief Free memory
 */
void MCP_Free(void* ptr) {
    if (ptr != NULL) {
        free(ptr);
        // Note: We can't accurately track freed memory size without additional bookkeeping
    }
}

/**
 * @brief Reallocate memory
 */
void* MCP_Realloc(void* ptr, size_t size) {
    // Note: This doesn't accurately track allocation stats for realloc operations
    return realloc(ptr, size);
}

/**
 * @brief Get memory usage statistics
 */
void MCP_GetMemoryStats(size_t* total, size_t* used, size_t* peak) {
    if (total != NULL) {
        *total = 0; // Unknown without hardware-specific memory info
    }
    
    if (used != NULL) {
        *used = s_total_allocated;
    }
    
    if (peak != NULL) {
        *peak = s_peak_allocated;
    }
}

// --- Time and delay functions ---

/**
 * @brief Get system time in milliseconds
 */
uint64_t MCP_GetTimeMs(void) {
    return hal_rpi_get_time_ms();
}

/**
 * @brief Delay for specified milliseconds
 */
void MCP_DelayMs(uint32_t ms) {
    hal_rpi_delay_ms(ms);
}

// --- Task scheduling ---

/**
 * @brief Create a new task
 */
MCP_TaskHandle MCP_TaskCreate(MCP_TaskFunction func, const char* name, size_t stack_size, 
                              void* arg, MCP_TaskPriority priority) {
    // Note: Real task creation requires an RTOS
    // For bare metal implementation, we could create a cooperative scheduler
    
    // For now, we just call the function directly
    if (func != NULL) {
        func(arg);
    }
    
    return 1; // Return a dummy task handle
}

/**
 * @brief Delete a task
 */
int MCP_TaskDelete(MCP_TaskHandle task) {
    // Not implemented in bare metal
    return 0;
}

/**
 * @brief Suspend a task
 */
int MCP_TaskSuspend(MCP_TaskHandle task) {
    // Not implemented in bare metal
    return 0;
}

/**
 * @brief Resume a task
 */
int MCP_TaskResume(MCP_TaskHandle task) {
    // Not implemented in bare metal
    return 0;
}

/**
 * @brief Yield execution to other tasks
 */
void MCP_TaskYield(void) {
    // Not implemented in bare metal
}

/**
 * @brief Get current task handle
 */
MCP_TaskHandle MCP_TaskGetCurrent(void) {
    return 1; // Return a dummy task handle
}

/**
 * @brief Get task name
 */
const char* MCP_TaskGetName(MCP_TaskHandle task) {
    return "main"; // Return a dummy task name
}

// --- Mutex synchronization ---

/**
 * @brief Create a mutex
 */
MCP_MutexHandle MCP_MutexCreate(void) {
    // Find a free mutex
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (!s_mutex_used[i]) {
            s_mutex_used[i] = true;
            s_mutexes[i].locked = false;
            s_mutexes[i].owner = 0;
            return i;
        }
    }
    
    return -1; // No free mutex available
}

/**
 * @brief Delete a mutex
 */
int MCP_MutexDelete(MCP_MutexHandle mutex) {
    if (mutex < 0 || mutex >= MAX_MUTEXES || !s_mutex_used[mutex]) {
        return -1; // Invalid mutex handle
    }
    
    s_mutex_used[mutex] = false;
    return 0;
}

/**
 * @brief Lock a mutex
 */
int MCP_MutexLock(MCP_MutexHandle mutex, uint32_t timeout_ms) {
    if (mutex < 0 || mutex >= MAX_MUTEXES || !s_mutex_used[mutex]) {
        return -1; // Invalid mutex handle
    }
    
    // Simple implementation without timeout support
    uint64_t start_time = MCP_GetTimeMs();
    
    while (s_mutexes[mutex].locked) {
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            return -2; // Timeout
        }
        
        // Yield to other tasks (would be a no-op in bare metal)
        MCP_TaskYield();
    }
    
    s_mutexes[mutex].locked = true;
    s_mutexes[mutex].owner = MCP_TaskGetCurrent();
    
    return 0;
}

/**
 * @brief Unlock a mutex
 */
int MCP_MutexUnlock(MCP_MutexHandle mutex) {
    if (mutex < 0 || mutex >= MAX_MUTEXES || !s_mutex_used[mutex]) {
        return -1; // Invalid mutex handle
    }
    
    if (!s_mutexes[mutex].locked) {
        return -2; // Mutex not locked
    }
    
    if (s_mutexes[mutex].owner != MCP_TaskGetCurrent()) {
        return -3; // Not the owner
    }
    
    s_mutexes[mutex].locked = false;
    s_mutexes[mutex].owner = 0;
    
    return 0;
}

// --- Semaphore synchronization ---

/**
 * @brief Create a semaphore
 */
MCP_SemaphoreHandle MCP_SemaphoreCreate(uint32_t max_count, uint32_t initial_count) {
    if (initial_count > max_count) {
        return -1; // Invalid parameters
    }
    
    // Find a free semaphore
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!s_semaphore_used[i]) {
            s_semaphore_used[i] = true;
            s_semaphores[i].count = initial_count;
            s_semaphores[i].max_count = max_count;
            return i;
        }
    }
    
    return -2; // No free semaphore available
}

/**
 * @brief Delete a semaphore
 */
int MCP_SemaphorDelete(MCP_SemaphoreHandle semaphore) {
    if (semaphore < 0 || semaphore >= MAX_SEMAPHORES || !s_semaphore_used[semaphore]) {
        return -1; // Invalid semaphore handle
    }
    
    s_semaphore_used[semaphore] = false;
    return 0;
}

/**
 * @brief Take a semaphore
 */
int MCP_SemaphoreTake(MCP_SemaphoreHandle semaphore, uint32_t timeout_ms) {
    if (semaphore < 0 || semaphore >= MAX_SEMAPHORES || !s_semaphore_used[semaphore]) {
        return -1; // Invalid semaphore handle
    }
    
    uint64_t start_time = MCP_GetTimeMs();
    
    while (s_semaphores[semaphore].count == 0) {
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            return -2; // Timeout
        }
        
        // Yield to other tasks (would be a no-op in bare metal)
        MCP_TaskYield();
    }
    
    s_semaphores[semaphore].count--;
    return 0;
}

/**
 * @brief Give a semaphore
 */
int MCP_SemaphoreGive(MCP_SemaphoreHandle semaphore) {
    if (semaphore < 0 || semaphore >= MAX_SEMAPHORES || !s_semaphore_used[semaphore]) {
        return -1; // Invalid semaphore handle
    }
    
    if (s_semaphores[semaphore].count >= s_semaphores[semaphore].max_count) {
        return -2; // Semaphore already at max count
    }
    
    s_semaphores[semaphore].count++;
    return 0;
}

// --- Timer management ---

/**
 * @brief Timer callback wrapper
 */
static void timer_callback_wrapper(void* arg) {
    int timer_id = *((int*)arg);
    
    if (timer_id >= 0 && timer_id < MAX_TIMERS && s_timer_used[timer_id]) {
        if (s_timers[timer_id].callback != NULL) {
            s_timers[timer_id].callback(s_timers[timer_id].arg);
        }
        
        // If not auto-reload, stop the timer
        if (!s_timers[timer_id].auto_reload) {
            MCP_TimerStop(timer_id);
        }
    }
}

/**
 * @brief Create a timer
 */
MCP_TimerHandle MCP_TimerCreate(const char* name, uint32_t period_ms, bool auto_reload, 
                               void* arg, MCP_TimerCallback callback) {
    if (name == NULL || period_ms == 0 || callback == NULL) {
        return -1; // Invalid parameters
    }
    
    // Find a free timer
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!s_timer_used[i]) {
            s_timer_used[i] = true;
            
            // Initialize the timer
            strncpy(s_timers[i].name, name, sizeof(s_timers[i].name) - 1);
            s_timers[i].period_ms = period_ms;
            s_timers[i].auto_reload = auto_reload;
            s_timers[i].arg = arg;
            s_timers[i].callback = callback;
            s_timers[i].hal_timer_id = -1;
            s_timers[i].active = false;
            
            return i;
        }
    }
    
    return -2; // No free timer available
}

/**
 * @brief Delete a timer
 */
int MCP_TimerDelete(MCP_TimerHandle timer) {
    if (timer < 0 || timer >= MAX_TIMERS || !s_timer_used[timer]) {
        return -1; // Invalid timer handle
    }
    
    // Stop the timer if it's active
    if (s_timers[timer].active) {
        MCP_TimerStop(timer);
    }
    
    s_timer_used[timer] = false;
    return 0;
}

/**
 * @brief Start a timer
 */
int MCP_TimerStart(MCP_TimerHandle timer) {
    if (timer < 0 || timer >= MAX_TIMERS || !s_timer_used[timer]) {
        return -1; // Invalid timer handle
    }
    
    // Stop the timer if it's already active
    if (s_timers[timer].active) {
        MCP_TimerStop(timer);
    }
    
    // Create context for callback (the timer ID)
    int* timer_id_ptr = (int*)malloc(sizeof(int));
    if (timer_id_ptr == NULL) {
        return -2; // Memory allocation failed
    }
    
    *timer_id_ptr = timer;
    
    // Start HAL timer
    int hal_timer_id = hal_rpi_timer_init(s_timers[timer].period_ms, timer_callback_wrapper, timer_id_ptr);
    
    if (hal_timer_id < 0) {
        free(timer_id_ptr);
        return -3; // Failed to start HAL timer
    }
    
    s_timers[timer].hal_timer_id = hal_timer_id;
    s_timers[timer].active = true;
    
    return 0;
}

/**
 * @brief Stop a timer
 */
int MCP_TimerStop(MCP_TimerHandle timer) {
    if (timer < 0 || timer >= MAX_TIMERS || !s_timer_used[timer]) {
        return -1; // Invalid timer handle
    }
    
    if (!s_timers[timer].active) {
        return 0; // Timer already stopped
    }
    
    // Stop HAL timer
    if (s_timers[timer].hal_timer_id >= 0) {
        hal_rpi_timer_stop(s_timers[timer].hal_timer_id);
    }
    
    s_timers[timer].hal_timer_id = -1;
    s_timers[timer].active = false;
    
    return 0;
}

/**
 * @brief Change timer period
 */
int MCP_TimerChangePeriod(MCP_TimerHandle timer, uint32_t period_ms) {
    if (timer < 0 || timer >= MAX_TIMERS || !s_timer_used[timer] || period_ms == 0) {
        return -1; // Invalid parameters
    }
    
    s_timers[timer].period_ms = period_ms;
    
    // If timer is active, restart it with the new period
    if (s_timers[timer].active) {
        MCP_TimerStop(timer);
        MCP_TimerStart(timer);
    }
    
    return 0;
}

// --- Event management ---

/**
 * @brief Create an event group
 */
MCP_EventGroupHandle MCP_EventGroupCreate(void) {
    // Find a free event group
    for (int i = 0; i < MAX_EVENT_GROUPS; i++) {
        if (!s_event_group_used[i]) {
            s_event_group_used[i] = true;
            s_event_groups[i].bits = 0;
            return i;
        }
    }
    
    return -1; // No free event group available
}

/**
 * @brief Delete an event group
 */
int MCP_EventGroupDelete(MCP_EventGroupHandle group) {
    if (group < 0 || group >= MAX_EVENT_GROUPS || !s_event_group_used[group]) {
        return -1; // Invalid event group handle
    }
    
    s_event_group_used[group] = false;
    return 0;
}

/**
 * @brief Set bits in an event group
 */
MCP_EventBits MCP_EventGroupSetBits(MCP_EventGroupHandle group, MCP_EventBits bits) {
    if (group < 0 || group >= MAX_EVENT_GROUPS || !s_event_group_used[group]) {
        return 0; // Invalid event group handle
    }
    
    s_event_groups[group].bits |= bits;
    return s_event_groups[group].bits;
}

/**
 * @brief Clear bits in an event group
 */
MCP_EventBits MCP_EventGroupClearBits(MCP_EventGroupHandle group, MCP_EventBits bits) {
    if (group < 0 || group >= MAX_EVENT_GROUPS || !s_event_group_used[group]) {
        return 0; // Invalid event group handle
    }
    
    s_event_groups[group].bits &= ~bits;
    return s_event_groups[group].bits;
}

/**
 * @brief Wait for bits in an event group
 */
MCP_EventBits MCP_EventGroupWaitBits(MCP_EventGroupHandle group, MCP_EventBits bits,
                                     bool clear_on_exit, bool wait_for_all, uint32_t timeout_ms) {
    if (group < 0 || group >= MAX_EVENT_GROUPS || !s_event_group_used[group]) {
        return 0; // Invalid event group handle
    }
    
    uint64_t start_time = MCP_GetTimeMs();
    
    while (1) {
        // Check bits
        if (wait_for_all) {
            if ((s_event_groups[group].bits & bits) == bits) {
                break;
            }
        } else {
            if (s_event_groups[group].bits & bits) {
                break;
            }
        }
        
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            return s_event_groups[group].bits; // Timeout
        }
        
        // Yield to other tasks (would be a no-op in bare metal)
        MCP_TaskYield();
    }
    
    // Get the bits
    MCP_EventBits result = s_event_groups[group].bits;
    
    // Clear bits if requested
    if (clear_on_exit) {
        s_event_groups[group].bits &= ~bits;
    }
    
    return result;
}

// --- Queue management ---

/**
 * @brief Create a queue
 */
MCP_QueueHandle MCP_QueueCreate(size_t item_size, size_t max_items) {
    if (item_size == 0 || max_items == 0) {
        return -1; // Invalid parameters
    }
    
    // Find a free queue
    for (int i = 0; i < MAX_QUEUES; i++) {
        if (!s_queue_used[i]) {
            // Allocate buffer
            void* buffer = malloc(item_size * max_items);
            if (buffer == NULL) {
                return -2; // Memory allocation failed
            }
            
            s_queue_used[i] = true;
            s_queues[i].buffer = buffer;
            s_queues[i].item_size = item_size;
            s_queues[i].max_items = max_items;
            s_queues[i].item_count = 0;
            s_queues[i].read_index = 0;
            s_queues[i].write_index = 0;
            
            // Create internal synchronization primitives
            s_queues[i].mutex.locked = false;
            s_queues[i].mutex.owner = 0;
            
            s_queues[i].not_empty.count = 0;
            s_queues[i].not_empty.max_count = 1;
            
            s_queues[i].not_full.count = 1;
            s_queues[i].not_full.max_count = 1;
            
            return i;
        }
    }
    
    return -3; // No free queue available
}

/**
 * @brief Delete a queue
 */
int MCP_QueueDelete(MCP_QueueHandle queue) {
    if (queue < 0 || queue >= MAX_QUEUES || !s_queue_used[queue]) {
        return -1; // Invalid queue handle
    }
    
    if (s_queues[queue].buffer != NULL) {
        free(s_queues[queue].buffer);
    }
    
    s_queue_used[queue] = false;
    return 0;
}

/**
 * @brief Send an item to the back of a queue
 */
int MCP_QueueSend(MCP_QueueHandle queue, const void* item, uint32_t timeout_ms) {
    if (queue < 0 || queue >= MAX_QUEUES || !s_queue_used[queue] || item == NULL) {
        return -1; // Invalid parameters
    }
    
    uint64_t start_time = MCP_GetTimeMs();
    
    // Lock queue mutex
    while (s_queues[queue].mutex.locked) {
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            return -2; // Timeout
        }
        
        MCP_TaskYield();
    }
    
    s_queues[queue].mutex.locked = true;
    s_queues[queue].mutex.owner = MCP_TaskGetCurrent();
    
    // Check if queue is full
    while (s_queues[queue].item_count >= s_queues[queue].max_items) {
        s_queues[queue].mutex.locked = false;
        
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            return -2; // Timeout
        }
        
        MCP_TaskYield();
        
        // Try to lock the queue mutex again
        while (s_queues[queue].mutex.locked) {
            // Check for timeout
            if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
                return -2; // Timeout
            }
            
            MCP_TaskYield();
        }
        
        s_queues[queue].mutex.locked = true;
        s_queues[queue].mutex.owner = MCP_TaskGetCurrent();
    }
    
    // Copy item to queue
    uint8_t* dest = (uint8_t*)s_queues[queue].buffer + s_queues[queue].write_index * s_queues[queue].item_size;
    memcpy(dest, item, s_queues[queue].item_size);
    
    // Update indices
    s_queues[queue].write_index = (s_queues[queue].write_index + 1) % s_queues[queue].max_items;
    s_queues[queue].item_count++;
    
    // Signal that the queue is not empty
    s_queues[queue].not_empty.count = 1;
    
    // Update not_full semaphore
    if (s_queues[queue].item_count >= s_queues[queue].max_items) {
        s_queues[queue].not_full.count = 0;
    }
    
    // Unlock queue mutex
    s_queues[queue].mutex.locked = false;
    
    return 0;
}

/**
 * @brief Send an item to the front of a queue
 */
int MCP_QueueSendToFront(MCP_QueueHandle queue, const void* item, uint32_t timeout_ms) {
    if (queue < 0 || queue >= MAX_QUEUES || !s_queue_used[queue] || item == NULL) {
        return -1; // Invalid parameters
    }
    
    uint64_t start_time = MCP_GetTimeMs();
    
    // Lock queue mutex
    while (s_queues[queue].mutex.locked) {
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            return -2; // Timeout
        }
        
        MCP_TaskYield();
    }
    
    s_queues[queue].mutex.locked = true;
    s_queues[queue].mutex.owner = MCP_TaskGetCurrent();
    
    // Check if queue is full
    while (s_queues[queue].item_count >= s_queues[queue].max_items) {
        s_queues[queue].mutex.locked = false;
        
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            return -2; // Timeout
        }
        
        MCP_TaskYield();
        
        // Try to lock the queue mutex again
        while (s_queues[queue].mutex.locked) {
            // Check for timeout
            if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
                return -2; // Timeout
            }
            
            MCP_TaskYield();
        }
        
        s_queues[queue].mutex.locked = true;
        s_queues[queue].mutex.owner = MCP_TaskGetCurrent();
    }
    
    // Update read index
    if (s_queues[queue].read_index == 0) {
        s_queues[queue].read_index = s_queues[queue].max_items - 1;
    } else {
        s_queues[queue].read_index--;
    }
    
    // Copy item to queue
    uint8_t* dest = (uint8_t*)s_queues[queue].buffer + s_queues[queue].read_index * s_queues[queue].item_size;
    memcpy(dest, item, s_queues[queue].item_size);
    
    // Update item count
    s_queues[queue].item_count++;
    
    // Signal that the queue is not empty
    s_queues[queue].not_empty.count = 1;
    
    // Update not_full semaphore
    if (s_queues[queue].item_count >= s_queues[queue].max_items) {
        s_queues[queue].not_full.count = 0;
    }
    
    // Unlock queue mutex
    s_queues[queue].mutex.locked = false;
    
    return 0;
}

/**
 * @brief Receive an item from a queue
 */
int MCP_QueueReceive(MCP_QueueHandle queue, void* item, uint32_t timeout_ms) {
    if (queue < 0 || queue >= MAX_QUEUES || !s_queue_used[queue] || item == NULL) {
        return -1; // Invalid parameters
    }
    
    uint64_t start_time = MCP_GetTimeMs();
    
    // Lock queue mutex
    while (s_queues[queue].mutex.locked) {
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            return -2; // Timeout
        }
        
        MCP_TaskYield();
    }
    
    s_queues[queue].mutex.locked = true;
    s_queues[queue].mutex.owner = MCP_TaskGetCurrent();
    
    // Check if queue is empty
    while (s_queues[queue].item_count == 0) {
        s_queues[queue].mutex.locked = false;
        
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            return -2; // Timeout
        }
        
        MCP_TaskYield();
        
        // Try to lock the queue mutex again
        while (s_queues[queue].mutex.locked) {
            // Check for timeout
            if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
                return -2; // Timeout
            }
            
            MCP_TaskYield();
        }
        
        s_queues[queue].mutex.locked = true;
        s_queues[queue].mutex.owner = MCP_TaskGetCurrent();
    }
    
    // Copy item from queue
    uint8_t* src = (uint8_t*)s_queues[queue].buffer + s_queues[queue].read_index * s_queues[queue].item_size;
    memcpy(item, src, s_queues[queue].item_size);
    
    // Update indices
    s_queues[queue].read_index = (s_queues[queue].read_index + 1) % s_queues[queue].max_items;
    s_queues[queue].item_count--;
    
    // Signal that the queue is not full
    s_queues[queue].not_full.count = 1;
    
    // Update not_empty semaphore
    if (s_queues[queue].item_count == 0) {
        s_queues[queue].not_empty.count = 0;
    }
    
    // Unlock queue mutex
    s_queues[queue].mutex.locked = false;
    
    return 0;
}

/**
 * @brief Get the number of items in a queue
 */
size_t MCP_QueueGetCount(MCP_QueueHandle queue) {
    if (queue < 0 || queue >= MAX_QUEUES || !s_queue_used[queue]) {
        return 0; // Invalid queue handle
    }
    
    return s_queues[queue].item_count;
}

/**
 * @brief Reset a queue
 */
int MCP_QueueReset(MCP_QueueHandle queue) {
    if (queue < 0 || queue >= MAX_QUEUES || !s_queue_used[queue]) {
        return -1; // Invalid queue handle
    }
    
    // Lock queue mutex
    while (s_queues[queue].mutex.locked) {
        MCP_TaskYield();
    }
    
    s_queues[queue].mutex.locked = true;
    s_queues[queue].mutex.owner = MCP_TaskGetCurrent();
    
    // Reset queue
    s_queues[queue].item_count = 0;
    s_queues[queue].read_index = 0;
    s_queues[queue].write_index = 0;
    
    // Update semaphores
    s_queues[queue].not_empty.count = 0;
    s_queues[queue].not_full.count = 1;
    
    // Unlock queue mutex
    s_queues[queue].mutex.locked = false;
    
    return 0;
}

// --- GPIO operations ---

/**
 * @brief Configure GPIO pin
 */
int MCP_GPIOConfig(int pin, MCP_GPIOMode mode) {
    int function;
    int pull;
    
    // Convert MCP GPIO mode to HAL GPIO function
    switch (mode) {
        case MCP_GPIO_MODE_INPUT:
            function = RPI_GPIO_FUNC_INPUT;
            pull = RPI_GPIO_PULL_NONE;
            break;
        case MCP_GPIO_MODE_OUTPUT:
            function = RPI_GPIO_FUNC_OUTPUT;
            pull = RPI_GPIO_PULL_NONE;
            break;
        case MCP_GPIO_MODE_INPUT_PULLUP:
            function = RPI_GPIO_FUNC_INPUT;
            pull = RPI_GPIO_PULL_UP;
            break;
        case MCP_GPIO_MODE_INPUT_PULLDOWN:
            function = RPI_GPIO_FUNC_INPUT;
            pull = RPI_GPIO_PULL_DOWN;
            break;
        case MCP_GPIO_MODE_ALT0:
            function = RPI_GPIO_FUNC_ALT0;
            pull = RPI_GPIO_PULL_NONE;
            break;
        case MCP_GPIO_MODE_ALT1:
            function = RPI_GPIO_FUNC_ALT1;
            pull = RPI_GPIO_PULL_NONE;
            break;
        case MCP_GPIO_MODE_ALT2:
            function = RPI_GPIO_FUNC_ALT2;
            pull = RPI_GPIO_PULL_NONE;
            break;
        case MCP_GPIO_MODE_ALT3:
            function = RPI_GPIO_FUNC_ALT3;
            pull = RPI_GPIO_PULL_NONE;
            break;
        case MCP_GPIO_MODE_ALT4:
            function = RPI_GPIO_FUNC_ALT4;
            pull = RPI_GPIO_PULL_NONE;
            break;
        case MCP_GPIO_MODE_ALT5:
            function = RPI_GPIO_FUNC_ALT5;
            pull = RPI_GPIO_PULL_NONE;
            break;
        default:
            return -1; // Invalid mode
    }
    
    // Configure GPIO
    int result = hal_rpi_gpio_function(pin, function);
    if (result != 0) {
        return result;
    }
    
    // Set pull-up/down
    return hal_rpi_gpio_pull(pin, pull);
}

/**
 * @brief Set GPIO output level
 */
int MCP_GPIOWrite(int pin, int level) {
    return hal_rpi_gpio_write(pin, level);
}

/**
 * @brief Read GPIO input level
 */
int MCP_GPIORead(int pin) {
    return hal_rpi_gpio_read(pin);
}

// --- UART operations ---

/**
 * @brief Initialize UART
 */
MCP_UARTHandle MCP_UARTInit(const MCP_UARTConfig* config) {
    if (config == NULL) {
        return -1; // Invalid parameters
    }
    
    // Initialize HAL UART
    int result = hal_rpi_uart_init(config->uart_num, config->baudrate);
    if (result != 0) {
        return -2; // Failed to initialize UART
    }
    
    // Return UART number as handle
    return config->uart_num;
}

/**
 * @brief Deinitialize UART
 */
int MCP_UARTDeinit(MCP_UARTHandle uart) {
    // Nothing to do in this implementation
    return 0;
}

/**
 * @brief Write data to UART
 */
size_t MCP_UARTWrite(MCP_UARTHandle uart, const uint8_t* data, size_t size) {
    if (data == NULL || size == 0) {
        return 0;
    }
    
    // Write each byte
    size_t written = 0;
    for (size_t i = 0; i < size; i++) {
        int result = hal_rpi_uart_putc(uart, data[i]);
        if (result != 0) {
            break;
        }
        written++;
    }
    
    return written;
}

/**
 * @brief Read data from UART
 */
size_t MCP_UARTRead(MCP_UARTHandle uart, uint8_t* data, size_t size, uint32_t timeout_ms) {
    if (data == NULL || size == 0) {
        return 0;
    }
    
    uint64_t start_time = MCP_GetTimeMs();
    size_t read = 0;
    
    // Read until buffer is full or timeout
    while (read < size) {
        // Check for timeout
        if (timeout_ms != 0xFFFFFFFF && MCP_GetTimeMs() - start_time >= timeout_ms) {
            break;
        }
        
        // Check if data is available
        if (hal_rpi_uart_available(uart)) {
            int c = hal_rpi_uart_getc(uart);
            if (c >= 0) {
                data[read++] = (uint8_t)c;
            }
        } else {
            // Yield to other tasks
            MCP_TaskYield();
        }
    }
    
    return read;
}

/**
 * @brief Check if UART has received data available
 */
int MCP_UARTAvailable(MCP_UARTHandle uart) {
    return hal_rpi_uart_available(uart);
}

// --- I2C operations ---

/**
 * @brief Initialize I2C
 */
MCP_I2CHandle MCP_I2CInit(const MCP_I2CConfig* config) {
    if (config == NULL) {
        return -1; // Invalid parameters
    }
    
    // Initialize HAL I2C
    int result = hal_rpi_i2c_init(config->i2c_num, config->clock_speed);
    if (result != 0) {
        return -2; // Failed to initialize I2C
    }
    
    // Return I2C number as handle
    return config->i2c_num;
}

/**
 * @brief Deinitialize I2C
 */
int MCP_I2CDeinit(MCP_I2CHandle i2c) {
    // Nothing to do in this implementation
    return 0;
}

/**
 * @brief Write data to I2C device
 */
int MCP_I2CWrite(MCP_I2CHandle i2c, uint8_t address, const uint8_t* data, size_t size) {
    return hal_rpi_i2c_write(i2c, address, data, size);
}

/**
 * @brief Read data from I2C device
 */
int MCP_I2CRead(MCP_I2CHandle i2c, uint8_t address, uint8_t* data, size_t size) {
    return hal_rpi_i2c_read(i2c, address, data, size);
}

// --- SPI operations ---

/**
 * @brief Initialize SPI
 */
MCP_SPIHandle MCP_SPIInit(const MCP_SPIConfig* config) {
    if (config == NULL) {
        return -1; // Invalid parameters
    }
    
    // Initialize HAL SPI
    int result = hal_rpi_spi_init(config->spi_num, config->clock_div, config->mode);
    if (result != 0) {
        return -2; // Failed to initialize SPI
    }
    
    // Configure CS pin as output if specified
    if (config->cs_pin >= 0) {
        hal_rpi_gpio_function(config->cs_pin, RPI_GPIO_FUNC_OUTPUT);
        hal_rpi_gpio_write(config->cs_pin, 1); // Deactivate CS
    }
    
    // Return SPI number as handle
    return config->spi_num;
}

/**
 * @brief Deinitialize SPI
 */
int MCP_SPIDeinit(MCP_SPIHandle spi) {
    // Nothing to do in this implementation
    return 0;
}

/**
 * @brief Transfer data over SPI
 */
int MCP_SPITransfer(MCP_SPIHandle spi, const uint8_t* tx_data, uint8_t* rx_data, size_t size) {
    if (tx_data == NULL || rx_data == NULL || size == 0) {
        return -1; // Invalid parameters
    }
    
    // Transfer each byte
    for (size_t i = 0; i < size; i++) {
        int result = hal_rpi_spi_transfer(spi, tx_data[i]);
        if (result < 0) {
            return -2; // Transfer failed
        }
        rx_data[i] = (uint8_t)result;
    }
    
    return 0;
}

// --- PWM operations ---

/**
 * @brief Initialize PWM
 */
MCP_PWMHandle MCP_PWMInit(const MCP_PWMConfig* config) {
    if (config == NULL || config->channel < 0 || config->channel >= MAX_PWM) {
        return -1; // Invalid parameters
    }
    
    // Calculate range and clock divider based on frequency and resolution
    int range = 1 << config->resolution;
    int clock_div = 19200000 / (range * config->frequency); // Assuming 19.2MHz clock source
    
    // Initialize HAL PWM
    int result = hal_rpi_pwm_init(config->channel, range, clock_div);
    if (result != 0) {
        return -2; // Failed to initialize PWM
    }
    
    // Return channel as handle
    return config->channel;
}

/**
 * @brief Deinitialize PWM
 */
int MCP_PWMDeinit(MCP_PWMHandle pwm) {
    // Nothing to do in this implementation
    return 0;
}

/**
 * @brief Set PWM duty cycle
 */
int MCP_PWMSetDuty(MCP_PWMHandle pwm, int duty) {
    return hal_rpi_pwm_set_duty(pwm, duty);
}

/**
 * @brief Set PWM frequency
 */
int MCP_PWMSetFrequency(MCP_PWMHandle pwm, int frequency) {
    // Not implemented in this version
    return -1;
}

// --- System functions ---

/**
 * @brief Reboot the system
 */
void MCP_SystemReboot(void) {
    // Not implemented in this version (would require platform-specific reboot mechanism)
}

/**
 * @brief Get system uptime in seconds
 */
uint32_t MCP_SystemGetUptime(void) {
    return (uint32_t)(hal_rpi_get_time_ms() / 1000);
}

/**
 * @brief Get system temperature in Celsius
 */
float MCP_SystemGetTemperature(void) {
    return hal_rpi_get_temperature();
}

/**
 * @brief Get system memory information
 */
int MCP_SystemGetMemoryInfo(uint32_t* total, uint32_t* free) {
    return hal_rpi_get_memory_info(total, free);
}

/**
 * @brief Get system information string
 */
const char* MCP_SystemGetInfo(void) {
    // In a real implementation, this would return system information
    static char info[128];
    snprintf(info, sizeof(info), "Raspberry Pi MCP System, Device: %s, Firmware: %s", 
             s_persistent_config.device_name, s_persistent_config.firmware_version);
    return info;
}

/**
 * @brief Set a configuration parameter by name
 * 
 * @param json_config JSON string containing the configuration to update
 * @return int 0 on success, negative error code on failure
 */
int MCP_SetConfiguration(const char* json_config) {
    if (json_config == NULL) {
        log_error("Invalid configuration: NULL pointer");
        return -1;
    }
    
    if (!s_config_initialized) {
        log_error("Configuration not initialized");
        return -2;
    }
    
    // Lock config mutex
    int lock_result = MCP_MutexLock(s_config_mutex, 5000);
    if (lock_result != 0) {
        log_error("Failed to acquire config mutex: %d", lock_result);
        return -3;
    }
    
    // Parse the JSON configuration and update persistent config
    int result = ParseJSONConfig(json_config);
    if (result != 0) {
        log_error("Failed to parse configuration JSON: %d", result);
        MCP_MutexUnlock(s_config_mutex);
        return -4;
    }
    
    // Unlock config mutex
    MCP_MutexUnlock(s_config_mutex);
    
    // Apply configuration changes
    
    // Server configuration
    if (s_server_state.initialized) {
        s_server_state.port = s_persistent_config.server_port;
    }
    
    // WiFi configuration changes are not automatically applied,
    // the user must call MCP_WiFiConnect, MCP_WiFiDisconnect, etc.
    
    // BLE configuration changes are not automatically applied,
    // the user must call MCP_BLEInit, MCP_BLEStartAdvertising, etc.
    
    // Ethernet configuration changes are not automatically applied,
    // the user must call MCP_EthernetConfigure
    
    // Save the updated configuration
    result = MCP_SavePersistentState();
    if (result != 0) {
        log_warn("Failed to save persistent state: %d", result);
        // Non-fatal, continue
    }
    
    return 0;
}

/**
 * @brief Get the current configuration as JSON
 * 
 * @param buffer Buffer to store the JSON string
 * @param size Size of the buffer
 * @return int Length of the JSON string or negative error code on failure
 */
int MCP_GetConfiguration(char* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        log_error("Invalid buffer");
        return -1;
    }
    
    if (!s_config_initialized) {
        log_error("Configuration not initialized");
        return -2;
    }
    
    // Lock config mutex
    int lock_result = MCP_MutexLock(s_config_mutex, 5000);
    if (lock_result != 0) {
        log_error("Failed to acquire config mutex: %d", lock_result);
        return -3;
    }
    
    // Serialize configuration to JSON
    int result = SerializeConfigToJSON(buffer, size);
    
    // Unlock config mutex
    MCP_MutexUnlock(s_config_mutex);
    
    if (result <= 0) {
        log_error("Failed to serialize configuration to JSON: %d", result);
        return -4;
    }
    
    return result;
}

// --- File system operations ---

// Private structure to represent file handles
typedef struct {
    FILE* handle;
    char* path;
    MCP_FileMode mode;
} RPi_File;

// File handle storage
#define MAX_FILES 16
static RPi_File s_files[MAX_FILES];
static bool s_file_used[MAX_FILES];

/**
 * @brief Open a file
 */
MCP_FileHandle MCP_FileOpen(const char* path, MCP_FileMode mode) {
    if (path == NULL) {
        return NULL;
    }
    
    // Find a free file handle
    int index = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!s_file_used[i]) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        log_error("No free file handles available");
        return NULL;
    }
    
    // Convert MCP file mode to standard C file mode
    const char* file_mode;
    switch (mode) {
        case MCP_FILE_MODE_READ:
            file_mode = "r";
            break;
        case MCP_FILE_MODE_WRITE:
            file_mode = "w";
            break;
        case MCP_FILE_MODE_APPEND:
            file_mode = "a";
            break;
        case MCP_FILE_MODE_READ_WRITE:
            file_mode = "r+";
            break;
        case MCP_FILE_MODE_READ_WRITE_CREATE:
            file_mode = "w+";
            break;
        default:
            log_error("Invalid file mode: %d", mode);
            return NULL;
    }
    
    // Open the file
    FILE* file = fopen(path, file_mode);
    if (file == NULL) {
        log_error("Failed to open file %s: %s", path, strerror(errno));
        return NULL;
    }
    
    // Store file information
    s_files[index].handle = file;
    s_files[index].path = strdup(path);
    s_files[index].mode = mode;
    s_file_used[index] = true;
    
    // Return the index as a handle
    return (MCP_FileHandle)&s_files[index];
}

/**
 * @brief Close a file
 */
int MCP_FileClose(MCP_FileHandle file) {
    if (file == NULL) {
        return -1;
    }
    
    RPi_File* f = (RPi_File*)file;
    
    // Validate the handle
    if (f < s_files || f >= s_files + MAX_FILES) {
        log_error("Invalid file handle");
        return -2;
    }
    
    int index = f - s_files;
    if (!s_file_used[index]) {
        log_error("File handle not in use");
        return -3;
    }
    
    // Close the file
    if (f->handle != NULL) {
        fclose(f->handle);
        f->handle = NULL;
    }
    
    // Free path
    if (f->path != NULL) {
        free(f->path);
        f->path = NULL;
    }
    
    // Mark as unused
    s_file_used[index] = false;
    
    return 0;
}

/**
 * @brief Read from a file
 */
size_t MCP_FileRead(MCP_FileHandle file, void* buffer, size_t size) {
    if (file == NULL || buffer == NULL || size == 0) {
        return 0;
    }
    
    RPi_File* f = (RPi_File*)file;
    
    // Validate the handle
    if (f < s_files || f >= s_files + MAX_FILES) {
        log_error("Invalid file handle");
        return 0;
    }
    
    int index = f - s_files;
    if (!s_file_used[index] || f->handle == NULL) {
        log_error("File handle not in use or file not open");
        return 0;
    }
    
    // Read from the file
    return fread(buffer, 1, size, f->handle);
}

/**
 * @brief Write to a file
 */
size_t MCP_FileWrite(MCP_FileHandle file, const void* buffer, size_t size) {
    if (file == NULL || buffer == NULL || size == 0) {
        return 0;
    }
    
    RPi_File* f = (RPi_File*)file;
    
    // Validate the handle
    if (f < s_files || f >= s_files + MAX_FILES) {
        log_error("Invalid file handle");
        return 0;
    }
    
    int index = f - s_files;
    if (!s_file_used[index] || f->handle == NULL) {
        log_error("File handle not in use or file not open");
        return 0;
    }
    
    // Write to the file
    return fwrite(buffer, 1, size, f->handle);
}

/**
 * @brief Seek within a file
 */
int MCP_FileSeek(MCP_FileHandle file, long offset, int origin) {
    if (file == NULL) {
        return -1;
    }
    
    RPi_File* f = (RPi_File*)file;
    
    // Validate the handle
    if (f < s_files || f >= s_files + MAX_FILES) {
        log_error("Invalid file handle");
        return -2;
    }
    
    int index = f - s_files;
    if (!s_file_used[index] || f->handle == NULL) {
        log_error("File handle not in use or file not open");
        return -3;
    }
    
    // Convert origin to standard seek values
    int whence;
    switch (origin) {
        case 0: // SEEK_SET
            whence = SEEK_SET;
            break;
        case 1: // SEEK_CUR
            whence = SEEK_CUR;
            break;
        case 2: // SEEK_END
            whence = SEEK_END;
            break;
        default:
            log_error("Invalid seek origin: %d", origin);
            return -4;
    }
    
    // Seek in the file
    if (fseek(f->handle, offset, whence) != 0) {
        log_error("File seek failed: %s", strerror(errno));
        return -5;
    }
    
    return 0;
}

/**
 * @brief Get file position
 */
long MCP_FileTell(MCP_FileHandle file) {
    if (file == NULL) {
        return -1;
    }
    
    RPi_File* f = (RPi_File*)file;
    
    // Validate the handle
    if (f < s_files || f >= s_files + MAX_FILES) {
        log_error("Invalid file handle");
        return -2;
    }
    
    int index = f - s_files;
    if (!s_file_used[index] || f->handle == NULL) {
        log_error("File handle not in use or file not open");
        return -3;
    }
    
    // Get file position
    return ftell(f->handle);
}

/**
 * @brief Check if file exists
 */
bool MCP_FileExists(const char* path) {
    if (path == NULL) {
        return false;
    }
    
    // Try to open the file for reading
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return false;
    }
    
    // Close the file and return true
    fclose(file);
    return true;
}

/**
 * @brief Delete a file
 */
int MCP_FileDelete(const char* path) {
    if (path == NULL) {
        return -1;
    }
    
    // Delete the file
    if (remove(path) != 0) {
        log_error("Failed to delete file %s: %s", path, strerror(errno));
        return -2;
    }
    
    return 0;
}

/**
 * @brief Create a directory
 */
int MCP_DirCreate(const char* path) {
    if (path == NULL) {
        return -1;
    }
    
    // Create the directory (0755 permissions)
    if (mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
        log_error("Failed to create directory %s: %s", path, strerror(errno));
        return -2;
    }
    
    return 0;
}

/**
 * @brief Delete a directory
 */
int MCP_DirDelete(const char* path) {
    if (path == NULL) {
        return -1;
    }
    
    // Delete the directory
    if (rmdir(path) != 0) {
        log_error("Failed to delete directory %s: %s", path, strerror(errno));
        return -2;
    }
    
    return 0;
}

// --- MCP Server sensor integration ---

// Define structures for sensor storage
#define MAX_SENSORS 32

typedef struct {
    bool used;
    char id[32];
    char name[64];
    char type[32];
    int pin;
    int interface;
    uint64_t last_read_time;
    uint32_t sample_count;
    bool enabled;
    void* driver_data;
} RPi_Sensor;

static RPi_Sensor s_sensors[MAX_SENSORS];
static MCP_MutexHandle s_sensor_mutex = -1;

/**
 * @brief Initialize the sensor system
 */
int MCP_SensorSystemInit(void) {
    // Clear sensor registry
    memset(s_sensors, 0, sizeof(s_sensors));
    
    // Create mutex for thread safety
    s_sensor_mutex = MCP_MutexCreate();
    if (s_sensor_mutex < 0) {
        log_error("Failed to create sensor mutex");
        return -1;
    }
    
    log_info("Sensor system initialized");
    return 0;
}

/**
 * @brief Register a sensor
 */
int MCP_SensorRegister(const char* id, const char* name, const char* type, int interface, int pin, const char* driver_id) {
    if (id == NULL || name == NULL || type == NULL) {
        return -1;
    }
    
    // Lock mutex
    MCP_MutexLock(s_sensor_mutex, 1000);
    
    // Find a free sensor slot
    int index = -1;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (!s_sensors[i].used) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        log_error("No free sensor slots available");
        MCP_MutexUnlock(s_sensor_mutex);
        return -2;
    }
    
    // Check if sensor with this ID already exists
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (s_sensors[i].used && strcmp(s_sensors[i].id, id) == 0) {
            log_error("Sensor with ID '%s' already exists", id);
            MCP_MutexUnlock(s_sensor_mutex);
            return -3;
        }
    }
    
    // Initialize sensor
    s_sensors[index].used = true;
    strncpy(s_sensors[index].id, id, sizeof(s_sensors[index].id) - 1);
    strncpy(s_sensors[index].name, name, sizeof(s_sensors[index].name) - 1);
    strncpy(s_sensors[index].type, type, sizeof(s_sensors[index].type) - 1);
    s_sensors[index].interface = interface;
    s_sensors[index].pin = pin;
    s_sensors[index].last_read_time = 0;
    s_sensors[index].sample_count = 0;
    s_sensors[index].enabled = true;
    s_sensors[index].driver_data = NULL;
    
    // Initialize sensor hardware based on type
    // This would normally involve specific initialization code for different sensor types
    
    MCP_MutexUnlock(s_sensor_mutex);
    log_info("Registered sensor '%s' (%s) on pin %d", id, name, pin);
    return 0;
}

/**
 * @brief Unregister a sensor
 */
int MCP_SensorUnregister(const char* id) {
    if (id == NULL) {
        return -1;
    }
    
    // Lock mutex
    MCP_MutexLock(s_sensor_mutex, 1000);
    
    // Find sensor with this ID
    int index = -1;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (s_sensors[i].used && strcmp(s_sensors[i].id, id) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        log_error("Sensor with ID '%s' not found", id);
        MCP_MutexUnlock(s_sensor_mutex);
        return -2;
    }
    
    // Free driver data if needed
    if (s_sensors[index].driver_data != NULL) {
        free(s_sensors[index].driver_data);
    }
    
    // Clear sensor slot
    memset(&s_sensors[index], 0, sizeof(RPi_Sensor));
    
    MCP_MutexUnlock(s_sensor_mutex);
    log_info("Unregistered sensor '%s'", id);
    return 0;
}

/**
 * @brief Read a sensor value
 */
int MCP_SensorRead(const char* id, void* value, size_t size) {
    if (id == NULL || value == NULL || size == 0) {
        return -1;
    }
    
    // Lock mutex
    MCP_MutexLock(s_sensor_mutex, 1000);
    
    // Find sensor with this ID
    int index = -1;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (s_sensors[i].used && strcmp(s_sensors[i].id, id) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        log_error("Sensor with ID '%s' not found", id);
        MCP_MutexUnlock(s_sensor_mutex);
        return -2;
    }
    
    // Check if sensor is enabled
    if (!s_sensors[index].enabled) {
        log_error("Sensor '%s' is disabled", id);
        MCP_MutexUnlock(s_sensor_mutex);
        return -3;
    }
    
    // Read sensor value based on type
    int result = 0;
    if (strcmp(s_sensors[index].type, "temperature") == 0) {
        // Example implementation for temperature sensor
        if (size >= sizeof(float)) {
            // This would normally read from the actual sensor hardware
            // For now, we'll return a simulated value
            *((float*)value) = 25.0f + (rand() % 100) / 10.0f;
            result = sizeof(float);
        } else {
            result = -4; // Buffer too small
        }
    } 
    else if (strcmp(s_sensors[index].type, "humidity") == 0) {
        // Example implementation for humidity sensor
        if (size >= sizeof(float)) {
            // This would normally read from the actual sensor hardware
            // For now, we'll return a simulated value
            *((float*)value) = 40.0f + (rand() % 300) / 10.0f;
            result = sizeof(float);
        } else {
            result = -4; // Buffer too small
        }
    }
    else if (strcmp(s_sensors[index].type, "motion") == 0) {
        // Example implementation for motion sensor
        if (size >= sizeof(bool)) {
            // This would normally read from the actual sensor hardware
            // For now, we'll return a simulated value
            *((bool*)value) = (rand() % 10) < 2; // 20% chance of motion detected
            result = sizeof(bool);
        } else {
            result = -4; // Buffer too small
        }
    }
    else {
        log_error("Unknown sensor type '%s'", s_sensors[index].type);
        result = -5;
    }
    
    // Update sensor statistics
    if (result > 0) {
        s_sensors[index].last_read_time = MCP_GetTimeMs();
        s_sensors[index].sample_count++;
    }
    
    MCP_MutexUnlock(s_sensor_mutex);
    return result;
}

/**
 * @brief Enable or disable a sensor
 */
int MCP_SensorSetEnabled(const char* id, bool enabled) {
    if (id == NULL) {
        return -1;
    }
    
    // Lock mutex
    MCP_MutexLock(s_sensor_mutex, 1000);
    
    // Find sensor with this ID
    int index = -1;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (s_sensors[i].used && strcmp(s_sensors[i].id, id) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        log_error("Sensor with ID '%s' not found", id);
        MCP_MutexUnlock(s_sensor_mutex);
        return -2;
    }
    
    // Set enabled state
    s_sensors[index].enabled = enabled;
    
    MCP_MutexUnlock(s_sensor_mutex);
    log_info("Sensor '%s' %s", id, enabled ? "enabled" : "disabled");
    return 0;
}

/**
 * @brief Get sensor information
 */
int MCP_SensorGetInfo(const char* id, char* buffer, size_t size) {
    if (id == NULL || buffer == NULL || size == 0) {
        return -1;
    }
    
    // Lock mutex
    MCP_MutexLock(s_sensor_mutex, 1000);
    
    // Find sensor with this ID
    int index = -1;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (s_sensors[i].used && strcmp(s_sensors[i].id, id) == 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        log_error("Sensor with ID '%s' not found", id);
        MCP_MutexUnlock(s_sensor_mutex);
        return -2;
    }
    
    // Format sensor information as JSON
    int result = snprintf(buffer, size, 
        "{"
        "\"id\":\"%s\","
        "\"name\":\"%s\","
        "\"type\":\"%s\","
        "\"interface\":%d,"
        "\"pin\":%d,"
        "\"enabled\":%s,"
        "\"lastReadTime\":%llu,"
        "\"sampleCount\":%u"
        "}",
        s_sensors[index].id,
        s_sensors[index].name,
        s_sensors[index].type,
        s_sensors[index].interface,
        s_sensors[index].pin,
        s_sensors[index].enabled ? "true" : "false",
        s_sensors[index].last_read_time,
        s_sensors[index].sample_count
    );
    
    MCP_MutexUnlock(s_sensor_mutex);
    return (result < 0 || (size_t)result >= size) ? -3 : result;
}

/**
 * @brief Get list of all sensors
 */
int MCP_SensorListAll(char* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return -1;
    }
    
    // Lock mutex
    MCP_MutexLock(s_sensor_mutex, 1000);
    
    // Count active sensors
    int count = 0;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (s_sensors[i].used) {
            count++;
        }
    }
    
    // Start JSON array
    int pos = 0;
    pos += snprintf(buffer + pos, size - pos, "[");
    
    // Add each sensor
    int added = 0;
    for (int i = 0; i < MAX_SENSORS && pos < (int)size - 2; i++) {
        if (s_sensors[i].used) {
            // Add comma if not the first item
            if (added > 0) {
                pos += snprintf(buffer + pos, size - pos, ",");
            }
            
            // Add sensor info
            pos += snprintf(buffer + pos, size - pos,
                "{"
                "\"id\":\"%s\","
                "\"name\":\"%s\","
                "\"type\":\"%s\","
                "\"enabled\":%s"
                "}",
                s_sensors[i].id,
                s_sensors[i].name,
                s_sensors[i].type,
                s_sensors[i].enabled ? "true" : "false"
            );
            
            added++;
        }
    }
    
    // Close JSON array
    pos += snprintf(buffer + pos, size - pos, "]");
    
    MCP_MutexUnlock(s_sensor_mutex);
    return pos;
}

// --- Network operations ---

// Persistent configuration state
typedef struct {
    // Device information
    char device_name[64];
    char firmware_version[32];
    bool debug_enabled;
    
    // Server configuration
    bool server_enabled;
    uint16_t server_port;
    bool auto_start_server;
    
    // WiFi configuration
    bool wifi_enabled;
    char wifi_ssid[64];
    char wifi_password[64];
    bool wifi_auto_connect;
    
    // WiFi AP configuration
    bool wifi_ap_enabled;
    char wifi_ap_ssid[64];
    char wifi_ap_password[64];
    uint16_t wifi_ap_channel;
    
    // BLE configuration
    bool ble_enabled;
    char ble_device_name[64];
    bool ble_auto_advertise;
    
    // Ethernet configuration
    bool ethernet_enabled;
    char ethernet_interface[16];
    bool ethernet_dhcp;
    char ethernet_static_ip[16];
    char ethernet_gateway[16];
    char ethernet_subnet[16];
    
    // Interface configurations
    bool i2c_enabled;
    int i2c_bus_number;
    bool spi_enabled;
    int spi_bus_number;
    bool uart_enabled;
    int uart_number;
    uint32_t uart_baud_rate;
    bool gpio_enabled;
    
    // System configurations
    uint32_t heap_size;
    char config_file_path[128];
} MCP_PersistentConfig;

// Global persistent configuration 
static MCP_PersistentConfig s_persistent_config;
static MCP_MutexHandle s_config_mutex;
static bool s_config_initialized = false;

// WiFi connection state
static struct {
    bool initialized;
    MCP_WiFiStatus status;
    char ssid[64];
    char password[64];
    char ip_address[16];
    int rssi;
    bool ap_mode;
    char ap_ssid[64];
    MCP_MutexHandle mutex;
} s_wifi_state;

// BLE state
static struct {
    bool initialized;
    bool advertising;
    bool connected;
    char device_name[64];
    MCP_MutexHandle mutex;
} s_ble_state;

// Ethernet state
static struct {
    bool initialized;
    bool connected;
    char interface[16];
    char ip_address[16];
    bool dhcp;
    char static_ip[16];
    char gateway[16];
    char subnet[16];
    MCP_MutexHandle mutex;
} s_ethernet_state;

/**
 * @brief Initialize network subsystems
 */
int MCP_NetworkInit(void) {
    // Initialize WiFi state
    memset(&s_wifi_state, 0, sizeof(s_wifi_state));
    s_wifi_state.mutex = MCP_MutexCreate();
    if (s_wifi_state.mutex <= 0) {
        log_error("Failed to create WiFi mutex");
        return -1;
    }
    
    // Initialize BLE state
    memset(&s_ble_state, 0, sizeof(s_ble_state));
    s_ble_state.mutex = MCP_MutexCreate();
    if (s_ble_state.mutex <= 0) {
        log_error("Failed to create BLE mutex");
        MCP_MutexDelete(s_wifi_state.mutex);
        return -2;
    }
    
    // Initialize Ethernet state
    memset(&s_ethernet_state, 0, sizeof(s_ethernet_state));
    s_ethernet_state.mutex = MCP_MutexCreate();
    if (s_ethernet_state.mutex <= 0) {
        log_error("Failed to create Ethernet mutex");
        MCP_MutexDelete(s_wifi_state.mutex);
        MCP_MutexDelete(s_ble_state.mutex);
        return -3;
    }
    
    // Set initial states
    s_wifi_state.status = MCP_WIFI_STATUS_DISCONNECTED;
    s_wifi_state.initialized = false;
    s_ble_state.initialized = false;
    s_ethernet_state.initialized = false;
    
    log_info("Network subsystem initialized successfully");
    return 0;
}

/**
 * @brief Initialize the MCP system for Raspberry Pi platform
 */
int MCP_SystemInit(const MCP_RPiConfig* config) {
    // Initialize platform (hardware and resource pools)
    int result = MCP_PlatformInit();
    if (result != 0) {
        log_error("Failed to initialize platform: %d", result);
        return result;
    }
    
    // Load persistent configuration from storage
    result = MCP_LoadPersistentState();
    if (result != 0) {
        log_warn("Failed to load persistent state: %d, using defaults", result);
        // Non-fatal, continue with default configuration
    }
    
    // If config parameter is provided, override persistent settings
    if (config != NULL) {
        // Lock config mutex
        int lock_result = MCP_MutexLock(s_config_mutex, 5000);
        if (lock_result != 0) {
            log_error("Failed to acquire config mutex: %d", lock_result);
            MCP_PlatformDeinit();
            return -1;
        }
        
        // Override with provided configuration
        strncpy(s_persistent_config.device_name, config->deviceName ? config->deviceName : "Raspberry Pi", 
                sizeof(s_persistent_config.device_name) - 1);
        
        if (config->version) {
            strncpy(s_persistent_config.firmware_version, config->version, 
                    sizeof(s_persistent_config.firmware_version) - 1);
        }
        
        s_persistent_config.debug_enabled = config->enableDebug;
        
        // Server configuration
        s_persistent_config.server_enabled = config->enableServer;
        s_persistent_config.server_port = config->serverPort;
        s_persistent_config.auto_start_server = config->autoStartServer;
        
        // WiFi configuration
        s_persistent_config.wifi_enabled = config->enableWifi;
        if (config->ssid) {
            strncpy(s_persistent_config.wifi_ssid, config->ssid, sizeof(s_persistent_config.wifi_ssid) - 1);
        }
        if (config->password) {
            strncpy(s_persistent_config.wifi_password, config->password, sizeof(s_persistent_config.wifi_password) - 1);
        }
        
        // WiFi AP configuration
        s_persistent_config.wifi_ap_enabled = config->enableHotspot;
        if (config->hotspotSsid) {
            strncpy(s_persistent_config.wifi_ap_ssid, config->hotspotSsid, sizeof(s_persistent_config.wifi_ap_ssid) - 1);
        }
        if (config->hotspotPassword) {
            strncpy(s_persistent_config.wifi_ap_password, config->hotspotPassword, 
                    sizeof(s_persistent_config.wifi_ap_password) - 1);
        }
        s_persistent_config.wifi_ap_channel = config->hotspotChannel;
        
        // BLE configuration
        s_persistent_config.ble_enabled = config->enableBLE;
        if (config->deviceName) {
            strncpy(s_persistent_config.ble_device_name, config->deviceName, 
                    sizeof(s_persistent_config.ble_device_name) - 1);
        }
        
        // Ethernet configuration
        s_persistent_config.ethernet_enabled = config->enableEthernet;
        if (config->ethernetInterface) {
            strncpy(s_persistent_config.ethernet_interface, config->ethernetInterface, 
                    sizeof(s_persistent_config.ethernet_interface) - 1);
        }
        
        // Interface configurations
        s_persistent_config.i2c_enabled = config->enableI2C;
        s_persistent_config.i2c_bus_number = config->i2cBusNumber;
        
        s_persistent_config.spi_enabled = config->enableSPI;
        s_persistent_config.spi_bus_number = config->spiBusNumber;
        
        s_persistent_config.uart_enabled = config->enableUART;
        s_persistent_config.uart_number = config->uartNumber;
        s_persistent_config.uart_baud_rate = config->uartBaudRate;
        
        s_persistent_config.gpio_enabled = config->enableGPIO;
        
        // System configurations
        s_persistent_config.heap_size = config->heapSize;
        if (config->configFile) {
            strncpy(s_persistent_config.config_file_path, config->configFile, 
                    sizeof(s_persistent_config.config_file_path) - 1);
        }
        
        // Unlock config mutex
        MCP_MutexUnlock(s_config_mutex);
        
        // Save the updated configuration
        MCP_SavePersistentState();
    }
    
    // Initialize network subsystem
    result = MCP_NetworkInit();
    if (result != 0) {
        log_error("Failed to initialize network subsystem: %d", result);
        MCP_PlatformDeinit();
        return result;
    }
    
    // Initialize sensor system
    result = MCP_SensorSystemInit();
    if (result != 0) {
        log_error("Failed to initialize sensor system: %d", result);
        MCP_PlatformDeinit();
        return result;
    }
    
    // Initialize server state
    if (s_server_state.mutex == 0) {
        s_server_state.mutex = MCP_MutexCreate();
        if (s_server_state.mutex < 0) {
            log_error("Failed to create server mutex");
            MCP_PlatformDeinit();
            return -2;
        }
    }
    
    // Configure server from persistent config
    s_server_state.port = s_persistent_config.server_port;
    s_server_state.initialized = true;
    
    // Initialize hardware interfaces based on configuration
    
    // Initialize I2C if enabled
    if (s_persistent_config.i2c_enabled) {
        log_info("Initializing I2C bus %d", s_persistent_config.i2c_bus_number);
        // Call I2C initialization functions
        // In a real implementation, we'd initialize I2C hardware here
        // For example: hal_rpi_i2c_init(s_persistent_config.i2c_bus_number, 100000);
    }
    
    // Initialize SPI if enabled
    if (s_persistent_config.spi_enabled) {
        log_info("Initializing SPI bus %d", s_persistent_config.spi_bus_number);
        // Call SPI initialization functions
        // In a real implementation, we'd initialize SPI hardware here
        // For example: hal_rpi_spi_init(s_persistent_config.spi_bus_number, 1000000, 0);
    }
    
    // Initialize UART if enabled
    if (s_persistent_config.uart_enabled) {
        log_info("Initializing UART %d at %d baud", s_persistent_config.uart_number, s_persistent_config.uart_baud_rate);
        result = hal_rpi_uart_init(s_persistent_config.uart_number, s_persistent_config.uart_baud_rate);
        if (result != 0) {
            log_warn("Failed to initialize UART: %d", result);
            // Non-fatal error, continue
        }
    }
    
    // Initialize WiFi if enabled and auto-connect is set
    if (s_persistent_config.wifi_enabled && s_persistent_config.wifi_auto_connect) {
        if (s_persistent_config.wifi_ssid[0] != '\0') {
            log_info("Auto-connecting to WiFi network '%s'", s_persistent_config.wifi_ssid);
            // Connect to WiFi with an extended timeout
            result = MCP_WiFiConnect(s_persistent_config.wifi_ssid, s_persistent_config.wifi_password, 30000);
            if (result != 0) {
                log_warn("Failed to connect to WiFi: %d", result);
                // Non-fatal error, continue
            }
        } else {
            log_warn("WiFi enabled but no SSID configured");
        }
    }
    
    // Initialize BLE if enabled
    if (s_persistent_config.ble_enabled) {
        log_info("Initializing Bluetooth LE");
        result = MCP_BLEInit(s_persistent_config.ble_device_name);
        if (result != 0) {
            log_warn("Failed to initialize BLE: %d", result);
            // Non-fatal error, continue
        } else if (s_persistent_config.ble_auto_advertise) {
            // Start advertising if auto-advertise is enabled
            result = MCP_BLEStartAdvertising();
            if (result != 0) {
                log_warn("Failed to start BLE advertising: %d", result);
                // Non-fatal error, continue
            }
        }
    }
    
    // Initialize Ethernet if enabled
    if (s_persistent_config.ethernet_enabled) {
        log_info("Initializing Ethernet interface '%s'", s_persistent_config.ethernet_interface);
        // Configure Ethernet with DHCP or static IP as configured
        result = MCP_EthernetConfigure(
            s_persistent_config.ethernet_interface,
            s_persistent_config.ethernet_dhcp,
            s_persistent_config.ethernet_dhcp ? NULL : s_persistent_config.ethernet_static_ip,
            s_persistent_config.ethernet_dhcp ? NULL : s_persistent_config.ethernet_gateway,
            s_persistent_config.ethernet_dhcp ? NULL : s_persistent_config.ethernet_subnet
        );
        if (result != 0) {
            log_warn("Failed to configure Ethernet: %d", result);
            // Non-fatal error, continue
        }
    }
    
    // Start WiFi hotspot if enabled
    if (s_persistent_config.wifi_ap_enabled) {
        if (s_persistent_config.wifi_ap_ssid[0] != '\0') {
            log_info("Starting WiFi hotspot '%s'", s_persistent_config.wifi_ap_ssid);
            result = MCP_WiFiStartAP(
                s_persistent_config.wifi_ap_ssid,
                s_persistent_config.wifi_ap_password,
                s_persistent_config.wifi_ap_channel
            );
            if (result != 0) {
                log_warn("Failed to start WiFi hotspot: %d", result);
                // Non-fatal error, continue
            }
        } else {
            log_warn("Hotspot enabled but no SSID configured");
        }
    }
    
    // Auto-start server if configured
    if (s_persistent_config.server_enabled && s_persistent_config.auto_start_server) {
        log_info("Auto-starting MCP server");
        result = MCP_ServerStart();
        if (result != 0) {
            log_warn("Failed to start MCP server: %d", result);
            // Non-fatal error, continue
        }
    }
    
    log_info("Raspberry Pi MCP system initialized successfully");
    return 0;
}

/**
 * @brief Configure and connect to WiFi network
 * 
 * This function configures and connects to a WiFi network on Raspberry Pi.
 * It creates a secure configuration file and uses system commands to
 * establish the connection.
 * 
 * @param ssid WiFi network SSID
 * @param password WiFi password (NULL for open networks)
 * @param timeout_ms Connection timeout in milliseconds
 * @return 0 on success, negative error code on failure
 */
int MCP_WiFiConnect(const char* ssid, const char* password, uint32_t timeout_ms) {
    FILE* wpa_conf = NULL;
    char config_path[128] = {0};
    int result = 0;
    
    // Validate parameters
    if (ssid == NULL) {
        log_error("WiFi connection failed: NULL SSID");
        return -1;
    }
    
    if (strlen(ssid) == 0) {
        log_error("WiFi connection failed: Empty SSID");
        return -2;
    }
    
    // Validate timeout
    if (timeout_ms == 0) {
        timeout_ms = 30000; // Default to 30 seconds if 0 is provided
    }
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi connection failed: WiFi subsystem not initialized");
        return -3;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 5000); // Use 5 second timeout for mutex
    if (lock_result != 0) {
        log_error("WiFi connection failed: Failed to acquire mutex: %d", lock_result);
        return -4;
    }
    
    // State protection - remember to unlock mutex in all exit paths
    
    // If already connected to the same network, just return success
    if (s_wifi_state.initialized && 
        s_wifi_state.status == MCP_WIFI_STATUS_CONNECTED &&
        strcmp(s_wifi_state.ssid, ssid) == 0) {
        
        log_info("Already connected to WiFi network '%s'", ssid);
        MCP_MutexUnlock(s_wifi_state.mutex);
        return 0;
    }
    
    // If already connecting, return busy
    if (s_wifi_state.status == MCP_WIFI_STATUS_CONNECTING) {
        log_error("WiFi connection failed: Already connecting to another network");
        MCP_MutexUnlock(s_wifi_state.mutex);
        return -5;
    }
    
    // Update status and store credentials safely
    s_wifi_state.status = MCP_WIFI_STATUS_CONNECTING;
    
    // Clear and copy SSID with bounds checking
    memset(s_wifi_state.ssid, 0, sizeof(s_wifi_state.ssid));
    strncpy(s_wifi_state.ssid, ssid, sizeof(s_wifi_state.ssid) - 1);
    
    // Clear and copy password with bounds checking
    memset(s_wifi_state.password, 0, sizeof(s_wifi_state.password));
    if (password != NULL) {
        strncpy(s_wifi_state.password, password, sizeof(s_wifi_state.password) - 1);
    }
    
    // Create a wpa_supplicant configuration file with secure permissions
    // Use a unique filename based on PID and timestamp to avoid collisions
    snprintf(config_path, sizeof(config_path), "/tmp/wpa_config_%d_%llu.conf", 
             getpid(), (unsigned long long)MCP_GetTimeMs());
    
    wpa_conf = fopen(config_path, "w");
    if (wpa_conf == NULL) {
        log_error("WiFi connection failed: Could not create config file: %s", strerror(errno));
        s_wifi_state.status = MCP_WIFI_STATUS_CONNECTION_FAILED;
        MCP_MutexUnlock(s_wifi_state.mutex);
        return -6;
    }
    
    // Set secure permissions on the config file (600)
    int chmod_result = chmod(config_path, S_IRUSR | S_IWUSR);
    if (chmod_result != 0) {
        log_warn("Failed to set secure permissions on WiFi config file: %s", strerror(errno));
        // Continue anyway, since this is not critical
    }
    
    // Write configuration
    fprintf(wpa_conf, "ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev\n");
    fprintf(wpa_conf, "update_config=1\n");
    fprintf(wpa_conf, "country=US\n\n");
    fprintf(wpa_conf, "network={\n");
    
    // Write SSID with proper escaping to prevent injection attacks
    fprintf(wpa_conf, "    ssid=\"");
    for (const char* c = ssid; *c != '\0'; c++) {
        if (*c == '"' || *c == '\\') {
            fputc('\\', wpa_conf);
        }
        fputc(*c, wpa_conf);
    }
    fprintf(wpa_conf, "\"\n");
    
    // Write password with proper escaping (if provided)
    if (password != NULL && password[0] != '\0') {
        fprintf(wpa_conf, "    psk=\"");
        for (const char* c = password; *c != '\0'; c++) {
            if (*c == '"' || *c == '\\') {
                fputc('\\', wpa_conf);
            }
            fputc(*c, wpa_conf);
        }
        fprintf(wpa_conf, "\"\n");
    } else {
        fprintf(wpa_conf, "    key_mgmt=NONE\n");
    }
    
    fprintf(wpa_conf, "}\n");
    
    // Ensure all data is written and close the file
    if (fflush(wpa_conf) != 0) {
        log_error("WiFi connection failed: Error flushing config file: %s", strerror(errno));
        result = -7;
        goto cleanup;
    }
    
    // Close the file properly
    fclose(wpa_conf);
    wpa_conf = NULL;
    
    log_info("Connecting to WiFi network '%s'...", ssid);
    
    // In a real implementation, we would:
    // 1. Kill any existing wpa_supplicant processes
    // 2. Start wpa_supplicant with the config file
    // 3. Run dhclient to get an IP address
    // 4. Parse ifconfig/ip addr to get the assigned IP
    // 5. Monitor the connection status
    
    // For now, simulate success with appropriate safeguards
    // We assume these operations succeeded
    
    // Update connection state
    s_wifi_state.status = MCP_WIFI_STATUS_CONNECTED;
    strncpy(s_wifi_state.ip_address, "192.168.1.100", sizeof(s_wifi_state.ip_address) - 1);
    s_wifi_state.rssi = -65; // Simulated RSSI
    s_wifi_state.initialized = true;
    
cleanup:
    // Clean up resources in all cases
    if (wpa_conf != NULL) {
        fclose(wpa_conf);
    }
    
    // Clean up the temporary configuration file
    if (config_path[0] != '\0' && unlink(config_path) != 0) {
        log_warn("Failed to remove temporary WiFi config file: %s", strerror(errno));
        // Non-fatal error, continue
    }
    
    // If there was an error, update the status
    if (result != 0) {
        s_wifi_state.status = MCP_WIFI_STATUS_CONNECTION_FAILED;
    }
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    
    return result;
}

/**
 * @brief Disconnect from WiFi network
 * 
 * @return 0 on success, negative error code on failure
 */
int MCP_WiFiDisconnect(void) {
    int result = 0;
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi disconnect failed: WiFi subsystem not initialized");
        return -1;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 5000);
    if (lock_result != 0) {
        log_error("WiFi disconnect failed: Failed to acquire mutex: %d", lock_result);
        return -2;
    }
    
    // Check if already disconnected
    if (!s_wifi_state.initialized || s_wifi_state.status == MCP_WIFI_STATUS_DISCONNECTED) {
        MCP_MutexUnlock(s_wifi_state.mutex);
        return 0; // Already disconnected
    }
    
    // If currently connecting, we should properly cancel that process
    if (s_wifi_state.status == MCP_WIFI_STATUS_CONNECTING) {
        log_info("Cancelling ongoing WiFi connection attempt...");
        // In a real implementation, this would kill any connecting process
    }
    
    // Execute system commands to disconnect
    log_info("Disconnecting from WiFi network '%s'...", s_wifi_state.ssid);
    
    // In a real implementation, this would:
    // 1. Kill wpa_supplicant processes
    // 2. Release DHCP leases
    // 3. Bring down the interface
    
    // Update state
    s_wifi_state.status = MCP_WIFI_STATUS_DISCONNECTED;
    memset(s_wifi_state.ip_address, 0, sizeof(s_wifi_state.ip_address));
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    return result;
}

/**
 * @brief Get current WiFi status
 * 
 * @return Current WiFi connection status, or MCP_WIFI_STATUS_DISCONNECTED if error
 */
MCP_WiFiStatus MCP_WiFiGetStatus(void) {
    MCP_WiFiStatus status = MCP_WIFI_STATUS_DISCONNECTED;
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi get status failed: WiFi subsystem not initialized");
        return status;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 1000);
    if (lock_result != 0) {
        log_error("WiFi get status failed: Failed to acquire mutex: %d", lock_result);
        return status;
    }
    
    // Get status safely
    status = s_wifi_state.status;
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    
    return status;
}

/**
 * @brief Get WiFi IP address as string
 * 
 * @param buffer Buffer to store the IP address
 * @param size Size of the buffer
 * @return Length of the IP address string or negative error code
 */
int MCP_WiFiGetIP(char* buffer, size_t size) {
    // Validate parameters
    if (buffer == NULL || size == 0) {
        log_error("WiFi get IP failed: Invalid buffer");
        return -1;
    }
    
    // Initialize buffer to empty string
    buffer[0] = '\0';
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi get IP failed: WiFi subsystem not initialized");
        return -2;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 1000);
    if (lock_result != 0) {
        log_error("WiFi get IP failed: Failed to acquire mutex: %d", lock_result);
        return -3;
    }
    
    // Check if connected
    if (!s_wifi_state.initialized || s_wifi_state.status != MCP_WIFI_STATUS_CONNECTED) {
        // Not connected, return empty string
        MCP_MutexUnlock(s_wifi_state.mutex);
        return 0;
    }
    
    // Copy IP address with bounds checking
    strncpy(buffer, s_wifi_state.ip_address, size - 1);
    buffer[size - 1] = '\0';
    
    // Get the length of the IP
    int length = strlen(buffer);
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    
    return length;
}

/**
 * @brief Get WiFi SSID
 * 
 * @param buffer Buffer to store the SSID
 * @param size Size of the buffer
 * @return Length of the SSID string or negative error code
 */
int MCP_WiFiGetSSID(char* buffer, size_t size) {
    // Validate parameters
    if (buffer == NULL || size == 0) {
        log_error("WiFi get SSID failed: Invalid buffer");
        return -1;
    }
    
    // Initialize buffer to empty string
    buffer[0] = '\0';
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi get SSID failed: WiFi subsystem not initialized");
        return -2;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 1000);
    if (lock_result != 0) {
        log_error("WiFi get SSID failed: Failed to acquire mutex: %d", lock_result);
        return -3;
    }
    
    // Check if connected or connecting
    if (!s_wifi_state.initialized || s_wifi_state.status == MCP_WIFI_STATUS_DISCONNECTED) {
        // Not connected, return empty string
        MCP_MutexUnlock(s_wifi_state.mutex);
        return 0;
    }
    
    // Copy SSID with bounds checking
    strncpy(buffer, s_wifi_state.ssid, size - 1);
    buffer[size - 1] = '\0';
    
    // Get the length of the SSID
    int length = strlen(buffer);
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    
    return length;
}

/**
 * @brief Get WiFi signal strength (RSSI)
 * 
 * @return RSSI value in dBm or 0 if not connected, or negative error code
 */
int MCP_WiFiGetRSSI(void) {
    int rssi = 0;
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi get RSSI failed: WiFi subsystem not initialized");
        return -1;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 1000);
    if (lock_result != 0) {
        log_error("WiFi get RSSI failed: Failed to acquire mutex: %d", lock_result);
        return -2;
    }
    
    // Check if connected
    if (!s_wifi_state.initialized || s_wifi_state.status != MCP_WIFI_STATUS_CONNECTED) {
        // Not connected, return 0
        MCP_MutexUnlock(s_wifi_state.mutex);
        return 0;
    }
    
    // Get RSSI safely
    rssi = s_wifi_state.rssi;
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    
    return rssi;
}

/**
 * @brief Start WiFi in AP (hotspot) mode
 * 
 * @param ssid SSID for the access point
 * @param password Password for the access point (NULL for open networks)
 * @param channel WiFi channel to use (1-14)
 * @return 0 on success, negative error code on failure
 */
int MCP_WiFiStartAP(const char* ssid, const char* password, uint16_t channel) {
    int result = 0;
    
    // Validate parameters
    if (ssid == NULL) {
        log_error("WiFi start AP failed: NULL SSID");
        return -1;
    }
    
    if (strlen(ssid) == 0) {
        log_error("WiFi start AP failed: Empty SSID");
        return -2;
    }
    
    // Validate channel
    if (channel < 1 || channel > 14) {
        log_error("WiFi start AP failed: Invalid channel %d (must be 1-14)", channel);
        return -3;
    }
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi start AP failed: WiFi subsystem not initialized");
        return -4;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 5000);
    if (lock_result != 0) {
        log_error("WiFi start AP failed: Failed to acquire mutex: %d", lock_result);
        return -5;
    }
    
    // Check if already in AP mode
    if (s_wifi_state.ap_mode) {
        log_warn("WiFi AP mode already active, stopping first");
        // In a real implementation, we would stop the current AP mode first
    }
    
    // Store AP credentials safely with bounds checking
    memset(s_wifi_state.ap_ssid, 0, sizeof(s_wifi_state.ap_ssid));
    strncpy(s_wifi_state.ap_ssid, ssid, sizeof(s_wifi_state.ap_ssid) - 1);
    s_wifi_state.ap_mode = true;
    
    // Execute system commands to start AP mode
    log_info("Starting WiFi AP with SSID '%s' on channel %d...", ssid, channel);
    
    // In a real implementation, this would:
    // 1. Create a hostapd configuration file
    // 2. Set up dnsmasq for DHCP
    // 3. Configure interface as AP mode
    // 4. Start hostapd and dnsmasq services
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    
    return result;
}

/**
 * @brief Stop WiFi AP mode
 * 
 * @return 0 on success, negative error code on failure
 */
int MCP_WiFiStopAP(void) {
    int result = 0;
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi stop AP failed: WiFi subsystem not initialized");
        return -1;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 5000);
    if (lock_result != 0) {
        log_error("WiFi stop AP failed: Failed to acquire mutex: %d", lock_result);
        return -2;
    }
    
    // Check if in AP mode
    if (!s_wifi_state.ap_mode) {
        log_info("WiFi is not in AP mode, nothing to stop");
        MCP_MutexUnlock(s_wifi_state.mutex);
        return 0; // Not in AP mode
    }
    
    // Execute system commands to stop AP mode
    log_info("Stopping WiFi AP with SSID '%s'...", s_wifi_state.ap_ssid);
    
    // In a real implementation, this would:
    // 1. Stop hostapd and dnsmasq services
    // 2. Reconfigure interface for normal station mode
    // 3. Clean up configuration files
    
    // Update state
    s_wifi_state.ap_mode = false;
    memset(s_wifi_state.ap_ssid, 0, sizeof(s_wifi_state.ap_ssid));
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    
    return result;
}

/**
 * @brief Check if WiFi hotspot is active
 * 
 * @return true if AP mode is active, false otherwise
 */
bool MCP_WiFiIsAPActive(void) {
    bool ap_active = false;
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi is AP active check failed: WiFi subsystem not initialized");
        return false;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 1000);
    if (lock_result != 0) {
        log_error("WiFi is AP active check failed: Failed to acquire mutex: %d", lock_result);
        return false;
    }
    
    // Get AP mode state safely
    ap_active = s_wifi_state.ap_mode;
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    
    return ap_active;
}

/**
 * @brief Scan for available WiFi networks
 * 
 * Returns a JSON array of available networks with SSID, signal strength, and security info
 * 
 * @param buffer Buffer to store the JSON result
 * @param size Size of the buffer
 * @return Length of the JSON result string or negative error code
 */
int MCP_WiFiScan(char* buffer, size_t size) {
    // Validate parameters
    if (buffer == NULL || size == 0) {
        log_error("WiFi scan failed: Invalid buffer");
        return -1;
    }
    
    // Initialize buffer to empty string
    buffer[0] = '\0';
    
    // Check if WiFi subsystem is initialized
    if (s_wifi_state.mutex <= 0) {
        log_error("WiFi scan failed: WiFi subsystem not initialized");
        return -2;
    }
    
    // Lock WiFi mutex with appropriate timeout
    int lock_result = MCP_MutexLock(s_wifi_state.mutex, 5000); // Use longer timeout for scan
    if (lock_result != 0) {
        log_error("WiFi scan failed: Failed to acquire mutex: %d", lock_result);
        return -3;
    }
    
    log_info("Scanning for available WiFi networks...");
    
    // In a real implementation, this would:
    // 1. Use iwlist/iw command to scan for networks
    // 2. Parse the output to extract SSIDs, signal strengths, etc.
    // 3. Format the results as a JSON array
    
    // For now, return a simulated result
    const char* simulated_scan = 
        "["
        "{"
            "\"ssid\":\"HomeNetwork\","
            "\"rssi\":-65,"
            "\"security\":\"WPA2\","
            "\"channel\":6"
        "},"
        "{"
            "\"ssid\":\"OfficeWiFi\","
            "\"rssi\":-72,"
            "\"security\":\"WPA2\","
            "\"channel\":11"
        "},"
        "{"
            "\"ssid\":\"GuestNetwork\","
            "\"rssi\":-80,"
            "\"security\":\"Open\","
            "\"channel\":1"
        "}"
        "]";
    
    // Copy result with bounds checking
    strncpy(buffer, simulated_scan, size - 1);
    buffer[size - 1] = '\0';
    
    // Get the length of the result
    int length = strlen(buffer);
    
    // Always unlock the mutex before returning
    MCP_MutexUnlock(s_wifi_state.mutex);
    
    return length;
}

/**
 * @brief Initialize BLE
 */
int MCP_BLEInit(const char* deviceName) {
    if (deviceName == NULL) {
        return -1;
    }
    
    // Lock BLE mutex
    MCP_MutexLock(s_ble_state.mutex, 1000);
    
    // Store device name
    strncpy(s_ble_state.device_name, deviceName, sizeof(s_ble_state.device_name) - 1);
    
    // Initialize BLE
    log_info("Initializing BLE with device name '%s'...", deviceName);
    
    // In a real implementation, this would initialize the Bluetooth
    // adapter using system commands or libraries
    
    s_ble_state.initialized = true;
    s_ble_state.advertising = false;
    s_ble_state.connected = false;
    
    MCP_MutexUnlock(s_ble_state.mutex);
    return 0;
}

/**
 * @brief Start BLE advertising
 */
int MCP_BLEStartAdvertising(void) {
    // Lock BLE mutex
    MCP_MutexLock(s_ble_state.mutex, 1000);
    
    if (!s_ble_state.initialized) {
        log_error("BLE not initialized");
        MCP_MutexUnlock(s_ble_state.mutex);
        return -1;
    }
    
    // Start advertising
    log_info("Starting BLE advertising...");
    
    // In a real implementation, this would use system commands
    // to start BLE advertising
    
    s_ble_state.advertising = true;
    
    MCP_MutexUnlock(s_ble_state.mutex);
    return 0;
}

/**
 * @brief Stop BLE advertising
 */
int MCP_BLEStopAdvertising(void) {
    // Lock BLE mutex
    MCP_MutexLock(s_ble_state.mutex, 1000);
    
    if (!s_ble_state.initialized || !s_ble_state.advertising) {
        MCP_MutexUnlock(s_ble_state.mutex);
        return 0; // Not advertising
    }
    
    // Stop advertising
    log_info("Stopping BLE advertising...");
    
    // In a real implementation, this would use system commands
    // to stop BLE advertising
    
    s_ble_state.advertising = false;
    
    MCP_MutexUnlock(s_ble_state.mutex);
    return 0;
}

/**
 * @brief Check if BLE is connected
 */
bool MCP_BLEIsConnected(void) {
    bool connected;
    
    // Lock BLE mutex
    MCP_MutexLock(s_ble_state.mutex, 1000);
    connected = s_ble_state.connected;
    MCP_MutexUnlock(s_ble_state.mutex);
    
    return connected;
}

/**
 * @brief Configure Ethernet
 */
int MCP_EthernetConfigure(const char* interface, bool useDHCP, const char* staticIP, 
                          const char* gateway, const char* subnet) {
    if (interface == NULL) {
        return -1;
    }
    
    // Lock Ethernet mutex
    MCP_MutexLock(s_ethernet_state.mutex, 1000);
    
    // Store configuration
    strncpy(s_ethernet_state.interface, interface, sizeof(s_ethernet_state.interface) - 1);
    s_ethernet_state.dhcp = useDHCP;
    
    if (!useDHCP && staticIP != NULL) {
        strncpy(s_ethernet_state.static_ip, staticIP, sizeof(s_ethernet_state.static_ip) - 1);
        
        if (gateway != NULL) {
            strncpy(s_ethernet_state.gateway, gateway, sizeof(s_ethernet_state.gateway) - 1);
        }
        
        if (subnet != NULL) {
            strncpy(s_ethernet_state.subnet, subnet, sizeof(s_ethernet_state.subnet) - 1);
        }
    }
    
    // Configure Ethernet
    log_info("Configuring Ethernet interface '%s'...", interface);
    
    // In a real implementation, this would use system commands
    // to configure the Ethernet interface
    
    s_ethernet_state.initialized = true;
    s_ethernet_state.connected = true;
    strcpy(s_ethernet_state.ip_address, useDHCP ? "192.168.1.101" : staticIP);
    
    MCP_MutexUnlock(s_ethernet_state.mutex);
    return 0;
}

/**
 * @brief Get current Ethernet status
 */
int MCP_EthernetGetStatus(void) {
    int status;
    
    // Lock Ethernet mutex
    MCP_MutexLock(s_ethernet_state.mutex, 1000);
    status = s_ethernet_state.connected ? 1 : 0;
    MCP_MutexUnlock(s_ethernet_state.mutex);
    
    return status;
}

/**
 * @brief Get Ethernet IP address as string
 */
int MCP_EthernetGetIP(char* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return -1;
    }
    
    // Lock Ethernet mutex
    MCP_MutexLock(s_ethernet_state.mutex, 1000);
    
    if (!s_ethernet_state.initialized || !s_ethernet_state.connected) {
        buffer[0] = '\0';
        MCP_MutexUnlock(s_ethernet_state.mutex);
        return 0;
    }
    
    // Copy IP address
    strncpy(buffer, s_ethernet_state.ip_address, size - 1);
    buffer[size - 1] = '\0';
    
    MCP_MutexUnlock(s_ethernet_state.mutex);
    return strlen(buffer);
}

/**
 * @brief MCP server state
 */
static struct {
    bool initialized;
    bool running;
    uint16_t port;
    MCP_MutexHandle mutex;
} s_server_state;

/**
 * @brief Start the MCP server
 */
int MCP_ServerStart(void) {
    // Lock server mutex
    if (s_server_state.mutex == 0) {
        s_server_state.mutex = MCP_MutexCreate();
    }
    
    MCP_MutexLock(s_server_state.mutex, 1000);
    
    if (s_server_state.running) {
        // Already running
        MCP_MutexUnlock(s_server_state.mutex);
        return 0;
    }
    
    // Start the server
    log_info("Starting MCP server on port %d", s_server_state.port);
    
    // In a real implementation, this would start an TCP/IP server
    // for handling MCP protocol messages
    
    s_server_state.initialized = true;
    s_server_state.running = true;
    
    MCP_MutexUnlock(s_server_state.mutex);
    return 0;
}

/**
 * @brief Serialize the config to JSON
 * 
 * @param buffer Buffer to store the JSON string
 * @param size Size of the buffer
 * @return int Length of the JSON string or negative error code
 */
static int SerializeConfigToJSON(char* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return -1;
    }
    
    int pos = 0;
    pos += snprintf(buffer + pos, size - pos, "{\n");
    
    // Device information
    pos += snprintf(buffer + pos, size - pos,
        "  \"device\": {\n"
        "    \"name\": \"%s\",\n"
        "    \"firmware_version\": \"%s\",\n"
        "    \"debug_enabled\": %s\n"
        "  },\n",
        s_persistent_config.device_name,
        s_persistent_config.firmware_version,
        s_persistent_config.debug_enabled ? "true" : "false"
    );
    
    // Server configuration
    pos += snprintf(buffer + pos, size - pos,
        "  \"server\": {\n"
        "    \"enabled\": %s,\n"
        "    \"port\": %u,\n"
        "    \"auto_start\": %s\n"
        "  },\n",
        s_persistent_config.server_enabled ? "true" : "false",
        s_persistent_config.server_port,
        s_persistent_config.auto_start_server ? "true" : "false"
    );
    
    // WiFi configuration
    pos += snprintf(buffer + pos, size - pos,
        "  \"wifi\": {\n"
        "    \"enabled\": %s,\n"
        "    \"ssid\": \"%s\",\n"
        "    \"password\": \"%s\",\n"
        "    \"auto_connect\": %s\n"
        "  },\n",
        s_persistent_config.wifi_enabled ? "true" : "false",
        s_persistent_config.wifi_ssid,
        s_persistent_config.wifi_password,
        s_persistent_config.wifi_auto_connect ? "true" : "false"
    );
    
    // WiFi AP configuration
    pos += snprintf(buffer + pos, size - pos,
        "  \"wifi_ap\": {\n"
        "    \"enabled\": %s,\n"
        "    \"ssid\": \"%s\",\n"
        "    \"password\": \"%s\",\n"
        "    \"channel\": %u\n"
        "  },\n",
        s_persistent_config.wifi_ap_enabled ? "true" : "false",
        s_persistent_config.wifi_ap_ssid,
        s_persistent_config.wifi_ap_password,
        s_persistent_config.wifi_ap_channel
    );
    
    // BLE configuration
    pos += snprintf(buffer + pos, size - pos,
        "  \"ble\": {\n"
        "    \"enabled\": %s,\n"
        "    \"device_name\": \"%s\",\n"
        "    \"auto_advertise\": %s\n"
        "  },\n",
        s_persistent_config.ble_enabled ? "true" : "false",
        s_persistent_config.ble_device_name,
        s_persistent_config.ble_auto_advertise ? "true" : "false"
    );
    
    // Ethernet configuration
    pos += snprintf(buffer + pos, size - pos,
        "  \"ethernet\": {\n"
        "    \"enabled\": %s,\n"
        "    \"interface\": \"%s\",\n"
        "    \"dhcp\": %s,\n"
        "    \"static_ip\": \"%s\",\n"
        "    \"gateway\": \"%s\",\n"
        "    \"subnet\": \"%s\"\n"
        "  },\n",
        s_persistent_config.ethernet_enabled ? "true" : "false",
        s_persistent_config.ethernet_interface,
        s_persistent_config.ethernet_dhcp ? "true" : "false",
        s_persistent_config.ethernet_static_ip,
        s_persistent_config.ethernet_gateway,
        s_persistent_config.ethernet_subnet
    );
    
    // Interface configurations
    pos += snprintf(buffer + pos, size - pos,
        "  \"interfaces\": {\n"
        "    \"i2c\": {\n"
        "      \"enabled\": %s,\n"
        "      \"bus_number\": %d\n"
        "    },\n"
        "    \"spi\": {\n"
        "      \"enabled\": %s,\n"
        "      \"bus_number\": %d\n"
        "    },\n"
        "    \"uart\": {\n"
        "      \"enabled\": %s,\n"
        "      \"number\": %d,\n"
        "      \"baud_rate\": %u\n"
        "    },\n"
        "    \"gpio\": {\n"
        "      \"enabled\": %s\n"
        "    }\n"
        "  },\n",
        s_persistent_config.i2c_enabled ? "true" : "false",
        s_persistent_config.i2c_bus_number,
        s_persistent_config.spi_enabled ? "true" : "false",
        s_persistent_config.spi_bus_number,
        s_persistent_config.uart_enabled ? "true" : "false",
        s_persistent_config.uart_number,
        s_persistent_config.uart_baud_rate,
        s_persistent_config.gpio_enabled ? "true" : "false"
    );
    
    // System configurations
    pos += snprintf(buffer + pos, size - pos,
        "  \"system\": {\n"
        "    \"heap_size\": %u,\n"
        "    \"config_file_path\": \"%s\"\n"
        "  }\n",
        s_persistent_config.heap_size,
        s_persistent_config.config_file_path
    );
    
    pos += snprintf(buffer + pos, size - pos, "}\n");
    
    return pos;
}

/**
 * @brief Parse JSON into configuration
 * 
 * @param json JSON string to parse
 * @return int 0 on success, negative error code on failure
 */
static int ParseJSONConfig(const char* json) {
    if (json == NULL) {
        return -1;
    }
    
    // Use json_parser.h functions to extract values
    // This is a simplified version - in a real implementation,
    // we would use a proper JSON parser library
    
    // Device information
    char* device_name = json_get_string_field(json, "device.name");
    if (device_name != NULL) {
        strncpy(s_persistent_config.device_name, device_name, sizeof(s_persistent_config.device_name) - 1);
        free(device_name);
    }
    
    char* firmware_version = json_get_string_field(json, "device.firmware_version");
    if (firmware_version != NULL) {
        strncpy(s_persistent_config.firmware_version, firmware_version, sizeof(s_persistent_config.firmware_version) - 1);
        free(firmware_version);
    }
    
    s_persistent_config.debug_enabled = json_get_bool_field(json, "device.debug_enabled", s_persistent_config.debug_enabled);
    
    // Server configuration
    s_persistent_config.server_enabled = json_get_bool_field(json, "server.enabled", s_persistent_config.server_enabled);
    s_persistent_config.server_port = json_get_int_field(json, "server.port", s_persistent_config.server_port);
    s_persistent_config.auto_start_server = json_get_bool_field(json, "server.auto_start", s_persistent_config.auto_start_server);
    
    // WiFi configuration
    s_persistent_config.wifi_enabled = json_get_bool_field(json, "wifi.enabled", s_persistent_config.wifi_enabled);
    
    char* wifi_ssid = json_get_string_field(json, "wifi.ssid");
    if (wifi_ssid != NULL) {
        strncpy(s_persistent_config.wifi_ssid, wifi_ssid, sizeof(s_persistent_config.wifi_ssid) - 1);
        free(wifi_ssid);
    }
    
    char* wifi_password = json_get_string_field(json, "wifi.password");
    if (wifi_password != NULL) {
        strncpy(s_persistent_config.wifi_password, wifi_password, sizeof(s_persistent_config.wifi_password) - 1);
        free(wifi_password);
    }
    
    s_persistent_config.wifi_auto_connect = json_get_bool_field(json, "wifi.auto_connect", s_persistent_config.wifi_auto_connect);
    
    // WiFi AP configuration
    s_persistent_config.wifi_ap_enabled = json_get_bool_field(json, "wifi_ap.enabled", s_persistent_config.wifi_ap_enabled);
    
    char* wifi_ap_ssid = json_get_string_field(json, "wifi_ap.ssid");
    if (wifi_ap_ssid != NULL) {
        strncpy(s_persistent_config.wifi_ap_ssid, wifi_ap_ssid, sizeof(s_persistent_config.wifi_ap_ssid) - 1);
        free(wifi_ap_ssid);
    }
    
    char* wifi_ap_password = json_get_string_field(json, "wifi_ap.password");
    if (wifi_ap_password != NULL) {
        strncpy(s_persistent_config.wifi_ap_password, wifi_ap_password, sizeof(s_persistent_config.wifi_ap_password) - 1);
        free(wifi_ap_password);
    }
    
    s_persistent_config.wifi_ap_channel = json_get_int_field(json, "wifi_ap.channel", s_persistent_config.wifi_ap_channel);
    
    // BLE configuration
    s_persistent_config.ble_enabled = json_get_bool_field(json, "ble.enabled", s_persistent_config.ble_enabled);
    
    char* ble_device_name = json_get_string_field(json, "ble.device_name");
    if (ble_device_name != NULL) {
        strncpy(s_persistent_config.ble_device_name, ble_device_name, sizeof(s_persistent_config.ble_device_name) - 1);
        free(ble_device_name);
    }
    
    s_persistent_config.ble_auto_advertise = json_get_bool_field(json, "ble.auto_advertise", s_persistent_config.ble_auto_advertise);
    
    // Ethernet configuration
    s_persistent_config.ethernet_enabled = json_get_bool_field(json, "ethernet.enabled", s_persistent_config.ethernet_enabled);
    
    char* ethernet_interface = json_get_string_field(json, "ethernet.interface");
    if (ethernet_interface != NULL) {
        strncpy(s_persistent_config.ethernet_interface, ethernet_interface, sizeof(s_persistent_config.ethernet_interface) - 1);
        free(ethernet_interface);
    }
    
    s_persistent_config.ethernet_dhcp = json_get_bool_field(json, "ethernet.dhcp", s_persistent_config.ethernet_dhcp);
    
    char* ethernet_static_ip = json_get_string_field(json, "ethernet.static_ip");
    if (ethernet_static_ip != NULL) {
        strncpy(s_persistent_config.ethernet_static_ip, ethernet_static_ip, sizeof(s_persistent_config.ethernet_static_ip) - 1);
        free(ethernet_static_ip);
    }
    
    char* ethernet_gateway = json_get_string_field(json, "ethernet.gateway");
    if (ethernet_gateway != NULL) {
        strncpy(s_persistent_config.ethernet_gateway, ethernet_gateway, sizeof(s_persistent_config.ethernet_gateway) - 1);
        free(ethernet_gateway);
    }
    
    char* ethernet_subnet = json_get_string_field(json, "ethernet.subnet");
    if (ethernet_subnet != NULL) {
        strncpy(s_persistent_config.ethernet_subnet, ethernet_subnet, sizeof(s_persistent_config.ethernet_subnet) - 1);
        free(ethernet_subnet);
    }
    
    // Interface configurations
    s_persistent_config.i2c_enabled = json_get_bool_field(json, "interfaces.i2c.enabled", s_persistent_config.i2c_enabled);
    s_persistent_config.i2c_bus_number = json_get_int_field(json, "interfaces.i2c.bus_number", s_persistent_config.i2c_bus_number);
    
    s_persistent_config.spi_enabled = json_get_bool_field(json, "interfaces.spi.enabled", s_persistent_config.spi_enabled);
    s_persistent_config.spi_bus_number = json_get_int_field(json, "interfaces.spi.bus_number", s_persistent_config.spi_bus_number);
    
    s_persistent_config.uart_enabled = json_get_bool_field(json, "interfaces.uart.enabled", s_persistent_config.uart_enabled);
    s_persistent_config.uart_number = json_get_int_field(json, "interfaces.uart.number", s_persistent_config.uart_number);
    s_persistent_config.uart_baud_rate = json_get_int_field(json, "interfaces.uart.baud_rate", s_persistent_config.uart_baud_rate);
    
    s_persistent_config.gpio_enabled = json_get_bool_field(json, "interfaces.gpio.enabled", s_persistent_config.gpio_enabled);
    
    // System configurations
    s_persistent_config.heap_size = json_get_int_field(json, "system.heap_size", s_persistent_config.heap_size);
    
    char* config_file_path = json_get_string_field(json, "system.config_file_path");
    if (config_file_path != NULL) {
        strncpy(s_persistent_config.config_file_path, config_file_path, sizeof(s_persistent_config.config_file_path) - 1);
        free(config_file_path);
    }
    
    return 0;
}

/**
 * @brief Load persistent state from storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_LoadPersistentState(void) {
    if (!s_config_initialized) {
        log_error("Configuration not initialized");
        return -1;
    }
    
    // Lock config mutex
    int lock_result = MCP_MutexLock(s_config_mutex, 5000);
    if (lock_result != 0) {
        log_error("Failed to acquire config mutex: %d", lock_result);
        return -2;
    }
    
    log_info("Loading persistent state from %s", s_persistent_config.config_file_path);
    
    // Check if config file exists
    if (!MCP_FileExists(s_persistent_config.config_file_path)) {
        log_warn("Configuration file does not exist, using defaults");
        MCP_MutexUnlock(s_config_mutex);
        return 0;
    }
    
    // Open config file
    MCP_FileHandle file = MCP_FileOpen(s_persistent_config.config_file_path, MCP_FILE_MODE_READ);
    if (file == NULL) {
        log_error("Failed to open configuration file");
        MCP_MutexUnlock(s_config_mutex);
        return -3;
    }
    
    // Read config file
    char* json_buffer = (char*)malloc(16384); // 16KB buffer for config
    if (json_buffer == NULL) {
        log_error("Failed to allocate memory for config file");
        MCP_FileClose(file);
        MCP_MutexUnlock(s_config_mutex);
        return -4;
    }
    
    size_t bytes_read = MCP_FileRead(file, json_buffer, 16383);
    json_buffer[bytes_read] = '\0'; // Null-terminate the string
    
    // Close the file
    MCP_FileClose(file);
    
    // Parse JSON config
    int result = ParseJSONConfig(json_buffer);
    free(json_buffer);
    
    if (result != 0) {
        log_error("Failed to parse configuration JSON: %d", result);
        MCP_MutexUnlock(s_config_mutex);
        return -5;
    }
    
    log_info("Persistent state loaded successfully");
    
    // Unlock config mutex
    MCP_MutexUnlock(s_config_mutex);
    
    return 0;
}

/**
 * @brief Save persistent state to storage
 * 
 * @return int 0 on success, negative error code on failure
 */
int MCP_SavePersistentState(void) {
    if (!s_config_initialized) {
        log_error("Configuration not initialized");
        return -1;
    }
    
    // Lock config mutex
    int lock_result = MCP_MutexLock(s_config_mutex, 5000);
    if (lock_result != 0) {
        log_error("Failed to acquire config mutex: %d", lock_result);
        return -2;
    }
    
    log_info("Saving persistent state to %s", s_persistent_config.config_file_path);
    
    // Create directory if it doesn't exist
    char dir_path[128];
    strncpy(dir_path, s_persistent_config.config_file_path, sizeof(dir_path) - 1);
    
    // Find the last slash in the path
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0'; // Truncate the string at the last slash
        
        // Create the directory if it doesn't exist
        if (!MCP_FileExists(dir_path)) {
            int result = MCP_DirCreate(dir_path);
            if (result != 0) {
                log_error("Failed to create directory for config file: %d", result);
                MCP_MutexUnlock(s_config_mutex);
                return -3;
            }
        }
    }
    
    // Open config file
    MCP_FileHandle file = MCP_FileOpen(s_persistent_config.config_file_path, MCP_FILE_MODE_WRITE);
    if (file == NULL) {
        log_error("Failed to open configuration file for writing");
        MCP_MutexUnlock(s_config_mutex);
        return -4;
    }
    
    // Serialize config to JSON
    char* json_buffer = (char*)malloc(16384); // 16KB buffer for config
    if (json_buffer == NULL) {
        log_error("Failed to allocate memory for config JSON");
        MCP_FileClose(file);
        MCP_MutexUnlock(s_config_mutex);
        return -5;
    }
    
    int json_length = SerializeConfigToJSON(json_buffer, 16384);
    if (json_length <= 0) {
        log_error("Failed to serialize configuration to JSON: %d", json_length);
        free(json_buffer);
        MCP_FileClose(file);
        MCP_MutexUnlock(s_config_mutex);
        return -6;
    }
    
    // Write config to file
    size_t bytes_written = MCP_FileWrite(file, json_buffer, json_length);
    free(json_buffer);
    
    // Close the file
    MCP_FileClose(file);
    
    if (bytes_written != (size_t)json_length) {
        log_error("Failed to write configuration file: %zu of %d bytes written", bytes_written, json_length);
        MCP_MutexUnlock(s_config_mutex);
        return -7;
    }
    
    log_info("Persistent state saved successfully");
    
    // Unlock config mutex
    MCP_MutexUnlock(s_config_mutex);
    
    return 0;
}

/**
 * @brief Process system tasks
 */
int MCP_SystemProcess(uint32_t timeout_ms) {
    // Process system tasks
    
    // In a real implementation, this would handle network events,
    // process incoming MCP messages, and run other background tasks
    
    // Process sensors
    
    // Process networking events
    
    // Delay for a bit to prevent CPU hogging
    if (timeout_ms > 0) {
        usleep(timeout_ms * 1000);
    }
    
    return 0;
}

/**
 * @brief Deinitialize the MCP system
 */
int MCP_SystemDeinit(void) {
    // Stop server
    if (s_server_state.running) {
        log_info("Stopping MCP server");
        s_server_state.running = false;
    }
    
    // Save state
    MCP_SavePersistentState();
    
    // Deinitialize platform
    MCP_PlatformDeinit();
    
    return 0;
}

/**
 * @brief Get system status as JSON
 */
int MCP_SystemGetStatus(char* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return -1;
    }
    
    // Get system status
    uint32_t total_mem = 0;
    uint32_t free_mem = 0;
    hal_rpi_get_memory_info(&total_mem, &free_mem);
    
    // Format as JSON
    int pos = snprintf(buffer, size,
        "{"
        "\"device\":\"Raspberry Pi\","
        "\"memory\":{"
            "\"total\":%u,"
            "\"free\":%u,"
            "\"used\":%u"
        "},"
        "\"temperature\":%.1f,"
        "\"uptime\":%u,"
        "\"network\":{"
            "\"wifi\":%s,"
            "\"ethernet\":%s,"
            "\"bluetooth\":%s"
        "}"
        "}",
        total_mem, free_mem, total_mem - free_mem,
        hal_rpi_get_temperature(),
        (uint32_t)(hal_rpi_get_time_ms() / 1000),
        s_wifi_state.status == MCP_WIFI_STATUS_CONNECTED ? "true" : "false",
        s_ethernet_state.connected ? "true" : "false",
        s_ble_state.connected ? "true" : "false"
    );
    
    return (pos < 0 || (size_t)pos >= size) ? -2 : pos;
}

/**
 * @brief Set debug output enable state
 */
int MCP_SystemSetDebug(bool enable) {
    // Enable/disable debug output
    log_set_level(enable ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO);
    
    return 0;
}

/**
 * @brief Print debug message
 */
void MCP_SystemDebugPrint(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Print debug message
    log_debugv("Debug", format, args);
    
    va_end(args);
}

/**
 * @brief Restart the Raspberry Pi
 */
void MCP_Restart(void) {
    // Save state before reboot
    MCP_SavePersistentState();
    
    // In a real implementation, this would reboot the system
    log_info("Restarting system...");
    
    // Simulated reboot for now
    exit(0);
}