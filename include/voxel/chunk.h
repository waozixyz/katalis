/**
 * Chunk System - Voxel Engine
 *
 * Manages chunk data and mesh generation
 */

#ifndef VOXEL_CHUNK_H
#define VOXEL_CHUNK_H

#include "voxel/block.h"
#include <raylib.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// CHUNK CONSTANTS
// ============================================================================

#define CHUNK_SIZE 16      // Width and depth
#define CHUNK_HEIGHT 256   // Maximum height

// ============================================================================
// CHUNK DATA
// ============================================================================

typedef struct {
    int x, z;                                                  // Chunk position in world
    Block blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];      // Block data (XYZ)
    Mesh mesh;                                                 // Raylib mesh for rendering
    bool needs_remesh;                                         // Dirty flag
    bool is_empty;                                             // Optimization: all air
    bool mesh_generated;                                       // Has mesh been created?
} Chunk;

// ============================================================================
// API
// ============================================================================

/**
 * Create a new chunk at world position (x, z)
 */
Chunk* chunk_create(int x, int z);

/**
 * Destroy chunk and free resources
 */
void chunk_destroy(Chunk* chunk);

/**
 * Set block at local coordinates (0-15, 0-255, 0-15)
 */
void chunk_set_block(Chunk* chunk, int x, int y, int z, Block block);

/**
 * Get block at local coordinates
 */
Block chunk_get_block(Chunk* chunk, int x, int y, int z);

/**
 * Check if coordinates are within chunk bounds
 */
bool chunk_in_bounds(int x, int y, int z);

/**
 * Generate mesh for chunk (greedy meshing)
 */
void chunk_generate_mesh(Chunk* chunk);

/**
 * Update mesh if needed (checks dirty flag)
 */
void chunk_update_mesh(Chunk* chunk);

/**
 * Fill chunk with specific block type (for testing)
 */
void chunk_fill(Chunk* chunk, BlockType type);

/**
 * Check if chunk is empty (all air blocks)
 */
bool chunk_is_empty(Chunk* chunk);

/**
 * Update the is_empty flag by scanning the chunk
 * Call this after bulk terrain generation
 */
void chunk_update_empty_status(Chunk* chunk);

#endif // VOXEL_CHUNK_H
