/**
 * Tree Generation & Leaf Decay System
 *
 * Generates procedural trees. Leaves decay over time when not connected to wood.
 */

#include "voxel/entity/tree.h"
#include "voxel/core/block.h"
#include "voxel/world/biome.h"
#include "voxel/world/noise.h"
#include "voxel/world/world.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// ============================================================================
// TREE TEMPLATES
// ============================================================================

typedef struct {
    int8_t dx, dy, dz;  // Offset from trunk base
    BlockType type;      // BLOCK_WOOD or BLOCK_LEAVES
} TreeBlock;

// Small Oak: 5 blocks tall, narrow canopy
static const TreeBlock SMALL_OAK[] = {
    // Trunk (y=0 to y=3)
    {0, 0, 0, BLOCK_WOOD},
    {0, 1, 0, BLOCK_WOOD},
    {0, 2, 0, BLOCK_WOOD},
    {0, 3, 0, BLOCK_WOOD},

    // Leaves layer at y=2 (around trunk)
    {-1, 2, 0, BLOCK_LEAVES}, {1, 2, 0, BLOCK_LEAVES},
    {0, 2, -1, BLOCK_LEAVES}, {0, 2, 1, BLOCK_LEAVES},
    {-1, 2, -1, BLOCK_LEAVES}, {1, 2, -1, BLOCK_LEAVES},
    {-1, 2, 1, BLOCK_LEAVES}, {1, 2, 1, BLOCK_LEAVES},

    // Leaves layer at y=3 (around trunk)
    {-1, 3, 0, BLOCK_LEAVES}, {1, 3, 0, BLOCK_LEAVES},
    {0, 3, -1, BLOCK_LEAVES}, {0, 3, 1, BLOCK_LEAVES},
    {-1, 3, -1, BLOCK_LEAVES}, {1, 3, -1, BLOCK_LEAVES},
    {-1, 3, 1, BLOCK_LEAVES}, {1, 3, 1, BLOCK_LEAVES},

    // Leaves top at y=4
    {0, 4, 0, BLOCK_LEAVES},
    {-1, 4, 0, BLOCK_LEAVES}, {1, 4, 0, BLOCK_LEAVES},
    {0, 4, -1, BLOCK_LEAVES}, {0, 4, 1, BLOCK_LEAVES},
};
#define SMALL_OAK_COUNT (sizeof(SMALL_OAK) / sizeof(TreeBlock))

// Medium Oak: 7 blocks tall, wider canopy
static const TreeBlock MEDIUM_OAK[] = {
    // Trunk (y=0 to y=4)
    {0, 0, 0, BLOCK_WOOD},
    {0, 1, 0, BLOCK_WOOD},
    {0, 2, 0, BLOCK_WOOD},
    {0, 3, 0, BLOCK_WOOD},
    {0, 4, 0, BLOCK_WOOD},

    // Leaves layer at y=3 (wide, 5x5 with corners missing)
    {-2, 3, 0, BLOCK_LEAVES}, {2, 3, 0, BLOCK_LEAVES},
    {0, 3, -2, BLOCK_LEAVES}, {0, 3, 2, BLOCK_LEAVES},
    {-1, 3, 0, BLOCK_LEAVES}, {1, 3, 0, BLOCK_LEAVES},
    {0, 3, -1, BLOCK_LEAVES}, {0, 3, 1, BLOCK_LEAVES},
    {-1, 3, -1, BLOCK_LEAVES}, {1, 3, -1, BLOCK_LEAVES},
    {-1, 3, 1, BLOCK_LEAVES}, {1, 3, 1, BLOCK_LEAVES},
    {-2, 3, -1, BLOCK_LEAVES}, {-2, 3, 1, BLOCK_LEAVES},
    {2, 3, -1, BLOCK_LEAVES}, {2, 3, 1, BLOCK_LEAVES},
    {-1, 3, -2, BLOCK_LEAVES}, {1, 3, -2, BLOCK_LEAVES},
    {-1, 3, 2, BLOCK_LEAVES}, {1, 3, 2, BLOCK_LEAVES},

    // Leaves layer at y=4 (around trunk, 5x5 with corners missing)
    {-2, 4, 0, BLOCK_LEAVES}, {2, 4, 0, BLOCK_LEAVES},
    {0, 4, -2, BLOCK_LEAVES}, {0, 4, 2, BLOCK_LEAVES},
    {-1, 4, 0, BLOCK_LEAVES}, {1, 4, 0, BLOCK_LEAVES},
    {0, 4, -1, BLOCK_LEAVES}, {0, 4, 1, BLOCK_LEAVES},
    {-1, 4, -1, BLOCK_LEAVES}, {1, 4, -1, BLOCK_LEAVES},
    {-1, 4, 1, BLOCK_LEAVES}, {1, 4, 1, BLOCK_LEAVES},
    {-2, 4, -1, BLOCK_LEAVES}, {-2, 4, 1, BLOCK_LEAVES},
    {2, 4, -1, BLOCK_LEAVES}, {2, 4, 1, BLOCK_LEAVES},
    {-1, 4, -2, BLOCK_LEAVES}, {1, 4, -2, BLOCK_LEAVES},
    {-1, 4, 2, BLOCK_LEAVES}, {1, 4, 2, BLOCK_LEAVES},

    // Leaves layer at y=5 (3x3)
    {0, 5, 0, BLOCK_LEAVES},
    {-1, 5, 0, BLOCK_LEAVES}, {1, 5, 0, BLOCK_LEAVES},
    {0, 5, -1, BLOCK_LEAVES}, {0, 5, 1, BLOCK_LEAVES},
    {-1, 5, -1, BLOCK_LEAVES}, {1, 5, -1, BLOCK_LEAVES},
    {-1, 5, 1, BLOCK_LEAVES}, {1, 5, 1, BLOCK_LEAVES},

    // Leaves top at y=6
    {0, 6, 0, BLOCK_LEAVES},
    {-1, 6, 0, BLOCK_LEAVES}, {1, 6, 0, BLOCK_LEAVES},
    {0, 6, -1, BLOCK_LEAVES}, {0, 6, 1, BLOCK_LEAVES},
};
#define MEDIUM_OAK_COUNT (sizeof(MEDIUM_OAK) / sizeof(TreeBlock))

