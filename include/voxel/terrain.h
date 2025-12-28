/**
 * Terrain Generator - Procedural Voxel Terrain
 *
 * Generates natural terrain using noise functions
 */

#ifndef VOXEL_TERRAIN_H
#define VOXEL_TERRAIN_H

#include "voxel/chunk.h"
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
    float cave_threshold;        // Cave formation threshold (default: 0.3)
    float cave_frequency;        // Cave noise frequency (default: 0.05)
    int cave_octaves;            // Cave noise octaves (default: 2)

    // Biome settings
    bool generate_biomes;        // Enable biome variation (default: false)
    float biome_frequency;       // Biome transition frequency (default: 0.005)

    // Block placement
    int dirt_depth;              // Dirt layers below grass (default: 4)
    int stone_depth;             // Stone layers below dirt (default: 64)
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
