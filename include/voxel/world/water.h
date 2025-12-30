/**
 * Water Flow System
 *
 * Handles dynamic water spreading and flow mechanics
 */

#ifndef VOXEL_WATER_H
#define VOXEL_WATER_H

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct World World;

// ============================================================================
// WATER CONSTANTS
// ============================================================================

// Water metadata encoding:
// Bits 0-2: Water level (0 = source/full, 7 = minimum)
// Bit 3: Falling flag (1 = falling water)
#define WATER_LEVEL_MASK    0x07
#define WATER_FALLING_BIT   0x08
#define WATER_MAX_LEVEL     7
#define WATER_FLOW_DELAY    4    // Ticks between flow updates

// ============================================================================
// WATER UPDATE QUEUE
// ============================================================================

typedef struct WaterUpdate {
    int x, y, z;
    int scheduled_tick;
    struct WaterUpdate* next;
} WaterUpdate;

typedef struct WaterUpdateQueue {
    WaterUpdate* pending;       // Linked list of pending updates
    WaterUpdate* free_list;     // Pool of reusable nodes
    int count;                  // Number of pending updates
    int current_tick;           // Current game tick
} WaterUpdateQueue;

// ============================================================================
// API
// ============================================================================

/**
 * Create a water update queue
 */
WaterUpdateQueue* water_queue_create(void);

/**
 * Destroy the water update queue
 */
void water_queue_destroy(WaterUpdateQueue* queue);

/**
 * Schedule a water update at position
 * delay: Number of ticks before the update runs
 */
void water_schedule_update(WaterUpdateQueue* queue, int x, int y, int z, int delay);

/**
 * Process pending water updates for this tick
 * Call this each game tick
 */
void water_process_tick(WaterUpdateQueue* queue, World* world);

/**
 * Called when a block changes - schedules water updates for neighbors
 */
void water_on_block_change(WaterUpdateQueue* queue, World* world, int x, int y, int z);

/**
 * Get water level from block metadata (0 = source, 7 = minimum)
 */
static inline int water_get_level(uint8_t metadata) {
    return metadata & WATER_LEVEL_MASK;
}

/**
 * Check if water is falling
 */
static inline bool water_is_falling(uint8_t metadata) {
    return (metadata & WATER_FALLING_BIT) != 0;
}

/**
 * Create water metadata
 */
static inline uint8_t water_make_metadata(int level, bool falling) {
    return (uint8_t)((level & WATER_LEVEL_MASK) | (falling ? WATER_FALLING_BIT : 0));
}

#endif // VOXEL_WATER_H