// Large Oak: 9 blocks tall, biggest canopy
static const TreeBlock LARGE_OAK[] = {
    // Trunk (y=0 to y=6)
    {0, 0, 0, BLOCK_WOOD},
    {0, 1, 0, BLOCK_WOOD},
    {0, 2, 0, BLOCK_WOOD},
    {0, 3, 0, BLOCK_WOOD},
    {0, 4, 0, BLOCK_WOOD},
    {0, 5, 0, BLOCK_WOOD},
    {0, 6, 0, BLOCK_WOOD},

    // Leaves layer at y=4 (wide 5x5)
    {-2, 4, 0, BLOCK_LEAVES}, {2, 4, 0, BLOCK_LEAVES},
    {0, 4, -2, BLOCK_LEAVES}, {0, 4, 2, BLOCK_LEAVES},
    {-1, 4, 0, BLOCK_LEAVES}, {1, 4, 0, BLOCK_LEAVES},
    {0, 4, -1, BLOCK_LEAVES}, {0, 4, 1, BLOCK_LEAVES},
    {-1, 4, -1, BLOCK_LEAVES}, {1, 4, -1, BLOCK_LEAVES},
    {-1, 4, 1, BLOCK_LEAVES}, {1, 4, 1, BLOCK_LEAVES},
    {-2, 4, -1, BLOCK_LEAVES}, {-2, 4, 1, BLOCK_LEAVES},
    {2, 4, -1, BLOCK_LEAVES}, {2, 4, 1, BLOCK_LEAVES},
    {-1, 4, -2, BLOCK_LEAVES}, {1, 4, -2, BLOCK_LEAVES},
    {-1, 4, 2, BLOCK_LEAVES}, {1, 4, 2, BLOCK_LEAVES},
    {-2, 4, -2, BLOCK_LEAVES}, {2, 4, -2, BLOCK_LEAVES},
    {-2, 4, 2, BLOCK_LEAVES}, {2, 4, 2, BLOCK_LEAVES},

    // Leaves layer at y=5 (wide 5x5)
    {-2, 5, 0, BLOCK_LEAVES}, {2, 5, 0, BLOCK_LEAVES},
    {0, 5, -2, BLOCK_LEAVES}, {0, 5, 2, BLOCK_LEAVES},
    {-1, 5, 0, BLOCK_LEAVES}, {1, 5, 0, BLOCK_LEAVES},
    {0, 5, -1, BLOCK_LEAVES}, {0, 5, 1, BLOCK_LEAVES},
    {-1, 5, -1, BLOCK_LEAVES}, {1, 5, -1, BLOCK_LEAVES},
    {-1, 5, 1, BLOCK_LEAVES}, {1, 5, 1, BLOCK_LEAVES},
    {-2, 5, -1, BLOCK_LEAVES}, {-2, 5, 1, BLOCK_LEAVES},
    {2, 5, -1, BLOCK_LEAVES}, {2, 5, 1, BLOCK_LEAVES},
    {-1, 5, -2, BLOCK_LEAVES}, {1, 5, -2, BLOCK_LEAVES},
    {-1, 5, 2, BLOCK_LEAVES}, {1, 5, 2, BLOCK_LEAVES},
    {-2, 5, -2, BLOCK_LEAVES}, {2, 5, -2, BLOCK_LEAVES},
    {-2, 5, 2, BLOCK_LEAVES}, {2, 5, 2, BLOCK_LEAVES},

    // Leaves layer at y=6 (around trunk, 5x5)
    {-2, 6, 0, BLOCK_LEAVES}, {2, 6, 0, BLOCK_LEAVES},
    {0, 6, -2, BLOCK_LEAVES}, {0, 6, 2, BLOCK_LEAVES},
    {-1, 6, 0, BLOCK_LEAVES}, {1, 6, 0, BLOCK_LEAVES},
    {0, 6, -1, BLOCK_LEAVES}, {0, 6, 1, BLOCK_LEAVES},
    {-1, 6, -1, BLOCK_LEAVES}, {1, 6, -1, BLOCK_LEAVES},
    {-1, 6, 1, BLOCK_LEAVES}, {1, 6, 1, BLOCK_LEAVES},
    {-2, 6, -1, BLOCK_LEAVES}, {-2, 6, 1, BLOCK_LEAVES},
    {2, 6, -1, BLOCK_LEAVES}, {2, 6, 1, BLOCK_LEAVES},
    {-1, 6, -2, BLOCK_LEAVES}, {1, 6, -2, BLOCK_LEAVES},
    {-1, 6, 2, BLOCK_LEAVES}, {1, 6, 2, BLOCK_LEAVES},

    // Leaves layer at y=7 (3x3)
    {0, 7, 0, BLOCK_LEAVES},
    {-1, 7, 0, BLOCK_LEAVES}, {1, 7, 0, BLOCK_LEAVES},
    {0, 7, -1, BLOCK_LEAVES}, {0, 7, 1, BLOCK_LEAVES},
    {-1, 7, -1, BLOCK_LEAVES}, {1, 7, -1, BLOCK_LEAVES},
    {-1, 7, 1, BLOCK_LEAVES}, {1, 7, 1, BLOCK_LEAVES},

    // Leaves top at y=8
    {0, 8, 0, BLOCK_LEAVES},
    {-1, 8, 0, BLOCK_LEAVES}, {1, 8, 0, BLOCK_LEAVES},
    {0, 8, -1, BLOCK_LEAVES}, {0, 8, 1, BLOCK_LEAVES},
};
#define LARGE_OAK_COUNT (sizeof(LARGE_OAK) / sizeof(TreeBlock))

// ============================================================================
// BIRCH TREE TEMPLATES (tall, thin, small canopy)
// ============================================================================

// Small Birch: 6 blocks tall trunk, small top canopy
static const TreeBlock SMALL_BIRCH[] = {
    // Trunk (y=0 to y=5)
    {0, 0, 0, BLOCK_BIRCH_WOOD},
    {0, 1, 0, BLOCK_BIRCH_WOOD},
    {0, 2, 0, BLOCK_BIRCH_WOOD},
    {0, 3, 0, BLOCK_BIRCH_WOOD},
    {0, 4, 0, BLOCK_BIRCH_WOOD},
    {0, 5, 0, BLOCK_BIRCH_WOOD},

    // Leaves layer at y=4 (3x3)
    {-1, 4, 0, BLOCK_BIRCH_LEAVES}, {1, 4, 0, BLOCK_BIRCH_LEAVES},
    {0, 4, -1, BLOCK_BIRCH_LEAVES}, {0, 4, 1, BLOCK_BIRCH_LEAVES},

    // Leaves layer at y=5 (3x3 around trunk)
    {-1, 5, 0, BLOCK_BIRCH_LEAVES}, {1, 5, 0, BLOCK_BIRCH_LEAVES},
    {0, 5, -1, BLOCK_BIRCH_LEAVES}, {0, 5, 1, BLOCK_BIRCH_LEAVES},
    {-1, 5, -1, BLOCK_BIRCH_LEAVES}, {1, 5, -1, BLOCK_BIRCH_LEAVES},
    {-1, 5, 1, BLOCK_BIRCH_LEAVES}, {1, 5, 1, BLOCK_BIRCH_LEAVES},

    // Leaves top at y=6
    {0, 6, 0, BLOCK_BIRCH_LEAVES},
    {-1, 6, 0, BLOCK_BIRCH_LEAVES}, {1, 6, 0, BLOCK_BIRCH_LEAVES},
    {0, 6, -1, BLOCK_BIRCH_LEAVES}, {0, 6, 1, BLOCK_BIRCH_LEAVES},
};
#define SMALL_BIRCH_COUNT (sizeof(SMALL_BIRCH) / sizeof(TreeBlock))

