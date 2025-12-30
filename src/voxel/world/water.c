/**
 * Water Flow System Implementation
 */

#include "voxel/world/water.h"
#include "voxel/world/world.h"
#include "voxel/core/block.h"
#include <stdlib.h>
#include <stdio.h>

// ============================================================================
// QUEUE MANAGEMENT
// ============================================================================

WaterUpdateQueue* water_queue_create(void) {
    WaterUpdateQueue* queue = (WaterUpdateQueue*)malloc(sizeof(WaterUpdateQueue));
    if (!queue) return NULL;

    queue->pending = NULL;
    queue->free_list = NULL;
    queue->count = 0;
    queue->current_tick = 0;

    printf("[WATER] Update queue created\n");
    return queue;
}

void water_queue_destroy(WaterUpdateQueue* queue) {
    if (!queue) return;

    // Free pending updates
    WaterUpdate* node = queue->pending;
    while (node) {
        WaterUpdate* next = node->next;
        free(node);
        node = next;
    }

    // Free reusable pool
    node = queue->free_list;
    while (node) {
        WaterUpdate* next = node->next;
        free(node);
        node = next;
    }

    free(queue);
    printf("[WATER] Update queue destroyed\n");
}

/**
 * Get a node from free list or allocate new one
 */
static WaterUpdate* get_node(WaterUpdateQueue* queue) {
    WaterUpdate* node;
    if (queue->free_list) {
        node = queue->free_list;
        queue->free_list = node->next;
    } else {
        node = (WaterUpdate*)malloc(sizeof(WaterUpdate));
    }
    return node;
}

/**
 * Return a node to the free list
 */
static void release_node(WaterUpdateQueue* queue, WaterUpdate* node) {
    node->next = queue->free_list;
    queue->free_list = node;
}

void water_schedule_update(WaterUpdateQueue* queue, int x, int y, int z, int delay) {
    if (!queue) return;

    // Check if this position is already scheduled
    WaterUpdate* node = queue->pending;
    while (node) {
        if (node->x == x && node->y == y && node->z == z) {
            // Already scheduled, update to earlier time if needed
            int target_tick = queue->current_tick + delay;
            if (target_tick < node->scheduled_tick) {
                node->scheduled_tick = target_tick;
            }
            return;
        }
        node = node->next;
    }

    // Create new update
    node = get_node(queue);
    if (!node) return;

    node->x = x;
    node->y = y;
    node->z = z;
    node->scheduled_tick = queue->current_tick + delay;

    // Insert at head (simple list)
    node->next = queue->pending;
    queue->pending = node;
    queue->count++;
}

// ============================================================================
// WATER FLOW LOGIC
// ============================================================================

/**
 * Try to flow water to a position
 * Returns true if water was placed
 */
static bool water_flow_to(WaterUpdateQueue* queue, World* world,
                          int x, int y, int z, int source_level, bool falling) {
    Block current = world_get_block(world, x, y, z);

    // Can only flow into air
    if (current.type != BLOCK_AIR) {
        // Can flow into lower-level water
        if (current.type == BLOCK_WATER) {
            int current_level = water_get_level(current.metadata);
            int new_level = falling ? 1 : source_level + 1;

            // Only replace if we would be stronger (lower level = stronger)
            if (new_level < current_level) {
                Block water = {BLOCK_WATER, current.light_level, water_make_metadata(new_level, falling)};
                world_set_block(world, x, y, z, water);
                water_schedule_update(queue, x, y, z, WATER_FLOW_DELAY);
                return true;
            }
        }
        return false;
    }

    // Calculate new water level
    int new_level = falling ? 1 : source_level + 1;
    if (new_level > WATER_MAX_LEVEL) {
        return false;  // Water has dissipated
    }

    // Place water block
    Block water = {BLOCK_WATER, 0, water_make_metadata(new_level, falling)};
    world_set_block(world, x, y, z, water);

    // Schedule this new water block to flow
    water_schedule_update(queue, x, y, z, WATER_FLOW_DELAY);

    return true;
}

/**
 * Process a single water block update
 */
