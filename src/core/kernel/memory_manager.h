#ifndef MCP_MEMORY_MANAGER_H
#define MCP_MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Memory region types
 */
typedef enum {
    MCP_MEMORY_REGION_STATIC,    // Static allocation region
    MCP_MEMORY_REGION_DYNAMIC,   // Dynamic allocation region
    MCP_MEMORY_REGION_TOOL,      // Tool execution region
    MCP_MEMORY_REGION_RESOURCE,  // Resource storage region
    MCP_MEMORY_REGION_SYSTEM     // System reserved region
} MCP_MemoryRegionType;

/**
 * @brief Memory region definition
 */
typedef struct {
    MCP_MemoryRegionType type;
    void* start;
    size_t size;
} MCP_MemoryRegion;

/**
 * @brief Memory statistics
 */
typedef struct {
    size_t totalSize;
    size_t usedSize;
    size_t peakUsage;
    size_t allocCount;
    size_t freeCount;
    size_t fragmentCount;
} MCP_MemoryStats;

/**
 * @brief Initialize the memory manager
 * 
 * @param regions Array of memory regions to manage
 * @param regionCount Number of regions in the array
 * @return int 0 on success, negative error code on failure
 */
int MCP_MemoryInit(MCP_MemoryRegion* regions, uint8_t regionCount);

/**
 * @brief Allocate memory from a specific region
 * 
 * @param regionType Region type to allocate from
 * @param size Size in bytes to allocate
 * @param tag Optional tag for tracking (can be NULL)
 * @return void* Pointer to allocated memory or NULL on failure
 */
void* MCP_MemoryAllocate(MCP_MemoryRegionType regionType, size_t size, const char* tag);

/**
 * @brief Free previously allocated memory
 * 
 * @param ptr Pointer to memory to free
 * @return int 0 on success, negative error code on failure
 */
int MCP_MemoryFree(void* ptr);

/**
 * @brief Get memory statistics for a specific region
 * 
 * @param regionType Region type to get statistics for
 * @param stats Pointer to structure to fill with statistics
 * @return int 0 on success, negative error code on failure
 */
int MCP_MemoryGetStats(MCP_MemoryRegionType regionType, MCP_MemoryStats* stats);

/**
 * @brief Optimize memory regions (defragment, compact)
 * 
 * @param regionType Region type to optimize (or -1 for all regions)
 * @return int 0 on success, negative error code on failure
 */
int MCP_MemoryOptimize(int regionType);

#endif /* MCP_MEMORY_MANAGER_H */