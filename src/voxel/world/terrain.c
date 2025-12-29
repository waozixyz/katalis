/**
 * Terrain Generator Implementation
 */

#include "voxel/world/terrain.h"
#include "voxel/world/noise.h"
#include "voxel/core/block.h"
#include "voxel/world/biome.h"
#include "voxel/entity/tree.h"
#include "voxel/render/light.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// DEFAULT PARAMETERS
// ============================================================================

TerrainParams terrain_default_params(void) {
    TerrainParams params;

    // Heightmap - Surface at y=160 for underground exploration
    params.height_scale = 24.0f;          // Moderate hills
    params.height_offset = 160.0f;        // Surface at y=160
    params.height_octaves = 5;            // Detail
    params.height_frequency = 0.006f;     // Broad rolling hills
    params.height_lacunarity = 2.0f;
    params.height_persistence = 0.45f;

    // Noise caves
    params.generate_caves = true;
    params.cave_threshold = 0.48f;        // Cave density
    params.cave_frequency = 0.035f;       // Cave size
    params.cave_octaves = 3;              // Organic shapes
    params.cave_min_depth = 25;           // Caves start 25 blocks below surface

    // Biomes
    params.generate_biomes = true;
    params.biome_frequency = 0.005f;

    // Layers
    params.dirt_depth = 8;           // 8 blocks of dirt
    params.subsoil_depth = 6;        // 6 blocks of mixed clay/gravel
    params.stone_depth = 100;        // Stone depth

    // Deep layer configuration
    params.deep_stone_start = 64;    // Deep stone below y=64
    params.bedrock_start = 8;        // Bedrock mixing at y=8
    params.bedrock_solid = 4;        // Solid bedrock at y=4

    // Ore generation - Coal (upper stone layer)
    params.coal_frequency = 0.10f;
    params.coal_min_y = 100;
    params.coal_max_y = 180;

    // Ore generation - Iron (mid-depths)
    params.iron_frequency = 0.07f;
    params.iron_min_y = 40;
    params.iron_max_y = 120;

    // Ore generation - Gold (deep)
    params.gold_frequency = 0.035f;
    params.gold_min_y = 16;
    params.gold_max_y = 50;

    // Ore generation - Diamond (very rare, near bedrock)
    params.diamond_frequency = 0.015f;
    params.diamond_min_y = 4;
    params.diamond_max_y = 20;

    // Gravel pockets (throughout stone)
    params.gravel_frequency = 0.10f;
    params.gravel_min_y = 40;
    params.gravel_max_y = 150;

    // Clay deposits (in subsoil layer near surface)
    params.clay_frequency = 0.08f;
    params.clay_min_y = 160;
    params.clay_max_y = 190;

    // Dungeons (spread through underground)
    params.generate_dungeons = true;
    params.dungeon_frequency = 0.05f;  // 5% chance per chunk
    params.dungeon_min_y = 20;
    params.dungeon_max_y = 150;
    params.dungeon_min_size = 5;
    params.dungeon_max_size = 9;

    // Cave tunnels (worm caves) - main cave system
    params.generate_cave_tunnels = true;
    params.tunnel_radius_min = 2.0f;
    params.tunnel_radius_max = 4.0f;
    params.tunnel_segments = 40;
    params.tunnels_per_chunk = 2;

    // Cave rooms - large open underground areas
    params.generate_cave_rooms = true;
    params.rooms_per_chunk = 1;
    params.room_radius_min = 4.0f;
    params.room_radius_max = 7.0f;

    return params;
}

// ============================================================================
// TERRAIN GENERATION
// ============================================================================

