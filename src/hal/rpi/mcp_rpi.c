/**
 * @file mcp_rpi.c
 * @brief MCP implementation for bare metal Raspberry Pi
 */
#include "mcp_rpi.h"
#include "hal_rpi.h"
#include "../../logging.h"
#include <stdlib.h>
#include <string.h>

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
 */
int MCP_PlatformInit(void) {
    if (s_platform_initialized) {
        return -1; // Already initialized
    }
    
    // Initialize HAL
    int result = hal_rpi_init();
    if (result != 0) {
        log_error("Failed to initialize Raspberry Pi HAL");
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
    
    s_platform_initialized = true;
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
    
    // Deinitialize HAL
    hal_rpi_deinit();
    
    s_platform_initialized = false;
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
    // Return a placeholder string
    return "Raspberry Pi Bare Metal MCP";
}