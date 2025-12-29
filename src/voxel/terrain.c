/**
 * Terrain Generator Implementation
 */

#include "voxel/terrain.h"
#include "voxel/noise.h"
#include "voxel/block.h"
#include "voxel/tree.h"
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

    // Deep layer configuration
    params.deep_stone_start = 32;
    params.bedrock_start = 8;
    params.bedrock_solid = 4;

    // Ore generation - Coal (common)
    params.coal_frequency = 0.15f;
    params.coal_min_y = 64;
    params.coal_max_y = 80;

    // Ore generation - Iron (uncommon)
    params.iron_frequency = 0.10f;
    params.iron_min_y = 24;
    params.iron_max_y = 48;

    // Ore generation - Gold (rare)
    params.gold_frequency = 0.05f;
    params.gold_min_y = 16;
    params.gold_max_y = 32;

    // Ore generation - Diamond (very rare)
    params.diamond_frequency = 0.02f;
    params.diamond_min_y = 8;
    params.diamond_max_y = 16;

    // Gravel layer
    params.gravel_frequency = 0.20f;
    params.gravel_min_y = 32;
    params.gravel_max_y = 48;

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
    // 1. Air above terrain
    if (world_y > terrain_height) {
        return BLOCK_AIR;
    }

    // 2. Grass surface
    if (world_y == terrain_height) {
        return BLOCK_GRASS;
    }

    // 3. Dirt subsurface
    if (world_y > terrain_height - params.dirt_depth) {
        return BLOCK_DIRT;
    }

    // 4. Solid bedrock foundation (y=0-4)
    if (world_y <= params.bedrock_solid) {
        return BLOCK_BEDROCK;
    }

    // 5. Mixed bedrock layer (y=5-8) with gradient
    if (world_y <= params.bedrock_start) {
        float bedrock_noise = noise_3d(
            (float)world_x * 0.1f,
            (float)world_y * 0.1f,
            (float)world_z * 0.1f
        );
        // More bedrock closer to y=0
        float bedrock_chance = (float)(params.bedrock_start - world_y) / 4.0f;
        if (bedrock_noise < bedrock_chance) {
            return BLOCK_BEDROCK;
        }
    }

    // 6. Deep stone layer (y=9-32)
    if (world_y <= params.deep_stone_start) {
        // Check for diamond ore (y=8-16, very rare 2%)
        if (world_y >= params.diamond_min_y && world_y <= params.diamond_max_y) {
            float ore_noise = noise_3d(
                (float)world_x * 0.2f,
                (float)world_y * 0.2f,
                (float)world_z * 0.2f
            );
            if (ore_noise > (1.0f - params.diamond_frequency)) {
                return BLOCK_DIAMOND_ORE;
            }
        }

        // Check for gold ore (y=16-32, rare 5%)
        if (world_y >= params.gold_min_y && world_y <= params.gold_max_y) {
            float ore_noise = noise_3d(
                (float)world_x * 0.2f + 1000.0f,  // Offset for different pattern
                (float)world_y * 0.2f,
                (float)world_z * 0.2f
            );
            if (ore_noise > (1.0f - params.gold_frequency)) {
                return BLOCK_GOLD_ORE;
            }
        }

        return BLOCK_DEEP_STONE;
    }

    // 7. Gravel pockets (y=32-48, 20%)
    if (world_y >= params.gravel_min_y && world_y <= params.gravel_max_y) {
        float gravel_noise = noise_3d(
            (float)world_x * 0.15f,
            (float)world_y * 0.15f,
            (float)world_z * 0.15f
        );
        if (gravel_noise > (1.0f - params.gravel_frequency)) {
            return BLOCK_GRAVEL;
        }
    }

    // 8. Standard stone with ores
    // Check for iron ore (y=24-48, uncommon 10%)
    if (world_y >= params.iron_min_y && world_y <= params.iron_max_y) {
        float ore_noise = noise_3d(
            (float)world_x * 0.2f + 2000.0f,
            (float)world_y * 0.2f,
            (float)world_z * 0.2f
        );
        if (ore_noise > (1.0f - params.iron_frequency)) {
            return BLOCK_IRON_ORE;
        }
    }

    // Check for coal ore (y=64-80, common 15%)
    if (world_y >= params.coal_min_y && world_y <= params.coal_max_y) {
        float ore_noise = noise_3d(
            (float)world_x * 0.2f + 3000.0f,
            (float)world_y * 0.2f,
            (float)world_z * 0.2f
        );
        if (ore_noise > (1.0f - params.coal_frequency)) {
            return BLOCK_COAL_ORE;
        }
    }

    // 9. Check for caves (after all solid blocks determined)
    if (is_cave(world_x, world_y, world_z, params)) {
        return BLOCK_AIR;
    }

    // Default: standard stone
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

    // Generate trees on the terrain
    tree_generate_for_chunk(chunk);

    // Mark chunk as needing mesh regeneration
    chunk->needs_remesh = true;
}
