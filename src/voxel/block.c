/**
 * Block System Implementation
 */

#include "voxel/block.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// BLOCK PROPERTIES TABLE
// ============================================================================

static BlockProperties g_block_properties[BLOCK_COUNT];
static bool g_initialized = false;

/**
 * Initialize block properties
 */
void block_system_init(void) {
    if (g_initialized) return;

    // Clear all properties
    memset(g_block_properties, 0, sizeof(g_block_properties));

    // AIR
    g_block_properties[BLOCK_AIR] = (BlockProperties){
        .name = "Air",
        .is_solid = false,
        .is_transparent = true,
        .is_fluid = false
    };

    // GRASS
    g_block_properties[BLOCK_GRASS] = (BlockProperties){
        .name = "Grass",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false
    };

    // DIRT
    g_block_properties[BLOCK_DIRT] = (BlockProperties){
        .name = "Dirt",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false
    };

    // STONE
    g_block_properties[BLOCK_STONE] = (BlockProperties){
        .name = "Stone",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false
    };

    // WOOD
    g_block_properties[BLOCK_WOOD] = (BlockProperties){
        .name = "Wood",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false
    };

    // LEAVES
    g_block_properties[BLOCK_LEAVES] = (BlockProperties){
        .name = "Leaves",
        .is_solid = true,
        .is_transparent = true,  // Can see through
        .is_fluid = false
    };

    // SAND
    g_block_properties[BLOCK_SAND] = (BlockProperties){
        .name = "Sand",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false
    };

    // WATER
    g_block_properties[BLOCK_WATER] = (BlockProperties){
        .name = "Water",
        .is_solid = false,
        .is_transparent = true,
        .is_fluid = true
    };

    // COBBLESTONE
    g_block_properties[BLOCK_COBBLESTONE] = (BlockProperties){
        .name = "Cobblestone",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false
    };

    // BEDROCK
    g_block_properties[BLOCK_BEDROCK] = (BlockProperties){
        .name = "Bedrock",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false
    };

    g_initialized = true;
    printf("[BLOCK] Block system initialized with %d block types\n", BLOCK_COUNT);
}

/**
 * Get properties for a block type
 */
const BlockProperties* block_get_properties(BlockType type) {
    if (!g_initialized) {
        block_system_init();
    }

    if (type >= 0 && type < BLOCK_COUNT) {
        return &g_block_properties[type];
    }

    return &g_block_properties[BLOCK_AIR];
}

/**
 * Check if block is solid
 */
bool block_is_solid(Block block) {
    const BlockProperties* props = block_get_properties(block.type);
    return props->is_solid;
}

/**
 * Check if block is transparent
 */
bool block_is_transparent(Block block) {
    const BlockProperties* props = block_get_properties(block.type);
    return props->is_transparent;
}

/**
 * Check if block is fluid
 */
bool block_is_fluid(Block block) {
    const BlockProperties* props = block_get_properties(block.type);
    return props->is_fluid;
}

/**
 * Get block name
 */
const char* block_get_name(BlockType type) {
    const BlockProperties* props = block_get_properties(type);
    return props->name;
}