/**
 * Get terrain height at world coordinates
 * Applies biome-specific height scaling for varied terrain
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

    // Apply biome height scaling
    BiomeType biome = biome_get_at(world_x, world_z);
    const BiomeProperties* bp = biome_get_properties(biome);
    float biome_scale = bp->height_scale;

    // Convert noise (-1 to 1) to height with biome scaling
    float height = params.height_offset + (noise_value * params.height_scale * biome_scale);
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

    // Limit noise caves to upper 150 blocks below surface
    if (depth_below_surface > 150) return false;

    float cave_noise = noise_fbm_3d(
        (float)world_x, (float)world_y, (float)world_z,
        params.cave_octaves,
        params.cave_frequency,
        1.0f,
        2.0f,
        0.5f
    );

    // Caves get more likely deeper down (gradual increase)
    float depth_factor = (float)depth_below_surface / 100.0f;
    if (depth_factor > 1.0f) depth_factor = 1.0f;
    float adjusted_threshold = params.cave_threshold + (1.0f - depth_factor) * 0.15f;

    return cave_noise > adjusted_threshold;
}

/**
 * Get block type at world coordinates
 * Uses biome-specific surface and subsurface blocks
 */
static BlockType get_terrain_block(int world_x, int world_y, int world_z, int terrain_height, TerrainParams params, BiomeType biome) {
    const BiomeProperties* bp = biome_get_properties(biome);

    // 1. Air above terrain
    if (world_y > terrain_height) {
        return BLOCK_AIR;
    }

    // 2. Surface block (biome-specific: grass, sand, snow)
    if (world_y == terrain_height) {
        return bp->surface_block;
    }

    // 3. Subsurface (biome-specific: dirt, sand)
    if (world_y > terrain_height - params.dirt_depth) {
        return bp->subsurface_block;
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

// ============================================================================
// CAVE TUNNEL (WORM) GENERATION
// ============================================================================

/**
 * Simple deterministic random from seed
 */
static float random_from_seed(unsigned int* seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return (float)*seed / (float)0x7FFFFFFF;
}

/**
 * Carve a sphere of air at the given position
 */
static void carve_sphere(Chunk* chunk, float cx, float cy, float cz, float radius, int terrain_height, TerrainParams params) {
    int min_x = (int)(cx - radius) - 1;
    int max_x = (int)(cx + radius) + 1;
    int min_y = (int)(cy - radius) - 1;
    int max_y = (int)(cy + radius) + 1;
    int min_z = (int)(cz - radius) - 1;
    int max_z = (int)(cz + radius) + 1;

    for (int x = min_x; x <= max_x; x++) {
        if (x < 0 || x >= CHUNK_SIZE) continue;
        for (int z = min_z; z <= max_z; z++) {
            if (z < 0 || z >= CHUNK_SIZE) continue;
            for (int y = min_y; y <= max_y; y++) {
                if (y < params.bedrock_start || y >= CHUNK_HEIGHT) continue;
                // Don't carve near surface
                if (y > terrain_height - params.cave_min_depth) continue;

                float dx = (float)x - cx;
                float dy = (float)y - cy;
                float dz = (float)z - cz;
                float dist = sqrtf(dx*dx + dy*dy + dz*dz);

                if (dist <= radius) {
                    Block block = {BLOCK_AIR, 0, 0};
                    chunk_set_block(chunk, x, y, z, block);
                }
            }
        }
    }
}

/**
 * Generate worm-style cave tunnels
 * Creates connected, explorable cave networks
 */
static void generate_cave_tunnels(Chunk* chunk, TerrainParams params, int terrain_height) {
    if (!params.generate_cave_tunnels) return;

    unsigned int seed = chunk_hash(chunk->x * 7, chunk->z * 13);

    // Number of tunnels varies per chunk
    int num_tunnels = params.tunnels_per_chunk + (seed % 3) - 1;
    if (num_tunnels < 1) num_tunnels = 1;

    for (int t = 0; t < num_tunnels; t++) {
        // Random starting position within chunk
        float x = random_from_seed(&seed) * CHUNK_SIZE;
        float z = random_from_seed(&seed) * CHUNK_SIZE;

        // Y range: caves in upper 150 blocks below surface
        int min_cave_y = terrain_height - 150;
        if (min_cave_y < 20) min_cave_y = 20;
        int max_cave_y = terrain_height - params.cave_min_depth - 10;
        if (max_cave_y < min_cave_y + 20) max_cave_y = min_cave_y + 20;
        float y = min_cave_y + random_from_seed(&seed) * (max_cave_y - min_cave_y);

        // Random direction (mostly horizontal)
        float dir_x = random_from_seed(&seed) * 2.0f - 1.0f;
        float dir_y = random_from_seed(&seed) * 0.4f - 0.2f;  // Slight vertical
        float dir_z = random_from_seed(&seed) * 2.0f - 1.0f;

        // Normalize direction
        float len = sqrtf(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z);
        if (len > 0.01f) {
            dir_x /= len;
            dir_y /= len;
            dir_z /= len;
        }

        // Starting radius
        float radius = params.tunnel_radius_min +
                       random_from_seed(&seed) * (params.tunnel_radius_max - params.tunnel_radius_min);

        // Carve the tunnel segment by segment
        for (int seg = 0; seg < params.tunnel_segments; seg++) {
            // Only carve if within chunk bounds
            if (x >= -radius && x < CHUNK_SIZE + radius &&
                z >= -radius && z < CHUNK_SIZE + radius) {
                carve_sphere(chunk, x, y, z, radius, terrain_height, params);
            }

            // Move along direction with noise wobble
            float wobble_x = noise_3d((float)seg * 0.1f, 0, 0) * 0.5f;
            float wobble_y = noise_3d((float)seg * 0.1f + 100.0f, 0, 0) * 0.3f;
            float wobble_z = noise_3d((float)seg * 0.1f + 200.0f, 0, 0) * 0.5f;

            x += dir_x + wobble_x;
            y += dir_y + wobble_y;
            z += dir_z + wobble_z;

            // Occasionally change direction
            if (seg % 12 == 0) {
                dir_x += random_from_seed(&seed) * 0.5f - 0.25f;
                dir_y += random_from_seed(&seed) * 0.2f - 0.1f;
                dir_z += random_from_seed(&seed) * 0.5f - 0.25f;

                // Renormalize
                len = sqrtf(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z);
                if (len > 0.01f) {
                    dir_x /= len;
                    dir_y /= len;
                    dir_z /= len;
                }
            }

            // Vary radius slightly
            radius += random_from_seed(&seed) * 0.4f - 0.2f;
            if (radius < params.tunnel_radius_min) radius = params.tunnel_radius_min;
            if (radius > params.tunnel_radius_max) radius = params.tunnel_radius_max;

            // Keep Y within valid range
            if (y < params.bedrock_start + 20) {
                dir_y = fabsf(dir_y);  // Go up
            }
            if (y > terrain_height - params.cave_min_depth - 30) {
                dir_y = -fabsf(dir_y);  // Go down
            }
        }
    }
}

// ============================================================================
// CAVE ROOM GENERATION
// ============================================================================

/**
 * Carve an ellipsoid-shaped room
 */
static void carve_ellipsoid(Chunk* chunk, float cx, float cy, float cz,
                            float rx, float ry, float rz,
                            int terrain_height, TerrainParams params) {
    int min_x = (int)(cx - rx) - 1;
    int max_x = (int)(cx + rx) + 1;
    int min_y = (int)(cy - ry) - 1;
    int max_y = (int)(cy + ry) + 1;
    int min_z = (int)(cz - rz) - 1;
    int max_z = (int)(cz + rz) + 1;

    for (int x = min_x; x <= max_x; x++) {
        if (x < 0 || x >= CHUNK_SIZE) continue;
        for (int z = min_z; z <= max_z; z++) {
            if (z < 0 || z >= CHUNK_SIZE) continue;
            for (int y = min_y; y <= max_y; y++) {
                if (y < params.bedrock_start || y >= CHUNK_HEIGHT) continue;
                if (y > terrain_height - params.cave_min_depth) continue;

                float dx = ((float)x - cx) / rx;
                float dy = ((float)y - cy) / ry;
                float dz = ((float)z - cz) / rz;
                float dist = dx*dx + dy*dy + dz*dz;

                if (dist <= 1.0f) {
                    Block block = {BLOCK_AIR, 0, 0};
                    chunk_set_block(chunk, x, y, z, block);
                }
            }
        }
    }
}

/**
 * Generate large cave rooms underground
 */
static void generate_cave_rooms(Chunk* chunk, TerrainParams params, int terrain_height) {
    if (!params.generate_cave_rooms) return;

    unsigned int seed = chunk_hash(chunk->x * 31, chunk->z * 17);

    // Number of rooms varies
    int num_rooms = 1 + (seed % (params.rooms_per_chunk + 1));

    for (int r = 0; r < num_rooms; r++) {
        // Random position within chunk
        float x = random_from_seed(&seed) * CHUNK_SIZE;
        float z = random_from_seed(&seed) * CHUNK_SIZE;

        // Y range: rooms in upper 120 blocks below surface
        int min_room_y = terrain_height - 120;
        if (min_room_y < 30) min_room_y = 30;
        int max_room_y = terrain_height - params.cave_min_depth - 20;
        if (max_room_y < min_room_y + 20) max_room_y = min_room_y + 20;
        float y = min_room_y + random_from_seed(&seed) * (max_room_y - min_room_y);

        // Random ellipsoid dimensions
        float rx = params.room_radius_min +
                   random_from_seed(&seed) * (params.room_radius_max - params.room_radius_min);
        float ry = (params.room_radius_min * 0.6f) +
                   random_from_seed(&seed) * ((params.room_radius_max * 0.6f) - (params.room_radius_min * 0.6f));
        float rz = params.room_radius_min +
                   random_from_seed(&seed) * (params.room_radius_max - params.room_radius_min);

        carve_ellipsoid(chunk, x, y, z, rx, ry, rz, terrain_height, params);
    }
}

/**
 * Generate terrain for chunk
 */
void terrain_generate_chunk(Chunk* chunk, TerrainParams params) {
    if (!chunk) return;

    // Calculate average terrain height for this chunk (for cave bounds)
    int center_x = chunk->x * CHUNK_SIZE + CHUNK_SIZE / 2;
    int center_z = chunk->z * CHUNK_SIZE + CHUNK_SIZE / 2;
    int avg_terrain_height = terrain_get_height_at(center_x, center_z, params);

    // For each column in the chunk
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            // Calculate world coordinates
            int world_x = chunk->x * CHUNK_SIZE + x;
            int world_z = chunk->z * CHUNK_SIZE + z;

            // Get biome and terrain height at this column
            BiomeType biome = biome_get_at(world_x, world_z);
            int terrain_height = terrain_get_height_at(world_x, world_z, params);

            // Fill vertical column
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                int world_y = y;

                // Determine block type (biome-aware)
                BlockType block_type = get_terrain_block(world_x, world_y, world_z, terrain_height, params, biome);

                // Set block
                Block block = {block_type, 0, 0};
                chunk_set_block(chunk, x, y, z, block);
            }
        }
    }

    // Generate cave tunnels (worm caves) - main cave system
    generate_cave_tunnels(chunk, params, avg_terrain_height);

    // Generate large cave rooms
    generate_cave_rooms(chunk, params, avg_terrain_height);

    // Generate dungeons underground
    generate_dungeon(chunk, params);

    // Generate trees on the terrain
    tree_generate_for_chunk(chunk);

    // Calculate skylight propagation (must be after all blocks are placed)
    light_calculate_chunk(chunk);

    // Mark chunk as needing mesh regeneration
    chunk->needs_remesh = true;
}
