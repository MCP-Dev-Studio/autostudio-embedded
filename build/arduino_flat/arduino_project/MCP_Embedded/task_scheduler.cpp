#include "arduino_compat.h"
#include "task_scheduler.h"
#include <stdlib.h>

// Internal task array
static MCP_Task* s_tasks = NULL;
static uint16_t s_maxTasks = 0;
static uint16_t s_taskCount = 0;
static uint32_t s_nextTaskId = 1;

int MCP_TaskSchedulerInit(uint16_t maxTasks) {
    if (s_tasks != NULL) {
        // Already initialized
        return -1;
    }
    
    s_tasks = (MCP_Task*)calloc(maxTasks, sizeof(MCP_Task));
    if (s_tasks == NULL) {
        return -2;  // Memory allocation failed
    }
    
    s_maxTasks = maxTasks;
    s_taskCount = 0;
    s_nextTaskId = 1;
    
    return 0;
}

uint32_t MCP_TaskCreate(MCP_TaskFunction function, void* param, uint32_t interval, MCP_TaskPriority priority) {
    if (s_tasks == NULL || function == NULL) {
        return 0;
    }
    
    if (s_taskCount >= s_maxTasks) {
        return 0;  // No space for new task
    }
    
    // Find free slot
    uint16_t i;
    for (i = 0; i < s_maxTasks; i++) {
        if (s_tasks[i].function == NULL) {
            break;
        }
    }
    
    if (i >= s_maxTasks) {
        return 0;  // No free slot found
    }
    
    // Assign task
    s_tasks[i].id = s_nextTaskId++;
    s_tasks[i].function = function;
    s_tasks[i].param = param;
    s_tasks[i].interval = interval;
    s_tasks[i].lastRun = 0;
    s_tasks[i].priority = priority;
    s_tasks[i].enabled = true;
    
    s_taskCount++;
    
    return s_tasks[i].id;
}

int MCP_TaskSetEnabled(uint32_t taskId, bool enabled) {
    if (s_tasks == NULL || taskId == 0) {
        return -1;
    }
    
    // Find task by ID
    for (uint16_t i = 0; i < s_maxTasks; i++) {
        if (s_tasks[i].id == taskId) {
            s_tasks[i].enabled = enabled;
            return 0;
        }
    }
    
    return -2;  // Task not found
}

int MCP_TaskDelete(uint32_t taskId) {
    if (s_tasks == NULL || taskId == 0) {
        return -1;
    }
    
    // Find task by ID
    for (uint16_t i = 0; i < s_maxTasks; i++) {
        if (s_tasks[i].id == taskId) {
            // Clear task
            s_tasks[i].function = NULL;
            s_tasks[i].id = 0;
            s_taskCount--;
            return 0;
        }
    }
    
    return -2;  // Task not found
}

int MCP_TaskProcess(uint32_t currentTimeMs) {
    if (s_tasks == NULL) {
        return -1;
    }
    
    int executedTasks = 0;
    
    // Process by priority (highest to lowest)
    for (int priority = MCP_TASK_PRIORITY_CRITICAL; priority >= MCP_TASK_PRIORITY_LOW; priority--) {
        for (uint16_t i = 0; i < s_maxTasks; i++) {
            // Skip empty slots and tasks with different priority
            if (s_tasks[i].function == NULL || (int)s_tasks[i].priority != priority) {
                continue;
            }
            
            // Skip disabled tasks
            if (!s_tasks[i].enabled) {
                continue;
            }
            
            // Check if task should run
            if (s_tasks[i].lastRun == 0 || (currentTimeMs - s_tasks[i].lastRun >= s_tasks[i].interval)) {
                // Execute task
                s_tasks[i].function(s_tasks[i].param);
                s_tasks[i].lastRun = currentTimeMs;
                executedTasks++;
                
                // Handle run-once tasks
                if (s_tasks[i].interval == 0) {
                    MCP_TaskDelete(s_tasks[i].id);
                }
            }
        }
    }
    
    return executedTasks;
}
