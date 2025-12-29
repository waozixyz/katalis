/**
 * Biome System
 *
 * Defines biome types and properties for varied terrain generation.
 * Biomes control surface blocks, terrain height, and vegetation density.
 */

#ifndef VOXEL_BIOME_H
#define VOXEL_BIOME_H

#include "voxel/core/block.h"

// ============================================================================
// BIOME TYPES
// ============================================================================

typedef enum {
    BIOME_PLAINS,      // Default grassland with moderate trees
    BIOME_FOREST,      // Dense trees, hillier terrain
    BIOME_DESERT,      // Sand surface, flat terrain, cacti
    BIOME_TUNDRA,      // Snow surface, flat terrain, sparse trees
    BIOME_COUNT
} BiomeType;

// ============================================================================
// BIOME PROPERTIES
// ============================================================================

typedef struct {
    const char* name;
    BlockType surface_block;      // Top surface block (GRASS, SAND, SNOW)
    BlockType subsurface_block;   // Block below surface (DIRT, SAND, etc.)
    float height_scale;           // Multiplier for terrain height variation
    float tree_density;           // 0.0-1.0, chance of tree placement
    bool has_trees;               // Whether trees generate in this biome
    bool has_cacti;               // Whether cacti generate (desert only)
} BiomeProperties;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize biome system
 */
void biome_init(void);

/**
 * Get the biome type at a world position
 * Uses 2D noise for large-scale biome regions
 *
 * @param world_x World X coordinate
 * @param world_z World Z coordinate
 * @return BiomeType at that position
 */
BiomeType biome_get_at(int world_x, int world_z);

/**
 * Get properties for a biome type
 *
 * @param type The biome type
 * @return Pointer to BiomeProperties (static, do not free)
 */
const BiomeProperties* biome_get_properties(BiomeType type);

/**
 * Get biome name for display/debug
 *
 * @param type The biome type
 * @return Biome name string
 */
const char* biome_get_name(BiomeType type);

#endif // VOXEL_BIOME_H
