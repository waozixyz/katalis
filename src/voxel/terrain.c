/**
 * Terrain Generator Implementation
 */

#include "voxel/terrain.h"
#include "voxel/noise.h"
#include "voxel/block.h"
#include "voxel/tree.h"
#include "voxel/light.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// DEFAULT PARAMETERS
// ============================================================================

TerrainParams terrain_default_params(void) {
    TerrainParams params;

    // Heightmap - VERY high terrain for massive underground
    params.height_scale = 32.0f;          // Moderate hills
    params.height_offset = 160.0f;        // Surface at y=160 = 150+ blocks underground!
    params.height_octaves = 5;            // Detail
    params.height_frequency = 0.006f;     // Broad rolling hills
    params.height_lacunarity = 2.0f;
    params.height_persistence = 0.45f;

    // Caves - Must be DEEP underground, not near surface
    params.generate_caves = true;
    params.cave_threshold = 0.48f;        // Caves are rare
    params.cave_frequency = 0.035f;       // Smaller caves
    params.cave_octaves = 3;              // Organic shapes
    params.cave_min_depth = 35;           // Caves start 35 blocks below surface!

    // Biomes
    params.generate_biomes = false;
    params.biome_frequency = 0.005f;

    // Layers - MASSIVE SOIL DEPTH
    params.dirt_depth = 12;          // 12 blocks of dirt!
    params.subsoil_depth = 10;       // 10 more blocks of mixed clay/gravel
    params.stone_depth = 100;        // Lots of stone

    // Deep layer configuration - spread across huge underground
    params.deep_stone_start = 64;    // Deep stone below y=64
    params.bedrock_start = 8;
    params.bedrock_solid = 4;

    // Ore generation - Coal (common, upper stone layer)
    params.coal_frequency = 0.10f;
    params.coal_min_y = 80;
    params.coal_max_y = 130;

    // Ore generation - Iron (mid-depths)
    params.iron_frequency = 0.07f;
    params.iron_min_y = 40;
    params.iron_max_y = 100;

    // Ore generation - Gold (deep)
    params.gold_frequency = 0.035f;
    params.gold_min_y = 16;
    params.gold_max_y = 50;

    // Ore generation - Diamond (very rare, near bedrock)
    params.diamond_frequency = 0.012f;
    params.diamond_min_y = 4;
    params.diamond_max_y = 20;

    // Gravel pockets (throughout stone)
    params.gravel_frequency = 0.10f;
    params.gravel_min_y = 30;
    params.gravel_max_y = 100;

    // Clay deposits (in subsoil layer)
    params.clay_frequency = 0.08f;
    params.clay_min_y = 100;
    params.clay_max_y = 140;

    // Dungeons (spread through underground)
    params.generate_dungeons = true;
    params.dungeon_frequency = 0.05f;  // 5% chance per chunk
    params.dungeon_min_y = 20;
    params.dungeon_max_y = 80;
    params.dungeon_min_size = 5;
    params.dungeon_max_size = 9;

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
 * Caves only form at least cave_min_depth blocks below the terrain surface
 */
static bool is_cave(int world_x, int world_y, int world_z, int terrain_height, TerrainParams params) {
    if (!params.generate_caves) return false;
    if (world_y < params.bedrock_start) return false;  // No caves in bedrock layer

    // KEY: Caves must be at least cave_min_depth blocks below the surface
    // This prevents surface holes and makes underground feel deep
    int depth_below_surface = terrain_height - world_y;
    if (depth_below_surface < params.cave_min_depth) return false;

    // Also limit caves to reasonable heights (not near surface)
    if (world_y > 120) return false;

    float cave_noise = noise_fbm_3d(
        (float)world_x, (float)world_y, (float)world_z,
        params.cave_octaves,
        params.cave_frequency,
        1.0f,
        2.0f,
        0.5f
    );

    // Caves get more likely deeper down (gradual increase)
    float depth_factor = (float)depth_below_surface / 50.0f;
    if (depth_factor > 1.0f) depth_factor = 1.0f;
    float adjusted_threshold = params.cave_threshold + (1.0f - depth_factor) * 0.2f;

    return cave_noise > adjusted_threshold;
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

    // 3. Dirt subsurface (8 blocks deep now)
    if (world_y > terrain_height - params.dirt_depth) {
        return BLOCK_DIRT;
    }

    // 4. Subsoil layer - mixed clay and gravel (6 blocks)
    int subsoil_bottom = terrain_height - params.dirt_depth - params.subsoil_depth;
    if (world_y > subsoil_bottom) {
        // Mix of clay and gravel in subsoil
        float subsoil_noise = noise_3d(
            (float)world_x * 0.1f,
            (float)world_y * 0.1f,
            (float)world_z * 0.1f
        );
        if (subsoil_noise > 0.3f) {
            return BLOCK_CLAY;
        } else if (subsoil_noise < -0.3f) {
            return BLOCK_GRAVEL;
        }
        return BLOCK_DIRT;  // Transitional dirt
    }

    // 5. Solid bedrock foundation (y=0-4)
    if (world_y <= params.bedrock_solid) {
        return BLOCK_BEDROCK;
    }

    // 6. Mixed bedrock layer (y=5-8) with gradient
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

    // 7. Deep stone layer (y=9-32)
    if (world_y <= params.deep_stone_start) {
        // Check for diamond ore (very rare, very deep)
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

        // Check for gold ore (rare)
        if (world_y >= params.gold_min_y && world_y <= params.gold_max_y) {
            float ore_noise = noise_3d(
                (float)world_x * 0.2f + 1000.0f,
                (float)world_y * 0.2f,
                (float)world_z * 0.2f
            );
            if (ore_noise > (1.0f - params.gold_frequency)) {
                return BLOCK_GOLD_ORE;
            }
        }

        return BLOCK_DEEP_STONE;
    }

    // 8. Gravel pockets
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

    // 9. Clay deposits in mid-levels
    if (world_y >= params.clay_min_y && world_y <= params.clay_max_y) {
        float clay_noise = noise_3d(
            (float)world_x * 0.12f + 500.0f,
            (float)world_y * 0.12f,
            (float)world_z * 0.12f
        );
        if (clay_noise > (1.0f - params.clay_frequency)) {
            return BLOCK_CLAY;
        }
    }

    // 10. Standard stone with ores
    // Check for iron ore
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

    // Check for coal ore
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

    // 11. Check for caves (after all solid blocks determined)
    // Pass terrain_height so caves only form deep underground
    if (is_cave(world_x, world_y, world_z, terrain_height, params)) {
        return BLOCK_AIR;
    }

    // Default: standard stone
    return BLOCK_STONE;
}

/**
 * Simple hash function for deterministic dungeon placement
 */
static unsigned int chunk_hash(int chunk_x, int chunk_z) {
    unsigned int h = 2166136261u;
    h ^= (unsigned int)chunk_x;
    h *= 16777619u;
    h ^= (unsigned int)chunk_z;
    h *= 16777619u;
    return h;
}

/**
 * Generate a dungeon room in the chunk
 */
static void generate_dungeon(Chunk* chunk, TerrainParams params) {
    if (!params.generate_dungeons) return;

    // Use chunk coordinates to deterministically decide if this chunk has a dungeon
    unsigned int hash = chunk_hash(chunk->x, chunk->z);
    float dungeon_roll = (float)(hash % 1000) / 1000.0f;

    if (dungeon_roll > params.dungeon_frequency) return;

    // Determine dungeon position and size from hash
    int size_x = params.dungeon_min_size + (hash % (params.dungeon_max_size - params.dungeon_min_size + 1));
    int size_z = params.dungeon_min_size + ((hash >> 8) % (params.dungeon_max_size - params.dungeon_min_size + 1));
    int size_y = 4 + (hash >> 16) % 3;  // Height 4-6

    // Position within chunk (centered-ish)
    int start_x = (CHUNK_SIZE - size_x) / 2;
    int start_z = (CHUNK_SIZE - size_z) / 2;
    int start_y = params.dungeon_min_y + ((hash >> 4) % (params.dungeon_max_y - params.dungeon_min_y));

    // Make sure we're underground
    int world_x = chunk->x * CHUNK_SIZE + start_x + size_x / 2;
    int world_z = chunk->z * CHUNK_SIZE + start_z + size_z / 2;
    int terrain_height = terrain_get_height_at(world_x, world_z, params);

    if (start_y + size_y >= terrain_height - 5) {
        // Too close to surface, move it down
        start_y = terrain_height - size_y - 10;
        if (start_y < params.dungeon_min_y) return;  // Can't fit
    }

    printf("[TERRAIN] Generating dungeon at chunk (%d,%d) y=%d size=%dx%dx%d\n",
           chunk->x, chunk->z, start_y, size_x, size_y, size_z);

    // Generate the dungeon room
    for (int x = 0; x < size_x; x++) {
        for (int z = 0; z < size_z; z++) {
            for (int y = 0; y < size_y; y++) {
                int local_x = start_x + x;
                int local_z = start_z + z;
                int local_y = start_y + y;

                // Skip if outside chunk bounds
                if (local_x < 0 || local_x >= CHUNK_SIZE) continue;
                if (local_z < 0 || local_z >= CHUNK_SIZE) continue;
                if (local_y < 0 || local_y >= CHUNK_HEIGHT) continue;

                BlockType block_type;

                // Determine if this is wall, floor, ceiling, or interior
                bool is_floor = (y == 0);
                bool is_ceiling = (y == size_y - 1);
                bool is_wall = (x == 0 || x == size_x - 1 || z == 0 || z == size_z - 1);

                if (is_floor || is_ceiling || is_wall) {
                    // Structure blocks - mix of stone brick and mossy/cracked variants
                    float damage_noise = noise_3d(
                        (float)(chunk->x * CHUNK_SIZE + local_x) * 0.3f,
                        (float)local_y * 0.3f,
                        (float)(chunk->z * CHUNK_SIZE + local_z) * 0.3f
                    );

                    if (damage_noise > 0.4f) {
                        block_type = BLOCK_MOSSY_COBBLE;
                    } else if (damage_noise < -0.3f) {
                        block_type = BLOCK_CRACKED_BRICK;
                    } else {
                        block_type = BLOCK_STONE_BRICK;
                    }
                } else {
                    // Interior is air
                    block_type = BLOCK_AIR;
                }

                Block block = {block_type, 0, 0};
                chunk_set_block(chunk, local_x, local_y, local_z, block);
            }
        }
    }

    // Add a corridor/entrance on one side (deterministic based on hash)
    int corridor_side = (hash >> 12) % 4;
    int corridor_width = 2;
    int corridor_height = 3;
    int corridor_length = 4;

    int cx, cz, dx, dz;
    switch (corridor_side) {
        case 0: // North
            cx = start_x + size_x / 2 - corridor_width / 2;
            cz = start_z - corridor_length;
            dx = 0; dz = 1;
            break;
        case 1: // South
            cx = start_x + size_x / 2 - corridor_width / 2;
            cz = start_z + size_z;
            dx = 0; dz = -1;
            break;
        case 2: // East
            cx = start_x + size_x;
            cz = start_z + size_z / 2 - corridor_width / 2;
            dx = -1; dz = 0;
            break;
        default: // West
            cx = start_x - corridor_length;
            cz = start_z + size_z / 2 - corridor_width / 2;
            dx = 1; dz = 0;
            break;
    }

    // Carve the corridor
    for (int i = 0; i < corridor_length; i++) {
        for (int w = 0; w < corridor_width; w++) {
            for (int h = 1; h < corridor_height + 1; h++) {
                int local_x = cx + (dz != 0 ? w : i * (dx == 0 ? 1 : dx));
                int local_z = cz + (dx != 0 ? w : i * (dz == 0 ? 1 : dz));
                int local_y = start_y + h;

                if (local_x >= 0 && local_x < CHUNK_SIZE &&
                    local_z >= 0 && local_z < CHUNK_SIZE &&
                    local_y >= 0 && local_y < CHUNK_HEIGHT) {
                    Block block = {BLOCK_AIR, 0, 0};
                    chunk_set_block(chunk, local_x, local_y, local_z, block);
                }
            }
        }
    }
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

    // Generate dungeons underground
    generate_dungeon(chunk, params);

    // Generate trees on the terrain
    tree_generate_for_chunk(chunk);

    // Calculate skylight propagation (must be after all blocks are placed)
    light_calculate_chunk(chunk);

    // Mark chunk as needing mesh regeneration
    chunk->needs_remesh = true;
}
