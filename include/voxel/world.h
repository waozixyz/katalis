/**
 * World System - Voxel Engine
 *
 * Manages multiple chunks with loading/unloading
 */

#ifndef VOXEL_WORLD_H
#define VOXEL_WORLD_H

#include "voxel/chunk.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// WORLD CONSTANTS
// ============================================================================

#define WORLD_MAX_CHUNKS 1024        // Maximum chunks loaded at once
#define WORLD_VIEW_DISTANCE 8        // Chunks visible in each direction

// ============================================================================
// CHUNK HASH MAP
// ============================================================================

typedef struct ChunkNode {
    int chunk_x;
    int chunk_z;
    Chunk* chunk;
    struct ChunkNode* next;  // For hash collision chaining
} ChunkNode;

typedef struct {
    ChunkNode* buckets[WORLD_MAX_CHUNKS];
    int chunk_count;
} ChunkHashMap;

// ============================================================================
// WORLD DATA
// ============================================================================

typedef struct {
    ChunkHashMap* chunks;
    int center_chunk_x;      // Center of loaded chunks (camera position)
    int center_chunk_z;
    int view_distance;       // How many chunks to load around center
} World;

// ============================================================================
// API
// ============================================================================

/**
 * Create a new world
 */
World* world_create(void);

/**
 * Destroy world and free all chunks
 */
void world_destroy(World* world);

/**
 * Get chunk at chunk coordinates (not block coordinates)
 * Returns NULL if chunk not loaded
 */
Chunk* world_get_chunk(World* world, int chunk_x, int chunk_z);

/**
 * Get or create chunk at chunk coordinates
 * If chunk doesn't exist, creates and adds it to world
 */
Chunk* world_get_or_create_chunk(World* world, int chunk_x, int chunk_z);

/**
 * Get block at world coordinates
 */
Block world_get_block(World* world, int x, int y, int z);

/**
 * Set block at world coordinates
 */
void world_set_block(World* world, int x, int y, int z, Block block);

/**
 * Update world - load/unload chunks based on center position
 * Call this when camera moves to stream chunks
 */
void world_update(World* world, int center_chunk_x, int center_chunk_z);

/**
 * Render all visible chunks
 */
void world_render(World* world);

/**
 * Convert world coordinates to chunk coordinates
 */
void world_to_chunk_coords(int world_x, int world_z, int* chunk_x, int* chunk_z);

/**
 * Convert world coordinates to local chunk coordinates
 */
void world_to_local_coords(int world_x, int world_y, int world_z,
                           int* chunk_x, int* chunk_z,
                           int* local_x, int* local_y, int* local_z);

#endif // VOXEL_WORLD_H