// Medium Birch: 7 blocks tall trunk
static const TreeBlock MEDIUM_BIRCH[] = {
    // Trunk (y=0 to y=6)
    {0, 0, 0, BLOCK_BIRCH_WOOD},
    {0, 1, 0, BLOCK_BIRCH_WOOD},
    {0, 2, 0, BLOCK_BIRCH_WOOD},
    {0, 3, 0, BLOCK_BIRCH_WOOD},
    {0, 4, 0, BLOCK_BIRCH_WOOD},
    {0, 5, 0, BLOCK_BIRCH_WOOD},
    {0, 6, 0, BLOCK_BIRCH_WOOD},

    // Leaves layer at y=5 (3x3)
    {-1, 5, 0, BLOCK_BIRCH_LEAVES}, {1, 5, 0, BLOCK_BIRCH_LEAVES},
    {0, 5, -1, BLOCK_BIRCH_LEAVES}, {0, 5, 1, BLOCK_BIRCH_LEAVES},

    // Leaves layer at y=6 (3x3 around trunk)
    {-1, 6, 0, BLOCK_BIRCH_LEAVES}, {1, 6, 0, BLOCK_BIRCH_LEAVES},
    {0, 6, -1, BLOCK_BIRCH_LEAVES}, {0, 6, 1, BLOCK_BIRCH_LEAVES},
    {-1, 6, -1, BLOCK_BIRCH_LEAVES}, {1, 6, -1, BLOCK_BIRCH_LEAVES},
    {-1, 6, 1, BLOCK_BIRCH_LEAVES}, {1, 6, 1, BLOCK_BIRCH_LEAVES},

    // Leaves top at y=7
    {0, 7, 0, BLOCK_BIRCH_LEAVES},
    {-1, 7, 0, BLOCK_BIRCH_LEAVES}, {1, 7, 0, BLOCK_BIRCH_LEAVES},
    {0, 7, -1, BLOCK_BIRCH_LEAVES}, {0, 7, 1, BLOCK_BIRCH_LEAVES},
};
#define MEDIUM_BIRCH_COUNT (sizeof(MEDIUM_BIRCH) / sizeof(TreeBlock))

// Large Birch: 8 blocks tall trunk
static const TreeBlock LARGE_BIRCH[] = {
    // Trunk (y=0 to y=7)
    {0, 0, 0, BLOCK_BIRCH_WOOD},
    {0, 1, 0, BLOCK_BIRCH_WOOD},
    {0, 2, 0, BLOCK_BIRCH_WOOD},
    {0, 3, 0, BLOCK_BIRCH_WOOD},
    {0, 4, 0, BLOCK_BIRCH_WOOD},
    {0, 5, 0, BLOCK_BIRCH_WOOD},
    {0, 6, 0, BLOCK_BIRCH_WOOD},
    {0, 7, 0, BLOCK_BIRCH_WOOD},

    // Leaves layer at y=6 (3x3)
    {-1, 6, 0, BLOCK_BIRCH_LEAVES}, {1, 6, 0, BLOCK_BIRCH_LEAVES},
    {0, 6, -1, BLOCK_BIRCH_LEAVES}, {0, 6, 1, BLOCK_BIRCH_LEAVES},
    {-1, 6, -1, BLOCK_BIRCH_LEAVES}, {1, 6, -1, BLOCK_BIRCH_LEAVES},
    {-1, 6, 1, BLOCK_BIRCH_LEAVES}, {1, 6, 1, BLOCK_BIRCH_LEAVES},

    // Leaves layer at y=7 (3x3 around trunk)
    {-1, 7, 0, BLOCK_BIRCH_LEAVES}, {1, 7, 0, BLOCK_BIRCH_LEAVES},
    {0, 7, -1, BLOCK_BIRCH_LEAVES}, {0, 7, 1, BLOCK_BIRCH_LEAVES},
    {-1, 7, -1, BLOCK_BIRCH_LEAVES}, {1, 7, -1, BLOCK_BIRCH_LEAVES},
    {-1, 7, 1, BLOCK_BIRCH_LEAVES}, {1, 7, 1, BLOCK_BIRCH_LEAVES},

    // Leaves top at y=8
    {0, 8, 0, BLOCK_BIRCH_LEAVES},
    {-1, 8, 0, BLOCK_BIRCH_LEAVES}, {1, 8, 0, BLOCK_BIRCH_LEAVES},
    {0, 8, -1, BLOCK_BIRCH_LEAVES}, {0, 8, 1, BLOCK_BIRCH_LEAVES},
};
#define LARGE_BIRCH_COUNT (sizeof(LARGE_BIRCH) / sizeof(TreeBlock))

// ============================================================================
// SPRUCE TREE TEMPLATES (conical, layered)
// ============================================================================