static void water_update_block(WaterUpdateQueue* queue, World* world, int x, int y, int z) {
    Block block = world_get_block(world, x, y, z);

    // Only process water blocks
    if (block.type != BLOCK_WATER) return;

    int level = water_get_level(block.metadata);
    bool falling = water_is_falling(block.metadata);

    // Check if water should disappear (no source)
    if (level > 0 && !falling) {
        // Check if there's a source or stronger water nearby
        bool has_source = false;

        // Check above
        Block above = world_get_block(world, x, y + 1, z);
        if (above.type == BLOCK_WATER) {
            has_source = true;
        }

        // Check horizontal neighbors for stronger water
        if (!has_source) {
            int dx[] = {1, -1, 0, 0};
            int dz[] = {0, 0, 1, -1};
            for (int i = 0; i < 4; i++) {
                Block neighbor = world_get_block(world, x + dx[i], y, z + dz[i]);
                if (neighbor.type == BLOCK_WATER) {
                    int neighbor_level = water_get_level(neighbor.metadata);
                    if (neighbor_level < level - 1) {
                        has_source = true;
                        break;
                    }
                }
            }
        }

        // If no source found, decay the water
        if (!has_source) {
            // Remove the water
            Block air = {BLOCK_AIR, 0, 0};
            world_set_block(world, x, y, z, air);

            // Schedule neighbor updates
            water_on_block_change(queue, world, x, y, z);
            return;
        }
    }

    // Try to flow down first (priority)
    Block below = world_get_block(world, x, y - 1, z);
    if (below.type == BLOCK_AIR) {
        water_flow_to(queue, world, x, y - 1, z, level, true);  // Falling water
        return;  // Water fell, don't spread horizontally yet
    }

    // If below is water, become falling water if not already
    if (below.type == BLOCK_WATER && !falling) {
        // Update to falling
        block.metadata = water_make_metadata(level, true);
        world_set_block(world, x, y, z, block);
    }

    // Spread horizontally (only if not source or if on solid ground)
    if (level < WATER_MAX_LEVEL) {
        // Check if we're on solid ground (or water)
        if (below.type != BLOCK_AIR) {
            int dx[] = {1, -1, 0, 0};
            int dz[] = {0, 0, 1, -1};

            for (int i = 0; i < 4; i++) {
                int nx = x + dx[i];
                int nz = z + dz[i];

                // Flow horizontally
                water_flow_to(queue, world, nx, y, nz, level, false);
            }
        }
    }
}

void water_process_tick(WaterUpdateQueue* queue, World* world) {
    if (!queue || !world) return;

    queue->current_tick++;

    // Process all updates scheduled for this tick
    WaterUpdate** prev = &queue->pending;
    WaterUpdate* node = queue->pending;

    int processed = 0;
    const int MAX_UPDATES_PER_TICK = 100;  // Limit for performance

    while (node && processed < MAX_UPDATES_PER_TICK) {
        if (node->scheduled_tick <= queue->current_tick) {
            // Remove from list
            *prev = node->next;

            // Process the update
            water_update_block(queue, world, node->x, node->y, node->z);

            // Release node
            release_node(queue, node);
            queue->count--;
            processed++;

            node = *prev;
        } else {
            prev = &node->next;
            node = node->next;
        }
    }
}

void water_on_block_change(WaterUpdateQueue* queue, World* world, int x, int y, int z) {
    if (!queue || !world) return;

    // Schedule updates for all neighboring water blocks
    int dx[] = {0, 0, 0, 1, -1, 0, 0};
    int dy[] = {1, -1, 0, 0, 0, 0, 0};
    int dz[] = {0, 0, 0, 0, 0, 1, -1};

    for (int i = 0; i < 7; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        int nz = z + dz[i];

        Block neighbor = world_get_block(world, nx, ny, nz);
        if (neighbor.type == BLOCK_WATER) {
            water_schedule_update(queue, nx, ny, nz, 1);  // Update next tick
        }
    }

    // Also check if the changed block itself could receive water
    Block block = world_get_block(world, x, y, z);
    if (block.type == BLOCK_AIR) {
        // Check if there's water above that could fall
        Block above = world_get_block(world, x, y + 1, z);
        if (above.type == BLOCK_WATER) {
            water_schedule_update(queue, x, y + 1, z, 1);
        }
    }
}
