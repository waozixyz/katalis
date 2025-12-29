/**
 * Block System - Voxel Engine
 *
 * Defines block types, properties, and data structures
 */

#ifndef VOXEL_BLOCK_H
#define VOXEL_BLOCK_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// TOOL TYPES (for mining)
// ============================================================================

typedef enum {
    TOOL_NONE = 0,    // Hand/any item
    TOOL_PICKAXE,     // Stone, ores, cobblestone
    TOOL_AXE,         // Wood, planks
    TOOL_SHOVEL,      // Dirt, sand, gravel
} ToolType;

// ============================================================================
// BLOCK TYPES
// ============================================================================

typedef enum {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_WOOD,
    BLOCK_LEAVES,
    BLOCK_SAND,
    BLOCK_WATER,
    BLOCK_COBBLESTONE,
    BLOCK_BEDROCK,
    BLOCK_DEEP_STONE,       // Darker stone for deep layers
    BLOCK_GRAVEL,           // Loose sediment
    BLOCK_COAL_ORE,         // Common ore
    BLOCK_IRON_ORE,         // Uncommon ore
    BLOCK_GOLD_ORE,         // Rare ore
    BLOCK_DIAMOND_ORE,      // Very rare ore
    BLOCK_MOSSY_COBBLE,     // Dungeon walls
    BLOCK_STONE_BRICK,      // Dungeon structure
    BLOCK_CRACKED_BRICK,    // Damaged dungeon walls
    BLOCK_CLAY,             // Underground clay deposits
    BLOCK_SNOW,             // Tundra surface block
    BLOCK_CACTUS,           // Desert vegetation
    BLOCK_COUNT  // Total number of block types
} BlockType;

// ============================================================================
// BLOCK DATA
// ============================================================================

typedef struct {
    uint8_t type;           // BlockType
    uint8_t light_level;    // 0-15 for lighting (future use)
    uint8_t metadata;       // Extra data (rotation, etc.)
} Block;

// ============================================================================
// BLOCK PROPERTIES
// ============================================================================

typedef struct {
    const char* name;
    bool is_solid;          // Can be walked through?
    bool is_transparent;    // Light passes through?
    bool is_fluid;          // Water-like behavior?
    float texture_coords[6][4];  // UV coords for each face (top, bottom, north, south, east, west)
    // Mining properties
    float hardness;           // Base dig time in seconds (0 = instant, -1 = unbreakable)
    ToolType preferred_tool;  // Which tool speeds this up
    bool requires_tool;       // Must use correct tool to drop item
} BlockProperties;

// ============================================================================
// API
// ============================================================================

/**
 * Initialize block system (sets up properties table)
 */
void block_system_init(void);

/**
 * Get properties for a block type
 */
const BlockProperties* block_get_properties(BlockType type);

/**
 * Check if block is solid (blocks movement)
 */
bool block_is_solid(Block block);

/**
 * Check if block is transparent (light passes through)
 */
bool block_is_transparent(Block block);

/**
 * Check if block is fluid (water-like)
 */
bool block_is_fluid(Block block);

/**
 * Get block name for debugging
 */
const char* block_get_name(BlockType type);

#endif // VOXEL_BLOCK_H
