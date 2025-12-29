/**
 * Tree Generation & Leaf Decay System
 *
 * Generates procedural trees. Leaves decay over time when not connected to wood.
 */

#ifndef VOXEL_TREE_H
#define VOXEL_TREE_H

#include "voxel/world/chunk.h"

// Forward declaration
struct World;

// Tree size variations
typedef enum {
    TREE_SMALL,    // 5 blocks tall
    TREE_MEDIUM,   // 7 blocks tall
    TREE_LARGE,    // 9 blocks tall
    TREE_SIZE_COUNT
} TreeSize;

/**
 * Place a tree at the given local chunk coordinates.
 * base_y is where the trunk starts (should be above grass).
 * Tree blocks are placed with metadata=1 to mark as natural.
 */
void tree_place_at(Chunk* chunk, int local_x, int base_y, int local_z, TreeSize size);

/**
 * Generate trees for an entire chunk.
 * Uses noise for natural placement on grass blocks.
 * Call this after terrain generation.
 */
void tree_generate_for_chunk(Chunk* chunk);

/**
 * Initialize the leaf decay system.
 */
void leaf_decay_init(void);

/**
 * Schedule leaves near a removed wood block for decay check.
 * Call this when a wood block is broken.
 */
void leaf_decay_on_wood_removed(struct World* world, int x, int y, int z);

/**
 * Update leaf decay - call each frame.
 * Processes pending leaves and removes those not connected to wood.
 */
void leaf_decay_update(struct World* world, float dt);

#endif // VOXEL_TREE_H
