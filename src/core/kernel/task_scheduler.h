#ifndef MCP_TASK_SCHEDULER_H
#define MCP_TASK_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Task priority levels
 */
typedef enum {
    MCP_TASK_PRIORITY_LOW,
    MCP_TASK_PRIORITY_NORMAL,
    MCP_TASK_PRIORITY_HIGH,
    MCP_TASK_PRIORITY_CRITICAL
} MCP_TaskPriority;

/**
 * @brief Task function type
 */
typedef void (*MCP_TaskFunction)(void* param);

/**
 * @brief Task structure
 */
typedef struct {
    uint32_t id;
    MCP_TaskFunction function;
    void* param;
    uint32_t interval;
    uint32_t lastRun;
    MCP_TaskPriority priority;
    bool enabled;
} MCP_Task;

/**
 * @brief Initialize the task scheduler
 * 
 * @param maxTasks Maximum number of tasks that can be scheduled
 * @return int 0 on success, negative error code on failure
 */
int MCP_TaskSchedulerInit(uint16_t maxTasks);

/**
 * @brief Create a new task
 * 
 * @param function Task function to execute
 * @param param Parameter to pass to the task function
 * @param interval Task execution interval in milliseconds (0 for run once)
 * @param priority Task priority
 * @return uint32_t Task ID or 0 on failure
 */
uint32_t MCP_TaskCreate(MCP_TaskFunction function, void* param, uint32_t interval, MCP_TaskPriority priority);

/**
 * @brief Set task enabled state
 * 
 * @param taskId Task ID
 * @param enabled Enabled state
 * @return int 0 on success, negative error code on failure
 */
int MCP_TaskSetEnabled(uint32_t taskId, bool enabled);

/**
 * @brief Delete a task
 * 
 * @param taskId Task ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_TaskDelete(uint32_t taskId);

/**
 * @brief Process due tasks
 * 
 * @param currentTimeMs Current system time in milliseconds
 * @return int Number of tasks executed
 */
int MCP_TaskProcess(uint32_t currentTimeMs);

#endif /* MCP_TASK_SCHEDULER_H */