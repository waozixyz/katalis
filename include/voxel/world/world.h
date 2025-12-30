/**
 * World System - Voxel Engine
 *
 * Manages multiple chunks with loading/unloading
 */

#ifndef VOXEL_WORLD_H
#define VOXEL_WORLD_H

#include "voxel/world/chunk.h"
#include "voxel/world/terrain.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct ChunkWorker ChunkWorker;
typedef struct Player Player;
typedef struct EntityManager EntityManager;
typedef struct WaterUpdateQueue WaterUpdateQueue;
typedef struct ChestRegistry ChestRegistry;

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

typedef struct World {
    ChunkHashMap* chunks;
    ChunkWorker* worker;     // Multi-threaded chunk generation
    int center_chunk_x;      // Center of loaded chunks (camera position)
    int center_chunk_z;
    int view_distance;       // How many chunks to load around center
    TerrainParams terrain_params;  // Terrain generation parameters
    Player* player;          // Reference to player (for entity AI)
    EntityManager* entity_manager;  // Entity manager for mobs
    float time_of_day;       // Current time (0-24 hours) for lighting
    WaterUpdateQueue* water_queue;  // Water flow update system
    int game_tick;           // Game tick counter for water updates
    ChestRegistry* chest_registry;  // Chest data storage
} World;

// ============================================================================
// API
// ============================================================================

/**
 * Create a new world with terrain generation parameters
 */
World* world_create(TerrainParams terrain_params);

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
 * Render all visible chunks with time-based ambient lighting
 */
void world_render_with_time(World* world, float time_of_day);

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

/**
 * Get ambient light color for a given time of day
 * Returns RGB values in range 0.0 to 1.0
 */
Vector3 world_get_ambient_color(float time_of_day);

/**
 * Get the entity manager for this world
 */
EntityManager* world_get_entity_manager(World* world);

/**
 * Set the entity manager for this world
 */
void world_set_entity_manager(World* world, EntityManager* manager);

#endif // VOXEL_WORLD_H
