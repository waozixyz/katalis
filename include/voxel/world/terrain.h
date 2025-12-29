/**
 * Terrain Generator - Procedural Voxel Terrain
 *
 * Generates natural terrain using noise functions
 */

#ifndef VOXEL_TERRAIN_H
#define VOXEL_TERRAIN_H

#include "voxel/world/chunk.h"
#include <stdint.h>

// ============================================================================
// TERRAIN PARAMETERS
// ============================================================================

typedef struct {
    // Heightmap settings
    float height_scale;          // Multiplier for terrain height (default: 32.0)
    float height_offset;         // Base height offset (default: 64.0)
    int height_octaves;          // Noise octaves for height (default: 4)
    float height_frequency;      // Noise frequency (default: 0.01)
    float height_lacunarity;     // Frequency multiplier (default: 2.0)
    float height_persistence;    // Amplitude multiplier (default: 0.5)

    // Cave settings
    bool generate_caves;         // Enable cave generation (default: true)
    float cave_threshold;        // Cave formation threshold (default: 0.45)
    float cave_frequency;        // Cave noise frequency (default: 0.04)
    int cave_octaves;            // Cave noise octaves (default: 3)
    int cave_min_depth;          // Minimum depth below surface for caves (default: 20)

    // Biome settings
    bool generate_biomes;        // Enable biome variation (default: false)
    float biome_frequency;       // Biome transition frequency (default: 0.005)

    // Block placement - DEEPER SOIL LAYERS
    int dirt_depth;              // Dirt layers below grass (default: 8, was 4)
    int subsoil_depth;           // Additional clay/gravel subsoil (default: 6)
    int stone_depth;             // Stone layers below dirt (default: 64)

    // Deep layer configuration
    int deep_stone_start;        // Y-level where deep stone begins (default: 32)
    int bedrock_start;           // Y-level where bedrock mixing starts (default: 8)
    int bedrock_solid;           // Y-level of solid bedrock (default: 4)

    // Ore generation parameters
    float coal_frequency;        // Coal vein frequency (default: 0.15)
    int coal_min_y;              // Min Y for coal (default: 48)
    int coal_max_y;              // Max Y for coal (default: 80)

    float iron_frequency;        // Iron vein frequency (default: 0.10)
    int iron_min_y;              // Min Y for iron (default: 16)
    int iron_max_y;              // Max Y for iron (default: 48)

    float gold_frequency;        // Gold vein frequency (default: 0.05)
    int gold_min_y;              // Min Y for gold (default: 8)
    int gold_max_y;              // Max Y for gold (default: 32)

    float diamond_frequency;     // Diamond vein frequency (default: 0.02)
    int diamond_min_y;           // Min Y for diamond (default: 4)
    int diamond_max_y;           // Max Y for diamond (default: 16)

    // Gravel and clay layers
    float gravel_frequency;      // Gravel pocket frequency (default: 0.15)
    int gravel_min_y;            // Min Y for gravel (default: 24)
    int gravel_max_y;            // Max Y for gravel (default: 48)

    float clay_frequency;        // Clay deposit frequency (default: 0.10)
    int clay_min_y;              // Min Y for clay (default: 32)
    int clay_max_y;              // Max Y for clay (default: 56)

    // Dungeon settings
    bool generate_dungeons;      // Enable dungeon generation (default: true)
    float dungeon_frequency;     // Chance per chunk of dungeon (default: 0.03)
    int dungeon_min_y;           // Min Y for dungeons (default: 12)
    int dungeon_max_y;           // Max Y for dungeons (default: 40)
    int dungeon_min_size;        // Minimum dungeon room size (default: 5)
    int dungeon_max_size;        // Maximum dungeon room size (default: 9)

    // Cave tunnel (worm) settings - creates connected cave networks
    bool generate_cave_tunnels;  // Enable worm-style cave tunnels
    float tunnel_radius_min;     // Minimum tunnel radius (default: 2.0)
    float tunnel_radius_max;     // Maximum tunnel radius (default: 5.0)
    int tunnel_segments;         // Segments per tunnel (default: 80)
    int tunnels_per_chunk;       // Base tunnels per chunk (default: 4)

    // Cave room settings - large open areas underground
    bool generate_cave_rooms;    // Enable large cave rooms
    int rooms_per_chunk;         // Maximum rooms per chunk (default: 2)
    float room_radius_min;       // Minimum room radius (default: 4.0)
    float room_radius_max;       // Maximum room radius (default: 10.0)
} TerrainParams;

// ============================================================================
// API
// ============================================================================

/**
 * Get default terrain parameters
 */
TerrainParams terrain_default_params(void);

/**
 * Generate terrain for a chunk
 * Fills chunk with terrain based on world position and parameters
 */
void terrain_generate_chunk(Chunk* chunk, TerrainParams params);

/**
 * Get terrain height at world coordinates (for preview/debug)
 */
int terrain_get_height_at(int world_x, int world_z, TerrainParams params);

#endif // VOXEL_TERRAIN_H
