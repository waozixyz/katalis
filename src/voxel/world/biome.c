/**
 * Biome System Implementation
 *
 * Uses 2D noise to determine biome regions across the world.
 */

#include "voxel/world/biome.h"
#include "voxel/world/noise.h"
#include <stdio.h>

// ============================================================================
// BIOME CONFIGURATION
// ============================================================================

// Noise parameters for biome selection
#define BIOME_FREQUENCY 0.003f    // Low frequency = large biome regions
#define BIOME_OCTAVES 2           // Few octaves for smooth regions
#define BIOME_OFFSET 10000.0f     // Offset from terrain noise

// ============================================================================
// BIOME PROPERTIES TABLE
// ============================================================================

static const BiomeProperties g_biome_properties[BIOME_COUNT] = {
    [BIOME_PLAINS] = {
        .name = "Plains",
        .surface_block = BLOCK_GRASS,
        .subsurface_block = BLOCK_DIRT,
        .height_scale = 1.0f,
        .tree_density = 0.5f,
        .has_trees = true,
        .has_cacti = false,
        .primary_tree = TREE_TYPE_OAK,
        .secondary_tree = TREE_TYPE_COUNT,  // None
        .secondary_chance = 0.0f,
    },
    [BIOME_FOREST] = {
        .name = "Forest",
        .surface_block = BLOCK_GRASS,
        .subsurface_block = BLOCK_DIRT,
        .height_scale = 1.3f,       // Hillier terrain
        .tree_density = 0.8f,       // Dense trees
        .has_trees = true,
        .has_cacti = false,
        .primary_tree = TREE_TYPE_OAK,
        .secondary_tree = TREE_TYPE_BIRCH,
        .secondary_chance = 0.4f,   // 40% birch trees
    },
    [BIOME_DESERT] = {
        .name = "Desert",
        .surface_block = BLOCK_SAND,
        .subsurface_block = BLOCK_SAND,
        .height_scale = 0.6f,       // Flatter terrain
        .tree_density = 0.15f,      // Sparse acacia
        .has_trees = true,          // Enable sparse acacia trees
        .has_cacti = true,
        .primary_tree = TREE_TYPE_ACACIA,
        .secondary_tree = TREE_TYPE_COUNT,
        .secondary_chance = 0.0f,
    },
    [BIOME_TUNDRA] = {
        .name = "Tundra",
        .surface_block = BLOCK_SNOW,
        .subsurface_block = BLOCK_DIRT,
        .height_scale = 0.8f,       // Mostly flat
        .tree_density = 0.2f,       // Sparse trees
        .has_trees = true,
        .has_cacti = false,
        .primary_tree = TREE_TYPE_SPRUCE,
        .secondary_tree = TREE_TYPE_COUNT,
        .secondary_chance = 0.0f,
    },
};

// ============================================================================
// PUBLIC API
// ============================================================================

void biome_init(void) {
    printf("[BIOME] Biome system initialized with %d biome types\n", BIOME_COUNT);
}

BiomeType biome_get_at(int world_x, int world_z) {
    // Use 2D fBm noise for biome selection
    // Offset ensures independence from terrain height noise
    float noise = noise_fbm_2d(
        (float)world_x + BIOME_OFFSET,
        (float)world_z + BIOME_OFFSET,
        BIOME_OCTAVES,
        BIOME_FREQUENCY,
        1.0f,       // amplitude
        2.0f,       // lacunarity
        0.5f        // persistence
    );

    // Map noise value (-1 to 1) to biome types
    // Distribution roughly: 20% desert, 30% plains, 30% forest, 20% tundra
    if (noise < -0.3f) {
        return BIOME_DESERT;
    } else if (noise < 0.1f) {
        return BIOME_PLAINS;
    } else if (noise < 0.5f) {
        return BIOME_FOREST;
    } else {
        return BIOME_TUNDRA;
    }
}

const BiomeProperties* biome_get_properties(BiomeType type) {
    if (type < 0 || type >= BIOME_COUNT) {
        return &g_biome_properties[BIOME_PLAINS];  // Default to plains
    }
    return &g_biome_properties[type];
}

const char* biome_get_name(BiomeType type) {
    const BiomeProperties* props = biome_get_properties(type);
    return props->name;
}
