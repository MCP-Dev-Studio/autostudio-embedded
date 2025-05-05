#include "memory_manager.h"
#include <string.h>
#include <stdlib.h>

// Memory block header
typedef struct MemoryBlockHeader {
    size_t size;                      // Block size including header
    MCP_MemoryRegionType regionType;  // Region this block belongs to
    bool free;                        // Is this block free?
    const char* tag;                  // Optional tag for tracking
    struct MemoryBlockHeader* next;   // Next block in the region
    struct MemoryBlockHeader* prev;   // Previous block in the region
} MemoryBlockHeader;

// Region information
typedef struct {
    MCP_MemoryRegion region;
    MemoryBlockHeader* firstBlock;
    MCP_MemoryStats stats;
} RegionInfo;

// Internal state
static RegionInfo* s_regions = NULL;
static uint8_t s_regionCount = 0;
static bool s_initialized = false;

// Size of the block header
#define HEADER_SIZE sizeof(MemoryBlockHeader)

int MCP_MemoryInit(MCP_MemoryRegion* regions, uint8_t regionCount) {
    if (s_initialized || regions == NULL || regionCount == 0) {
        return -1;
    }
    
    // Allocate region info array
    s_regions = (RegionInfo*)calloc(regionCount, sizeof(RegionInfo));
    if (s_regions == NULL) {
        return -2;
    }
    
    s_regionCount = regionCount;
    
    // Initialize each region
    for (uint8_t i = 0; i < regionCount; i++) {
        s_regions[i].region = regions[i];
        
        // Initialize stats
        s_regions[i].stats.totalSize = regions[i].size;
        s_regions[i].stats.usedSize = 0;
        s_regions[i].stats.peakUsage = 0;
        s_regions[i].stats.allocCount = 0;
        s_regions[i].stats.freeCount = 0;
        s_regions[i].stats.fragmentCount = 0;
        
        // Create initial free block
        MemoryBlockHeader* initialBlock = (MemoryBlockHeader*)regions[i].start;
        initialBlock->size = regions[i].size;
        initialBlock->regionType = regions[i].type;
        initialBlock->free = true;
        initialBlock->tag = NULL;
        initialBlock->next = NULL;
        initialBlock->prev = NULL;
        
        s_regions[i].firstBlock = initialBlock;
    }
    
    s_initialized = true;
    return 0;
}

static RegionInfo* findRegionByType(MCP_MemoryRegionType regionType) {
    for (uint8_t i = 0; i < s_regionCount; i++) {
        if (s_regions[i].region.type == regionType) {
            return &s_regions[i];
        }
    }
    return NULL;
}

static RegionInfo* findRegionForPointer(void* ptr) {
    MemoryBlockHeader* header = (MemoryBlockHeader*)((uint8_t*)ptr - HEADER_SIZE);
    
    for (uint8_t i = 0; i < s_regionCount; i++) {
        void* regionStart = s_regions[i].region.start;
        void* regionEnd = (uint8_t*)regionStart + s_regions[i].region.size;
        
        if ((void*)header >= regionStart && (void*)header < regionEnd) {
            return &s_regions[i];
        }
    }
    
    return NULL;
}

