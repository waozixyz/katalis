/**
 * Block System Implementation
 */

#include "voxel/core/block.h"
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
        .is_fluid = false,
        .hardness = 0.0f,
        .preferred_tool = TOOL_NONE,
        .requires_tool = false
    };

    // GRASS
    g_block_properties[BLOCK_GRASS] = (BlockProperties){
        .name = "Grass",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 0.5f,
        .preferred_tool = TOOL_SHOVEL,
        .requires_tool = false
    };

    // DIRT
    g_block_properties[BLOCK_DIRT] = (BlockProperties){
        .name = "Dirt",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 0.5f,
        .preferred_tool = TOOL_SHOVEL,
        .requires_tool = false
    };

    // STONE
    g_block_properties[BLOCK_STONE] = (BlockProperties){
        .name = "Stone",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 1.5f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // WOOD
    g_block_properties[BLOCK_WOOD] = (BlockProperties){
        .name = "Wood",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 2.0f,
        .preferred_tool = TOOL_AXE,
        .requires_tool = false
    };

    // LEAVES
    g_block_properties[BLOCK_LEAVES] = (BlockProperties){
        .name = "Leaves",
        .is_solid = true,
        .is_transparent = true,
        .is_fluid = false,
        .hardness = 0.2f,
        .preferred_tool = TOOL_NONE,
        .requires_tool = false
    };

    // SAND
    g_block_properties[BLOCK_SAND] = (BlockProperties){
        .name = "Sand",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 0.5f,
        .preferred_tool = TOOL_SHOVEL,
        .requires_tool = false
    };

    // WATER
    g_block_properties[BLOCK_WATER] = (BlockProperties){
        .name = "Water",
        .is_solid = false,
        .is_transparent = true,
        .is_fluid = true,
        .hardness = 0.0f,
        .preferred_tool = TOOL_NONE,
        .requires_tool = false
    };

    // COBBLESTONE
    g_block_properties[BLOCK_COBBLESTONE] = (BlockProperties){
        .name = "Cobblestone",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 2.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // BEDROCK
    g_block_properties[BLOCK_BEDROCK] = (BlockProperties){
        .name = "Bedrock",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = -1.0f,
        .preferred_tool = TOOL_NONE,
        .requires_tool = false
    };

    // DEEP_STONE
    g_block_properties[BLOCK_DEEP_STONE] = (BlockProperties){
        .name = "Deep Stone",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 2.5f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // GRAVEL
    g_block_properties[BLOCK_GRAVEL] = (BlockProperties){
        .name = "Gravel",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 0.6f,
        .preferred_tool = TOOL_SHOVEL,
        .requires_tool = false
    };

    // COAL_ORE
    g_block_properties[BLOCK_COAL_ORE] = (BlockProperties){
        .name = "Coal Ore",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 3.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // IRON_ORE
    g_block_properties[BLOCK_IRON_ORE] = (BlockProperties){
        .name = "Iron Ore",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 3.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // GOLD_ORE
    g_block_properties[BLOCK_GOLD_ORE] = (BlockProperties){
        .name = "Gold Ore",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 3.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // DIAMOND_ORE
    g_block_properties[BLOCK_DIAMOND_ORE] = (BlockProperties){
        .name = "Diamond Ore",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 5.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // MOSSY_COBBLE (Dungeon)
    g_block_properties[BLOCK_MOSSY_COBBLE] = (BlockProperties){
        .name = "Mossy Cobblestone",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 2.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // STONE_BRICK (Dungeon)
    g_block_properties[BLOCK_STONE_BRICK] = (BlockProperties){
        .name = "Stone Brick",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 2.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // CRACKED_BRICK (Dungeon)
    g_block_properties[BLOCK_CRACKED_BRICK] = (BlockProperties){
        .name = "Cracked Stone Brick",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 1.5f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
    };

    // CLAY
    g_block_properties[BLOCK_CLAY] = (BlockProperties){
        .name = "Clay",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 0.6f,
        .preferred_tool = TOOL_SHOVEL,
        .requires_tool = false
    };

    // SNOW (Tundra biome surface)
    g_block_properties[BLOCK_SNOW] = (BlockProperties){
        .name = "Snow",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 0.2f,
        .preferred_tool = TOOL_SHOVEL,
        .requires_tool = false
    };

    // CACTUS (Desert vegetation)
    g_block_properties[BLOCK_CACTUS] = (BlockProperties){
        .name = "Cactus",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 0.4f,
        .preferred_tool = TOOL_NONE,
        .requires_tool = false
    };

    // BIRCH_WOOD
    g_block_properties[BLOCK_BIRCH_WOOD] = (BlockProperties){
        .name = "Birch Wood",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 2.0f,
        .preferred_tool = TOOL_AXE,
        .requires_tool = false
    };

    // BIRCH_LEAVES
    g_block_properties[BLOCK_BIRCH_LEAVES] = (BlockProperties){
        .name = "Birch Leaves",
        .is_solid = true,
        .is_transparent = true,
        .is_fluid = false,
        .hardness = 0.2f,
        .preferred_tool = TOOL_NONE,
        .requires_tool = false
    };

    // SPRUCE_WOOD
    g_block_properties[BLOCK_SPRUCE_WOOD] = (BlockProperties){
        .name = "Spruce Wood",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 2.0f,
        .preferred_tool = TOOL_AXE,
        .requires_tool = false
    };

    // SPRUCE_LEAVES
    g_block_properties[BLOCK_SPRUCE_LEAVES] = (BlockProperties){
        .name = "Spruce Leaves",
        .is_solid = true,
        .is_transparent = true,
        .is_fluid = false,
        .hardness = 0.2f,
        .preferred_tool = TOOL_NONE,
        .requires_tool = false
    };

    // ACACIA_WOOD
    g_block_properties[BLOCK_ACACIA_WOOD] = (BlockProperties){
        .name = "Acacia Wood",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 2.0f,
        .preferred_tool = TOOL_AXE,
        .requires_tool = false
    };

    // ACACIA_LEAVES
    g_block_properties[BLOCK_ACACIA_LEAVES] = (BlockProperties){
        .name = "Acacia Leaves",
        .is_solid = true,
        .is_transparent = true,
        .is_fluid = false,
        .hardness = 0.2f,
        .preferred_tool = TOOL_NONE,
        .requires_tool = false
    };

    // STALACTITE (Cave decoration - hanging from ceiling)
    g_block_properties[BLOCK_STALACTITE] = (BlockProperties){
        .name = "Stalactite",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 1.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = false
    };

    // STALAGMITE (Cave decoration - rising from floor)
    g_block_properties[BLOCK_STALAGMITE] = (BlockProperties){
        .name = "Stalagmite",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 1.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = false
    };

    // CHEST (Interactive loot container)
    g_block_properties[BLOCK_CHEST] = (BlockProperties){
        .name = "Chest",
        .is_solid = true,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 2.5f,
        .preferred_tool = TOOL_AXE,
        .requires_tool = false
    };

    // BED_HEAD (Head of bed - where player sleeps)
    g_block_properties[BLOCK_BED_HEAD] = (BlockProperties){
        .name = "Bed Head",
        .is_solid = false,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 0.2f,
        .preferred_tool = TOOL_AXE,
        .requires_tool = false
    };

    // BED_FOOT (Foot of bed - pillow end)
    g_block_properties[BLOCK_BED_FOOT] = (BlockProperties){
        .name = "Bed Foot",
        .is_solid = false,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 0.2f,
        .preferred_tool = TOOL_AXE,
        .requires_tool = false
    };

    // WOOD_DOOR (Wooden door - can be opened by hand)
    g_block_properties[BLOCK_WOOD_DOOR] = (BlockProperties){
        .name = "Wood Door",
        .is_solid = false,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 3.0f,
        .preferred_tool = TOOL_AXE,
        .requires_tool = false
    };

    // IRON_DOOR (Iron door - needs interaction)
    g_block_properties[BLOCK_IRON_DOOR] = (BlockProperties){
        .name = "Iron Door",
        .is_solid = false,
        .is_transparent = false,
        .is_fluid = false,
        .hardness = 5.0f,
        .preferred_tool = TOOL_PICKAXE,
        .requires_tool = true
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
