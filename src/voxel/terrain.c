/**
 * Terrain Generator Implementation
 */

#include "voxel/terrain.h"
#include "voxel/noise.h"
#include "voxel/block.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// DEFAULT PARAMETERS
// ============================================================================

TerrainParams terrain_default_params(void) {
    TerrainParams params;

    // Heightmap
    params.height_scale = 32.0f;
    params.height_offset = 64.0f;
    params.height_octaves = 4;
    params.height_frequency = 0.01f;
    params.height_lacunarity = 2.0f;
    params.height_persistence = 0.5f;

    // Caves
    params.generate_caves = true;
    params.cave_threshold = 0.3f;
    params.cave_frequency = 0.05f;
    params.cave_octaves = 2;

    // Biomes
    params.generate_biomes = false;
    params.biome_frequency = 0.005f;

    // Layers
    params.dirt_depth = 4;
    params.stone_depth = 64;

    return params;
}

// ============================================================================
// TERRAIN GENERATION
// ============================================================================

/**
 * Get terrain height at world coordinates
 */
int terrain_get_height_at(int world_x, int world_z, TerrainParams params) {
    float noise_value = noise_fbm_2d(
        (float)world_x, (float)world_z,
        params.height_octaves,
        params.height_frequency,
        1.0f,
        params.height_lacunarity,
        params.height_persistence
    );

    // Convert noise (-1 to 1) to height
    float height = params.height_offset + (noise_value * params.height_scale);
    return (int)height;
}

/**
 * Check if position should be cave (air)
 */
static bool is_cave(int world_x, int world_y, int world_z, TerrainParams params) {
    if (!params.generate_caves) return false;
    if (world_y > params.height_offset + params.height_scale) return false;  // No caves in sky
    if (world_y < 0) return false;  // No caves in void

    float cave_noise = noise_fbm_3d(
        (float)world_x, (float)world_y, (float)world_z,
        params.cave_octaves,
        params.cave_frequency,
        1.0f,
        2.0f,
        0.5f
    );

    return cave_noise > params.cave_threshold;
}

/**
 * Get block type at world coordinates
 */
static BlockType get_terrain_block(int world_x, int world_y, int world_z, int terrain_height, TerrainParams params) {
    // Above terrain = air
    if (world_y > terrain_height) {
        return BLOCK_AIR;
    }

    // Check for caves
    if (is_cave(world_x, world_y, world_z, params)) {
        return BLOCK_AIR;
    }

    // Surface layer = grass
    if (world_y == terrain_height) {
        return BLOCK_GRASS;
    }

    // Dirt layers
    if (world_y > terrain_height - params.dirt_depth) {
        return BLOCK_DIRT;
    }

    // Deep underground = stone
    return BLOCK_STONE;
}

/**
 * Generate terrain for chunk
 */
void terrain_generate_chunk(Chunk* chunk, TerrainParams params) {
    if (!chunk) return;

    // For each column in the chunk
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            // Calculate world coordinates
            int world_x = chunk->x * CHUNK_SIZE + x;
            int world_z = chunk->z * CHUNK_SIZE + z;

            // Get terrain height at this column
            int terrain_height = terrain_get_height_at(world_x, world_z, params);

            // Fill vertical column
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                int world_y = y;

                // Determine block type
                BlockType block_type = get_terrain_block(world_x, world_y, world_z, terrain_height, params);

                // Set block
                Block block = {block_type, 0, 0};
                chunk_set_block(chunk, x, y, z, block);
            }
        }
    }

    // Mark chunk as needing mesh regeneration
    chunk->needs_remesh = true;
}