// Small Spruce: 7 blocks tall, conical shape
static const TreeBlock SMALL_SPRUCE[] = {
    // Trunk (y=0 to y=5)
    {0, 0, 0, BLOCK_SPRUCE_WOOD},
    {0, 1, 0, BLOCK_SPRUCE_WOOD},
    {0, 2, 0, BLOCK_SPRUCE_WOOD},
    {0, 3, 0, BLOCK_SPRUCE_WOOD},
    {0, 4, 0, BLOCK_SPRUCE_WOOD},
    {0, 5, 0, BLOCK_SPRUCE_WOOD},

    // Layer at y=2 (5x5 cross)
    {-2, 2, 0, BLOCK_SPRUCE_LEAVES}, {2, 2, 0, BLOCK_SPRUCE_LEAVES},
    {0, 2, -2, BLOCK_SPRUCE_LEAVES}, {0, 2, 2, BLOCK_SPRUCE_LEAVES},
    {-1, 2, 0, BLOCK_SPRUCE_LEAVES}, {1, 2, 0, BLOCK_SPRUCE_LEAVES},
    {0, 2, -1, BLOCK_SPRUCE_LEAVES}, {0, 2, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=3 (3x3)
    {-1, 3, 0, BLOCK_SPRUCE_LEAVES}, {1, 3, 0, BLOCK_SPRUCE_LEAVES},
    {0, 3, -1, BLOCK_SPRUCE_LEAVES}, {0, 3, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 3, -1, BLOCK_SPRUCE_LEAVES}, {1, 3, -1, BLOCK_SPRUCE_LEAVES},
    {-1, 3, 1, BLOCK_SPRUCE_LEAVES}, {1, 3, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=4 (3x3 cross)
    {-1, 4, 0, BLOCK_SPRUCE_LEAVES}, {1, 4, 0, BLOCK_SPRUCE_LEAVES},
    {0, 4, -1, BLOCK_SPRUCE_LEAVES}, {0, 4, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=5 (around trunk)
    {-1, 5, 0, BLOCK_SPRUCE_LEAVES}, {1, 5, 0, BLOCK_SPRUCE_LEAVES},
    {0, 5, -1, BLOCK_SPRUCE_LEAVES}, {0, 5, 1, BLOCK_SPRUCE_LEAVES},

    // Top at y=6
    {0, 6, 0, BLOCK_SPRUCE_LEAVES},
};
#define SMALL_SPRUCE_COUNT (sizeof(SMALL_SPRUCE) / sizeof(TreeBlock))

// Medium Spruce: 9 blocks tall
static const TreeBlock MEDIUM_SPRUCE[] = {
    // Trunk (y=0 to y=7)
    {0, 0, 0, BLOCK_SPRUCE_WOOD},
    {0, 1, 0, BLOCK_SPRUCE_WOOD},
    {0, 2, 0, BLOCK_SPRUCE_WOOD},
    {0, 3, 0, BLOCK_SPRUCE_WOOD},
    {0, 4, 0, BLOCK_SPRUCE_WOOD},
    {0, 5, 0, BLOCK_SPRUCE_WOOD},
    {0, 6, 0, BLOCK_SPRUCE_WOOD},
    {0, 7, 0, BLOCK_SPRUCE_WOOD},

    // Layer at y=2 (wide 5x5)
    {-2, 2, 0, BLOCK_SPRUCE_LEAVES}, {2, 2, 0, BLOCK_SPRUCE_LEAVES},
    {0, 2, -2, BLOCK_SPRUCE_LEAVES}, {0, 2, 2, BLOCK_SPRUCE_LEAVES},
    {-1, 2, 0, BLOCK_SPRUCE_LEAVES}, {1, 2, 0, BLOCK_SPRUCE_LEAVES},
    {0, 2, -1, BLOCK_SPRUCE_LEAVES}, {0, 2, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 2, -1, BLOCK_SPRUCE_LEAVES}, {1, 2, -1, BLOCK_SPRUCE_LEAVES},
    {-1, 2, 1, BLOCK_SPRUCE_LEAVES}, {1, 2, 1, BLOCK_SPRUCE_LEAVES},
    {-2, 2, -1, BLOCK_SPRUCE_LEAVES}, {-2, 2, 1, BLOCK_SPRUCE_LEAVES},
    {2, 2, -1, BLOCK_SPRUCE_LEAVES}, {2, 2, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 2, -2, BLOCK_SPRUCE_LEAVES}, {1, 2, -2, BLOCK_SPRUCE_LEAVES},
    {-1, 2, 2, BLOCK_SPRUCE_LEAVES}, {1, 2, 2, BLOCK_SPRUCE_LEAVES},

    // Layer at y=3 (3x3)
    {-1, 3, 0, BLOCK_SPRUCE_LEAVES}, {1, 3, 0, BLOCK_SPRUCE_LEAVES},
    {0, 3, -1, BLOCK_SPRUCE_LEAVES}, {0, 3, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=4 (5x5)
    {-2, 4, 0, BLOCK_SPRUCE_LEAVES}, {2, 4, 0, BLOCK_SPRUCE_LEAVES},
    {0, 4, -2, BLOCK_SPRUCE_LEAVES}, {0, 4, 2, BLOCK_SPRUCE_LEAVES},
    {-1, 4, 0, BLOCK_SPRUCE_LEAVES}, {1, 4, 0, BLOCK_SPRUCE_LEAVES},
    {0, 4, -1, BLOCK_SPRUCE_LEAVES}, {0, 4, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 4, -1, BLOCK_SPRUCE_LEAVES}, {1, 4, -1, BLOCK_SPRUCE_LEAVES},
    {-1, 4, 1, BLOCK_SPRUCE_LEAVES}, {1, 4, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=5 (3x3)
    {-1, 5, 0, BLOCK_SPRUCE_LEAVES}, {1, 5, 0, BLOCK_SPRUCE_LEAVES},
    {0, 5, -1, BLOCK_SPRUCE_LEAVES}, {0, 5, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=6 (3x3)
    {-1, 6, 0, BLOCK_SPRUCE_LEAVES}, {1, 6, 0, BLOCK_SPRUCE_LEAVES},
    {0, 6, -1, BLOCK_SPRUCE_LEAVES}, {0, 6, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 6, -1, BLOCK_SPRUCE_LEAVES}, {1, 6, -1, BLOCK_SPRUCE_LEAVES},
    {-1, 6, 1, BLOCK_SPRUCE_LEAVES}, {1, 6, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=7 (around trunk)
    {-1, 7, 0, BLOCK_SPRUCE_LEAVES}, {1, 7, 0, BLOCK_SPRUCE_LEAVES},
    {0, 7, -1, BLOCK_SPRUCE_LEAVES}, {0, 7, 1, BLOCK_SPRUCE_LEAVES},

    // Top at y=8
    {0, 8, 0, BLOCK_SPRUCE_LEAVES},
};
#define MEDIUM_SPRUCE_COUNT (sizeof(MEDIUM_SPRUCE) / sizeof(TreeBlock))

// Large Spruce: 11 blocks tall
static const TreeBlock LARGE_SPRUCE[] = {
    // Trunk (y=0 to y=9)
    {0, 0, 0, BLOCK_SPRUCE_WOOD},
    {0, 1, 0, BLOCK_SPRUCE_WOOD},
    {0, 2, 0, BLOCK_SPRUCE_WOOD},
    {0, 3, 0, BLOCK_SPRUCE_WOOD},
    {0, 4, 0, BLOCK_SPRUCE_WOOD},
    {0, 5, 0, BLOCK_SPRUCE_WOOD},
    {0, 6, 0, BLOCK_SPRUCE_WOOD},
    {0, 7, 0, BLOCK_SPRUCE_WOOD},
    {0, 8, 0, BLOCK_SPRUCE_WOOD},
    {0, 9, 0, BLOCK_SPRUCE_WOOD},

    // Layer at y=2 (wide 5x5)
    {-2, 2, 0, BLOCK_SPRUCE_LEAVES}, {2, 2, 0, BLOCK_SPRUCE_LEAVES},
    {0, 2, -2, BLOCK_SPRUCE_LEAVES}, {0, 2, 2, BLOCK_SPRUCE_LEAVES},
    {-1, 2, 0, BLOCK_SPRUCE_LEAVES}, {1, 2, 0, BLOCK_SPRUCE_LEAVES},
    {0, 2, -1, BLOCK_SPRUCE_LEAVES}, {0, 2, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 2, -1, BLOCK_SPRUCE_LEAVES}, {1, 2, -1, BLOCK_SPRUCE_LEAVES},
    {-1, 2, 1, BLOCK_SPRUCE_LEAVES}, {1, 2, 1, BLOCK_SPRUCE_LEAVES},
    {-2, 2, -1, BLOCK_SPRUCE_LEAVES}, {-2, 2, 1, BLOCK_SPRUCE_LEAVES},
    {2, 2, -1, BLOCK_SPRUCE_LEAVES}, {2, 2, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 2, -2, BLOCK_SPRUCE_LEAVES}, {1, 2, -2, BLOCK_SPRUCE_LEAVES},
    {-1, 2, 2, BLOCK_SPRUCE_LEAVES}, {1, 2, 2, BLOCK_SPRUCE_LEAVES},

    // Layer at y=3 (3x3)
    {-1, 3, 0, BLOCK_SPRUCE_LEAVES}, {1, 3, 0, BLOCK_SPRUCE_LEAVES},
    {0, 3, -1, BLOCK_SPRUCE_LEAVES}, {0, 3, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=4 (5x5)
    {-2, 4, 0, BLOCK_SPRUCE_LEAVES}, {2, 4, 0, BLOCK_SPRUCE_LEAVES},
    {0, 4, -2, BLOCK_SPRUCE_LEAVES}, {0, 4, 2, BLOCK_SPRUCE_LEAVES},
    {-1, 4, 0, BLOCK_SPRUCE_LEAVES}, {1, 4, 0, BLOCK_SPRUCE_LEAVES},
    {0, 4, -1, BLOCK_SPRUCE_LEAVES}, {0, 4, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 4, -1, BLOCK_SPRUCE_LEAVES}, {1, 4, -1, BLOCK_SPRUCE_LEAVES},
    {-1, 4, 1, BLOCK_SPRUCE_LEAVES}, {1, 4, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=5 (3x3)
    {-1, 5, 0, BLOCK_SPRUCE_LEAVES}, {1, 5, 0, BLOCK_SPRUCE_LEAVES},
    {0, 5, -1, BLOCK_SPRUCE_LEAVES}, {0, 5, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=6 (5x5)
    {-2, 6, 0, BLOCK_SPRUCE_LEAVES}, {2, 6, 0, BLOCK_SPRUCE_LEAVES},
    {0, 6, -2, BLOCK_SPRUCE_LEAVES}, {0, 6, 2, BLOCK_SPRUCE_LEAVES},
    {-1, 6, 0, BLOCK_SPRUCE_LEAVES}, {1, 6, 0, BLOCK_SPRUCE_LEAVES},
    {0, 6, -1, BLOCK_SPRUCE_LEAVES}, {0, 6, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 6, -1, BLOCK_SPRUCE_LEAVES}, {1, 6, -1, BLOCK_SPRUCE_LEAVES},
    {-1, 6, 1, BLOCK_SPRUCE_LEAVES}, {1, 6, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=7 (3x3)
    {-1, 7, 0, BLOCK_SPRUCE_LEAVES}, {1, 7, 0, BLOCK_SPRUCE_LEAVES},
    {0, 7, -1, BLOCK_SPRUCE_LEAVES}, {0, 7, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=8 (3x3)
    {-1, 8, 0, BLOCK_SPRUCE_LEAVES}, {1, 8, 0, BLOCK_SPRUCE_LEAVES},
    {0, 8, -1, BLOCK_SPRUCE_LEAVES}, {0, 8, 1, BLOCK_SPRUCE_LEAVES},
    {-1, 8, -1, BLOCK_SPRUCE_LEAVES}, {1, 8, -1, BLOCK_SPRUCE_LEAVES},
    {-1, 8, 1, BLOCK_SPRUCE_LEAVES}, {1, 8, 1, BLOCK_SPRUCE_LEAVES},

    // Layer at y=9 (around trunk)
    {-1, 9, 0, BLOCK_SPRUCE_LEAVES}, {1, 9, 0, BLOCK_SPRUCE_LEAVES},
    {0, 9, -1, BLOCK_SPRUCE_LEAVES}, {0, 9, 1, BLOCK_SPRUCE_LEAVES},

    // Top at y=10
    {0, 10, 0, BLOCK_SPRUCE_LEAVES},
};
#define LARGE_SPRUCE_COUNT (sizeof(LARGE_SPRUCE) / sizeof(TreeBlock))

// ============================================================================
// ACACIA TREE TEMPLATES (flat top, angled trunk)
// ============================================================================

// Small Acacia: 6 blocks tall with flat canopy
static const TreeBlock SMALL_ACACIA[] = {
    // Trunk (y=0 to y=2, straight)
    {0, 0, 0, BLOCK_ACACIA_WOOD},
    {0, 1, 0, BLOCK_ACACIA_WOOD},
    {0, 2, 0, BLOCK_ACACIA_WOOD},
    // Diagonal section (y=3 to y=4)
    {1, 3, 0, BLOCK_ACACIA_WOOD},
    {1, 4, 0, BLOCK_ACACIA_WOOD},

    // Flat canopy at y=5 (5x5)
    {0, 5, 0, BLOCK_ACACIA_LEAVES}, {1, 5, 0, BLOCK_ACACIA_LEAVES}, {2, 5, 0, BLOCK_ACACIA_LEAVES},
    {-1, 5, 0, BLOCK_ACACIA_LEAVES}, {3, 5, 0, BLOCK_ACACIA_LEAVES},
    {0, 5, -1, BLOCK_ACACIA_LEAVES}, {1, 5, -1, BLOCK_ACACIA_LEAVES}, {2, 5, -1, BLOCK_ACACIA_LEAVES},
    {-1, 5, -1, BLOCK_ACACIA_LEAVES}, {3, 5, -1, BLOCK_ACACIA_LEAVES},
    {0, 5, 1, BLOCK_ACACIA_LEAVES}, {1, 5, 1, BLOCK_ACACIA_LEAVES}, {2, 5, 1, BLOCK_ACACIA_LEAVES},
    {-1, 5, 1, BLOCK_ACACIA_LEAVES}, {3, 5, 1, BLOCK_ACACIA_LEAVES},
    {0, 5, -2, BLOCK_ACACIA_LEAVES}, {1, 5, -2, BLOCK_ACACIA_LEAVES}, {2, 5, -2, BLOCK_ACACIA_LEAVES},
    {0, 5, 2, BLOCK_ACACIA_LEAVES}, {1, 5, 2, BLOCK_ACACIA_LEAVES}, {2, 5, 2, BLOCK_ACACIA_LEAVES},

    // Small top layer at y=6
    {0, 6, 0, BLOCK_ACACIA_LEAVES}, {1, 6, 0, BLOCK_ACACIA_LEAVES}, {2, 6, 0, BLOCK_ACACIA_LEAVES},
    {1, 6, -1, BLOCK_ACACIA_LEAVES}, {1, 6, 1, BLOCK_ACACIA_LEAVES},
};
#define SMALL_ACACIA_COUNT (sizeof(SMALL_ACACIA) / sizeof(TreeBlock))

// Medium Acacia: 7 blocks tall with wider canopy
static const TreeBlock MEDIUM_ACACIA[] = {
    // Trunk (y=0 to y=2, straight)
    {0, 0, 0, BLOCK_ACACIA_WOOD},
    {0, 1, 0, BLOCK_ACACIA_WOOD},
    {0, 2, 0, BLOCK_ACACIA_WOOD},
    // Diagonal section (y=3 to y=5)
    {1, 3, 0, BLOCK_ACACIA_WOOD},
    {1, 4, 0, BLOCK_ACACIA_WOOD},
    {2, 5, 0, BLOCK_ACACIA_WOOD},

    // Flat canopy at y=6 (7x5)
    {0, 6, 0, BLOCK_ACACIA_LEAVES}, {1, 6, 0, BLOCK_ACACIA_LEAVES}, {2, 6, 0, BLOCK_ACACIA_LEAVES},
    {3, 6, 0, BLOCK_ACACIA_LEAVES}, {4, 6, 0, BLOCK_ACACIA_LEAVES},
    {-1, 6, 0, BLOCK_ACACIA_LEAVES},
    {0, 6, -1, BLOCK_ACACIA_LEAVES}, {1, 6, -1, BLOCK_ACACIA_LEAVES}, {2, 6, -1, BLOCK_ACACIA_LEAVES},
    {3, 6, -1, BLOCK_ACACIA_LEAVES}, {4, 6, -1, BLOCK_ACACIA_LEAVES},
    {0, 6, 1, BLOCK_ACACIA_LEAVES}, {1, 6, 1, BLOCK_ACACIA_LEAVES}, {2, 6, 1, BLOCK_ACACIA_LEAVES},
    {3, 6, 1, BLOCK_ACACIA_LEAVES}, {4, 6, 1, BLOCK_ACACIA_LEAVES},
    {0, 6, -2, BLOCK_ACACIA_LEAVES}, {1, 6, -2, BLOCK_ACACIA_LEAVES}, {2, 6, -2, BLOCK_ACACIA_LEAVES},
    {3, 6, -2, BLOCK_ACACIA_LEAVES},
    {0, 6, 2, BLOCK_ACACIA_LEAVES}, {1, 6, 2, BLOCK_ACACIA_LEAVES}, {2, 6, 2, BLOCK_ACACIA_LEAVES},
    {3, 6, 2, BLOCK_ACACIA_LEAVES},

    // Small top layer at y=7
    {1, 7, 0, BLOCK_ACACIA_LEAVES}, {2, 7, 0, BLOCK_ACACIA_LEAVES}, {3, 7, 0, BLOCK_ACACIA_LEAVES},
    {2, 7, -1, BLOCK_ACACIA_LEAVES}, {2, 7, 1, BLOCK_ACACIA_LEAVES},
};
#define MEDIUM_ACACIA_COUNT (sizeof(MEDIUM_ACACIA) / sizeof(TreeBlock))

// Large Acacia: 8 blocks tall with largest canopy
static const TreeBlock LARGE_ACACIA[] = {
    // Trunk (y=0 to y=3, straight)
    {0, 0, 0, BLOCK_ACACIA_WOOD},
    {0, 1, 0, BLOCK_ACACIA_WOOD},
    {0, 2, 0, BLOCK_ACACIA_WOOD},
    {0, 3, 0, BLOCK_ACACIA_WOOD},
    // Diagonal section (y=4 to y=6)
    {1, 4, 0, BLOCK_ACACIA_WOOD},
    {1, 5, 0, BLOCK_ACACIA_WOOD},
    {2, 6, 0, BLOCK_ACACIA_WOOD},

    // Flat canopy at y=7 (7x7)
    {0, 7, 0, BLOCK_ACACIA_LEAVES}, {1, 7, 0, BLOCK_ACACIA_LEAVES}, {2, 7, 0, BLOCK_ACACIA_LEAVES},
    {3, 7, 0, BLOCK_ACACIA_LEAVES}, {4, 7, 0, BLOCK_ACACIA_LEAVES},
    {-1, 7, 0, BLOCK_ACACIA_LEAVES}, {5, 7, 0, BLOCK_ACACIA_LEAVES},
    {0, 7, -1, BLOCK_ACACIA_LEAVES}, {1, 7, -1, BLOCK_ACACIA_LEAVES}, {2, 7, -1, BLOCK_ACACIA_LEAVES},
    {3, 7, -1, BLOCK_ACACIA_LEAVES}, {4, 7, -1, BLOCK_ACACIA_LEAVES},
    {-1, 7, -1, BLOCK_ACACIA_LEAVES}, {5, 7, -1, BLOCK_ACACIA_LEAVES},
    {0, 7, 1, BLOCK_ACACIA_LEAVES}, {1, 7, 1, BLOCK_ACACIA_LEAVES}, {2, 7, 1, BLOCK_ACACIA_LEAVES},
    {3, 7, 1, BLOCK_ACACIA_LEAVES}, {4, 7, 1, BLOCK_ACACIA_LEAVES},
    {-1, 7, 1, BLOCK_ACACIA_LEAVES}, {5, 7, 1, BLOCK_ACACIA_LEAVES},
    {0, 7, -2, BLOCK_ACACIA_LEAVES}, {1, 7, -2, BLOCK_ACACIA_LEAVES}, {2, 7, -2, BLOCK_ACACIA_LEAVES},
    {3, 7, -2, BLOCK_ACACIA_LEAVES}, {4, 7, -2, BLOCK_ACACIA_LEAVES},
    {0, 7, 2, BLOCK_ACACIA_LEAVES}, {1, 7, 2, BLOCK_ACACIA_LEAVES}, {2, 7, 2, BLOCK_ACACIA_LEAVES},
    {3, 7, 2, BLOCK_ACACIA_LEAVES}, {4, 7, 2, BLOCK_ACACIA_LEAVES},
    {1, 7, -3, BLOCK_ACACIA_LEAVES}, {2, 7, -3, BLOCK_ACACIA_LEAVES}, {3, 7, -3, BLOCK_ACACIA_LEAVES},
    {1, 7, 3, BLOCK_ACACIA_LEAVES}, {2, 7, 3, BLOCK_ACACIA_LEAVES}, {3, 7, 3, BLOCK_ACACIA_LEAVES},

    // Top layer at y=8
    {1, 8, 0, BLOCK_ACACIA_LEAVES}, {2, 8, 0, BLOCK_ACACIA_LEAVES}, {3, 8, 0, BLOCK_ACACIA_LEAVES},
    {2, 8, -1, BLOCK_ACACIA_LEAVES}, {2, 8, 1, BLOCK_ACACIA_LEAVES},
    {1, 8, -1, BLOCK_ACACIA_LEAVES}, {3, 8, -1, BLOCK_ACACIA_LEAVES},
    {1, 8, 1, BLOCK_ACACIA_LEAVES}, {3, 8, 1, BLOCK_ACACIA_LEAVES},
};
#define LARGE_ACACIA_COUNT (sizeof(LARGE_ACACIA) / sizeof(TreeBlock))

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static bool is_wood_block(BlockType type) {
    return type == BLOCK_WOOD ||
           type == BLOCK_BIRCH_WOOD ||
           type == BLOCK_SPRUCE_WOOD ||
           type == BLOCK_ACACIA_WOOD;
}

static bool is_leaf_block(BlockType type) {
    return type == BLOCK_LEAVES ||
           type == BLOCK_BIRCH_LEAVES ||
           type == BLOCK_SPRUCE_LEAVES ||
           type == BLOCK_ACACIA_LEAVES;
}

// Template accessor for typed trees
static const TreeBlock* get_template_typed(TreeSize size, TreeType type, int* count) {
    switch (type) {
        case TREE_TYPE_OAK:
            switch (size) {
                case TREE_SMALL:  *count = SMALL_OAK_COUNT;  return SMALL_OAK;
                case TREE_MEDIUM: *count = MEDIUM_OAK_COUNT; return MEDIUM_OAK;
                case TREE_LARGE:  *count = LARGE_OAK_COUNT;  return LARGE_OAK;
                default:          *count = SMALL_OAK_COUNT;  return SMALL_OAK;
            }
        case TREE_TYPE_BIRCH:
            switch (size) {
                case TREE_SMALL:  *count = SMALL_BIRCH_COUNT;  return SMALL_BIRCH;
                case TREE_MEDIUM: *count = MEDIUM_BIRCH_COUNT; return MEDIUM_BIRCH;
                case TREE_LARGE:  *count = LARGE_BIRCH_COUNT;  return LARGE_BIRCH;
                default:          *count = SMALL_BIRCH_COUNT;  return SMALL_BIRCH;
            }
        case TREE_TYPE_SPRUCE:
            switch (size) {
                case TREE_SMALL:  *count = SMALL_SPRUCE_COUNT;  return SMALL_SPRUCE;
                case TREE_MEDIUM: *count = MEDIUM_SPRUCE_COUNT; return MEDIUM_SPRUCE;
                case TREE_LARGE:  *count = LARGE_SPRUCE_COUNT;  return LARGE_SPRUCE;
                default:          *count = SMALL_SPRUCE_COUNT;  return SMALL_SPRUCE;
            }
        case TREE_TYPE_ACACIA:
            switch (size) {
                case TREE_SMALL:  *count = SMALL_ACACIA_COUNT;  return SMALL_ACACIA;
                case TREE_MEDIUM: *count = MEDIUM_ACACIA_COUNT; return MEDIUM_ACACIA;
                case TREE_LARGE:  *count = LARGE_ACACIA_COUNT;  return LARGE_ACACIA;
                default:          *count = SMALL_ACACIA_COUNT;  return SMALL_ACACIA;
            }
        default:
            *count = SMALL_OAK_COUNT;
            return SMALL_OAK;
    }
}

// Template accessor (legacy, uses Oak)
static const TreeBlock* get_template(TreeSize size, int* count) {
    switch (size) {
        case TREE_SMALL:
            *count = SMALL_OAK_COUNT;
            return SMALL_OAK;
        case TREE_MEDIUM:
            *count = MEDIUM_OAK_COUNT;
            return MEDIUM_OAK;
        case TREE_LARGE:
            *count = LARGE_OAK_COUNT;
            return LARGE_OAK;
        default:
            *count = SMALL_OAK_COUNT;
            return SMALL_OAK;
    }
}

// ============================================================================
// TREE PLACEMENT
// ============================================================================

void tree_place_at(Chunk* chunk, int local_x, int base_y, int local_z, TreeSize size) {
    if (!chunk) return;

    int count;
    const TreeBlock* template = get_template(size, &count);

    for (int i = 0; i < count; i++) {
        int x = local_x + template[i].dx;
        int y = base_y + template[i].dy;
        int z = local_z + template[i].dz;

        // Check bounds - skip blocks outside chunk
        if (x < 0 || x >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) continue;
        if (y < 0 || y >= CHUNK_HEIGHT) continue;

        // Don't overwrite existing solid blocks (except air)
        Block existing = chunk_get_block(chunk, x, y, z);
        if (existing.type != BLOCK_AIR && existing.type != BLOCK_LEAVES) continue;

        // Create block with metadata=1 for natural tree
        Block block = {
            .type = template[i].type,
            .light_level = 0,
            .metadata = 1  // Mark as natural tree
        };

        chunk_set_block(chunk, x, y, z, block);
    }
}

void tree_place_at_typed(Chunk* chunk, int local_x, int base_y, int local_z,
                         TreeSize size, TreeType type) {
    if (!chunk) return;

    int count;
    const TreeBlock* template = get_template_typed(size, type, &count);

    for (int i = 0; i < count; i++) {
        int x = local_x + template[i].dx;
        int y = base_y + template[i].dy;
        int z = local_z + template[i].dz;

        // Check bounds - skip blocks outside chunk
        if (x < 0 || x >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) continue;
        if (y < 0 || y >= CHUNK_HEIGHT) continue;

        // Don't overwrite existing solid blocks (except air and leaves)
        Block existing = chunk_get_block(chunk, x, y, z);
        if (existing.type != BLOCK_AIR && !is_leaf_block(existing.type)) continue;

        // Create block with metadata=1 for natural tree
        Block block = {
            .type = template[i].type,
            .light_level = 0,
            .metadata = 1  // Mark as natural tree
        };

        chunk_set_block(chunk, x, y, z, block);
    }
}

// ============================================================================
// TREE GENERATION
// ============================================================================

// Find the surface height at a column (checks for any biome surface block)
static int find_surface_y(Chunk* chunk, int x, int z, BlockType surface_block) {
    for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
        Block block = chunk_get_block(chunk, x, y, z);
        if (block.type == surface_block) {
            return y;
        }
        // Stop if we hit other solid blocks (not air)
        if (block.type != BLOCK_AIR && block.type != BLOCK_LEAVES) {
            return -1;  // Not expected surface
        }
    }
    return -1;  // No surface found
}

// Place a cactus at position (1-3 blocks tall)
static void place_cactus(Chunk* chunk, int x, int surface_y, int z) {
    int world_x = chunk->x * CHUNK_SIZE + x;
    int world_z = chunk->z * CHUNK_SIZE + z;

    // Use position hash for height variation
    uint32_t hash = (uint32_t)(world_x * 73856093 ^ world_z * 19349663);
    int height = 1 + (hash % 3);  // 1-3 blocks tall

    for (int i = 0; i < height; i++) {
        int y = surface_y + 1 + i;
        if (y < CHUNK_HEIGHT) {
            Block block = {BLOCK_CACTUS, 0, 0};
            chunk_set_block(chunk, x, y, z, block);
        }
    }
}

void tree_generate_for_chunk(Chunk* chunk) {
    if (!chunk) return;

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            // Calculate world coordinates
            int world_x = chunk->x * CHUNK_SIZE + x;
            int world_z = chunk->z * CHUNK_SIZE + z;

            // Get biome at this position
            BiomeType biome = biome_get_at(world_x, world_z);
            const BiomeProperties* bp = biome_get_properties(biome);

            // Find surface using biome-specific surface block
            int surface_y = find_surface_y(chunk, x, z, bp->surface_block);
            if (surface_y < 0) continue;

            // Vegetation noise (offset to be different from terrain)
            float veg_noise = noise_2d(
                (float)world_x * 0.08f + 5000.0f,
                (float)world_z * 0.08f + 5000.0f
            );

            // Desert biome: place cacti in addition to sparse trees
            if (bp->has_cacti) {
                // Cacti are sparse (use different threshold)
                if (veg_noise > 0.75f) {
                    place_cactus(chunk, x, surface_y, z);
                    continue;  // Don't place a tree where we placed a cactus
                }
            }

            // Skip if biome doesn't have trees
            if (!bp->has_trees) continue;

            // Calculate tree placement threshold based on biome density
            // Higher density = lower threshold = more trees
            float threshold = 1.0f - bp->tree_density;

            // Only place trees where noise exceeds threshold
            if (veg_noise > threshold) {
                // Check for minimum spacing (skip if another tree trunk nearby)
                bool too_close = false;
                for (int dx = -3; dx <= 3 && !too_close; dx++) {
                    for (int dz = -3; dz <= 3 && !too_close; dz++) {
                        if (dx == 0 && dz == 0) continue;
                        int nx = x + dx;
                        int nz = z + dz;
                        if (nx >= 0 && nx < CHUNK_SIZE && nz >= 0 && nz < CHUNK_SIZE) {
                            // Check if there's any wood block or cactus near the surface
                            for (int dy = 0; dy <= 3; dy++) {
                                Block neighbor = chunk_get_block(chunk, nx, surface_y + 1 + dy, nz);
                                if (is_wood_block(neighbor.type) || neighbor.type == BLOCK_CACTUS) {
                                    too_close = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (!too_close) {
                    // Pick tree size and type based on position hash
                    uint32_t hash = (uint32_t)(world_x * 73856093 ^ world_z * 19349663);
                    TreeSize size = (TreeSize)(hash % TREE_SIZE_COUNT);

                    // Determine tree type from biome
                    TreeType tree_type = bp->primary_tree;
                    if (bp->secondary_tree != TREE_TYPE_COUNT) {
                        // Use different hash bits for tree type selection
                        float roll = (float)((hash >> 16) % 1000) / 1000.0f;
                        if (roll < bp->secondary_chance) {
                            tree_type = bp->secondary_tree;
                        }
                    }

                    // Place tree with correct type (trunk starts above surface)
                    tree_place_at_typed(chunk, x, surface_y + 1, z, size, tree_type);
                }
            }
        }
    }

    // Update chunk status after placing vegetation
    chunk->needs_remesh = true;
    chunk->is_empty = false;
}

// ============================================================================
// LEAF DECAY SYSTEM
// ============================================================================

#define MAX_DECAY_QUEUE 256
#define LEAF_DECAY_RANGE 4  // Max distance leaves can be from wood

typedef struct {
    int x, y, z;
    float timer;  // Seconds until decay check
} DecayEntry;

static DecayEntry g_decay_queue[MAX_DECAY_QUEUE];
static int g_decay_count = 0;

void leaf_decay_init(void) {
    g_decay_count = 0;
}

// Check if position is already in decay queue
static bool is_in_decay_queue(int x, int y, int z) {
    for (int i = 0; i < g_decay_count; i++) {
        if (g_decay_queue[i].x == x &&
            g_decay_queue[i].y == y &&
            g_decay_queue[i].z == z) {
            return true;
        }
    }
    return false;
}

// Add a leaf to the decay queue
static void add_to_decay_queue(int x, int y, int z) {
    if (g_decay_count >= MAX_DECAY_QUEUE) return;
    if (is_in_decay_queue(x, y, z)) return;

    g_decay_queue[g_decay_count].x = x;
    g_decay_queue[g_decay_count].y = y;
    g_decay_queue[g_decay_count].z = z;
    // Random delay between 0.5 and 2.0 seconds
    g_decay_queue[g_decay_count].timer = 0.5f + ((float)(rand() % 150) / 100.0f);
    g_decay_count++;
}

void leaf_decay_on_wood_removed(struct World* world, int x, int y, int z) {
    if (!world) return;

    // Check all blocks within LEAF_DECAY_RANGE for leaves
    for (int dx = -LEAF_DECAY_RANGE; dx <= LEAF_DECAY_RANGE; dx++) {
        for (int dy = -LEAF_DECAY_RANGE; dy <= LEAF_DECAY_RANGE; dy++) {
            for (int dz = -LEAF_DECAY_RANGE; dz <= LEAF_DECAY_RANGE; dz++) {
                int nx = x + dx;
                int ny = y + dy;
                int nz = z + dz;

                Block block = world_get_block(world, nx, ny, nz);
                if (is_leaf_block(block.type)) {
                    add_to_decay_queue(nx, ny, nz);
                }
            }
        }
    }
}

// Check if a leaf at (x,y,z) is connected to wood within range
static bool leaf_is_supported(struct World* world, int x, int y, int z) {
    // BFS to find wood within LEAF_DECAY_RANGE blocks
    typedef struct { int x, y, z, dist; } SearchNode;
    SearchNode queue[512];
    int queue_head = 0, queue_tail = 0;

    // Simple visited tracking
    #define VISITED_SIZE 512
    int visited_x[VISITED_SIZE];
    int visited_y[VISITED_SIZE];
    int visited_z[VISITED_SIZE];
    int visited_count = 0;

    // Start from the leaf position
    queue[queue_tail++] = (SearchNode){x, y, z, 0};

    while (queue_head < queue_tail) {
        SearchNode node = queue[queue_head++];

        if (node.dist > LEAF_DECAY_RANGE) continue;

        // Check if already visited
        bool already_visited = false;
        for (int i = 0; i < visited_count; i++) {
            if (visited_x[i] == node.x && visited_y[i] == node.y && visited_z[i] == node.z) {
                already_visited = true;
                break;
            }
        }
        if (already_visited) continue;

        // Mark visited
        if (visited_count < VISITED_SIZE) {
            visited_x[visited_count] = node.x;
            visited_y[visited_count] = node.y;
            visited_z[visited_count] = node.z;
            visited_count++;
        }

        Block block = world_get_block(world, node.x, node.y, node.z);

        // Found wood! Leaf is supported
        if (is_wood_block(block.type)) {
            return true;
        }

        // Continue searching through leaves
        if (is_leaf_block(block.type) || (node.x == x && node.y == y && node.z == z)) {
            // Add neighbors
            if (queue_tail < 500) {
                queue[queue_tail++] = (SearchNode){node.x + 1, node.y, node.z, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x - 1, node.y, node.z, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x, node.y + 1, node.z, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x, node.y - 1, node.z, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x, node.y, node.z + 1, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x, node.y, node.z - 1, node.dist + 1};
            }
        }
    }

    return false;  // No wood found within range
}

void leaf_decay_update(struct World* world, float dt) {
    if (!world || g_decay_count == 0) return;

    // Process decay queue
    for (int i = g_decay_count - 1; i >= 0; i--) {
        g_decay_queue[i].timer -= dt;

        if (g_decay_queue[i].timer <= 0) {
            int x = g_decay_queue[i].x;
            int y = g_decay_queue[i].y;
            int z = g_decay_queue[i].z;

            // Check if block is still a leaf
            Block block = world_get_block(world, x, y, z);
            if (is_leaf_block(block.type)) {
                // Check if connected to wood
                if (!leaf_is_supported(world, x, y, z)) {
                    // Decay the leaf
                    Block air = {BLOCK_AIR, 0, 0};
                    world_set_block(world, x, y, z, air);

                    // Add nearby leaves to queue (chain reaction)
                    for (int dx = -1; dx <= 1; dx++) {
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dz = -1; dz <= 1; dz++) {
                                if (dx == 0 && dy == 0 && dz == 0) continue;
                                Block neighbor = world_get_block(world, x + dx, y + dy, z + dz);
                                if (is_leaf_block(neighbor.type)) {
                                    add_to_decay_queue(x + dx, y + dy, z + dz);
                                }
                            }
                        }
                    }
                }
            }

            // Remove from queue (swap with last)
            g_decay_queue[i] = g_decay_queue[g_decay_count - 1];
            g_decay_count--;
        }
    }
}