void* MCP_MemoryAllocate(MCP_MemoryRegionType regionType, size_t size, const char* tag) {
    if (!s_initialized || size == 0) {
        return NULL;
    }
    
    // Find the region
    RegionInfo* region = findRegionByType(regionType);
    if (region == NULL) {
        return NULL;
    }
    
    // Calculate total size needed (including header)
    size_t totalSize = size + HEADER_SIZE;
    
    // Find suitable free block using first-fit approach
    MemoryBlockHeader* currentBlock = region->firstBlock;
    while (currentBlock != NULL) {
        if (currentBlock->free && currentBlock->size >= totalSize) {
            // Found a suitable block
            
            // Check if we need to split the block
            if (currentBlock->size >= totalSize + HEADER_SIZE + 8) {
                // Split the block
                size_t originalSize = currentBlock->size;
                currentBlock->size = totalSize;
                
                // Create new free block after this one
                MemoryBlockHeader* newBlock = (MemoryBlockHeader*)(
                    (uint8_t*)currentBlock + totalSize
                );
                
                newBlock->size = originalSize - totalSize;
                newBlock->regionType = regionType;
                newBlock->free = true;
                newBlock->tag = NULL;
                
                // Update linked list
                newBlock->next = currentBlock->next;
                newBlock->prev = currentBlock;
                
                if (currentBlock->next != NULL) {
                    currentBlock->next->prev = newBlock;
                }
                
                currentBlock->next = newBlock;
                
                // Update fragmentation count
                region->stats.fragmentCount++;
            }
            
            // Mark block as used and set tag
            currentBlock->free = false;
            currentBlock->tag = tag;
            
            // Update stats
            region->stats.usedSize += currentBlock->size;
            region->stats.allocCount++;
            
            if (region->stats.usedSize > region->stats.peakUsage) {
                region->stats.peakUsage = region->stats.usedSize;
            }
            
            // Return pointer to the data portion
            return (void*)((uint8_t*)currentBlock + HEADER_SIZE);
        }
        
        currentBlock = currentBlock->next;
    }
    
    // No suitable block found
    return NULL;
}

int MCP_MemoryFree(void* ptr) {
    if (!s_initialized || ptr == NULL) {
        return -1;
    }
    
    // Get block header
    MemoryBlockHeader* header = (MemoryBlockHeader*)((uint8_t*)ptr - HEADER_SIZE);
    
    // Verify pointer is in a valid region
    RegionInfo* region = findRegionForPointer(ptr);
    if (region == NULL) {
        return -2;  // Invalid pointer
    }
    
    // Check if already free
    if (header->free) {
        return -3;  // Double free
    }
    
    // Mark as free
    header->free = true;
    header->tag = NULL;
    
    // Update stats
    region->stats.usedSize -= header->size;
    region->stats.freeCount++;
    
    // Try to merge with next block if it's free
    if (header->next != NULL && header->next->free) {
        header->size += header->next->size;
        
        // Remove next block from list
        MemoryBlockHeader* nextNext = header->next->next;
        
        if (nextNext != NULL) {
            nextNext->prev = header;
        }
        
        header->next = nextNext;
        
        // Update fragmentation count
        region->stats.fragmentCount--;
    }
    
    // Try to merge with previous block if it's free
    if (header->prev != NULL && header->prev->free) {
        header->prev->size += header->size;
        
        // Remove this block from list
        if (header->next != NULL) {
            header->next->prev = header->prev;
        }
        
        header->prev->next = header->next;
        
        // Update fragmentation count
        region->stats.fragmentCount--;
    }
    
    return 0;
}

int MCP_MemoryGetStats(MCP_MemoryRegionType regionType, MCP_MemoryStats* stats) {
    if (!s_initialized || stats == NULL) {
        return -1;
    }
    
    // Find the region
    RegionInfo* region = findRegionByType(regionType);
    if (region == NULL) {
        return -2;
    }
    
    // Copy stats
    *stats = region->stats;
    
    return 0;
}

int MCP_MemoryOptimize(int regionType) {
    if (!s_initialized) {
        return -1;
    }
    
    // Optimize each region or just the specified one
    for (uint8_t i = 0; i < s_regionCount; i++) {
        if (regionType >= 0 && (int)s_regions[i].region.type != regionType) {
            continue;
        }
        
        // For now, our optimization is just merging adjacent free blocks
        MemoryBlockHeader* currentBlock = s_regions[i].firstBlock;
        
        while (currentBlock != NULL && currentBlock->next != NULL) {
            if (currentBlock->free && currentBlock->next->free) {
                // Merge blocks
                currentBlock->size += currentBlock->next->size;
                
                // Remove next block from list
                MemoryBlockHeader* nextNext = currentBlock->next->next;
                
                if (nextNext != NULL) {
                    nextNext->prev = currentBlock;
                }
                
                currentBlock->next = nextNext;
                
                // Update fragmentation count
                s_regions[i].stats.fragmentCount--;
                
                // Don't advance currentBlock, try to merge more
            } else {
                // Move to next block
                currentBlock = currentBlock->next;
            }
        }
    }
    
    return 0;
}