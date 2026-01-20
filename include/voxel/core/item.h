/**
 * Item System
 *
 * Defines item types, properties, and drop tables for the inventory system.
 */

#ifndef VOXEL_ITEM_H
#define VOXEL_ITEM_H

#include <stdint.h>
#include <stdbool.h>
#include "voxel/core/block.h"

// ============================================================================
// ITEM TYPES
// ============================================================================

typedef enum {
    ITEM_NONE = 0,

    // Block items (placeable)
    ITEM_DIRT,
    ITEM_GRASS_BLOCK,
    ITEM_STONE,
    ITEM_COBBLESTONE,
    ITEM_SAND,
    ITEM_WOOD_LOG,
    ITEM_WOOD_PLANKS,
    ITEM_LEAVES,
    ITEM_BEDROCK,
    ITEM_IRON_BLOCK,        // Placeholder for iron ingot
    ITEM_DIAMOND_BLOCK,     // Placeholder for diamond

    // Crafting materials
    ITEM_STICK,

    // Wool (16 colors for bed crafting)
    ITEM_WOOL_WHITE,
    ITEM_WOOL_LIGHT_GRAY,
    ITEM_WOOL_GRAY,
    ITEM_WOOL_BLACK,
    ITEM_WOOL_BROWN,
    ITEM_WOOL_RED,
    ITEM_WOOL_ORANGE,
    ITEM_WOOL_YELLOW,
    ITEM_WOOL_LIME,
    ITEM_WOOL_GREEN,
    ITEM_WOOL_CYAN,
    ITEM_WOOL_LIGHT_BLUE,
    ITEM_WOOL_BLUE,
    ITEM_WOOL_PURPLE,
    ITEM_WOOL_MAGENTA,
    ITEM_WOOL_PINK,

    // Tools
    ITEM_WOODEN_PICKAXE,
    ITEM_STONE_PICKAXE,
    ITEM_WOODEN_SHOVEL,
    ITEM_STONE_SHOVEL,
    ITEM_WOODEN_AXE,
    ITEM_STONE_AXE,

    // Swords
    ITEM_WOODEN_SWORD,
    ITEM_STONE_SWORD,
    ITEM_IRON_SWORD,
    ITEM_DIAMOND_SWORD,

    // Food
    ITEM_MEAT,

    // Beds (16 wool colors)
    ITEM_WHITE_BED,
    ITEM_LIGHT_GRAY_BED,
    ITEM_GRAY_BED,
    ITEM_BLACK_BED,
    ITEM_BROWN_BED,
    ITEM_RED_BED,
    ITEM_ORANGE_BED,
    ITEM_YELLOW_BED,
    ITEM_LIME_BED,
    ITEM_GREEN_BED,
    ITEM_CYAN_BED,
    ITEM_LIGHT_BLUE_BED,
    ITEM_BLUE_BED,
    ITEM_PURPLE_BED,
    ITEM_MAGENTA_BED,
    ITEM_PINK_BED,

    // Doors
    ITEM_WOOD_DOOR,
    ITEM_IRON_DOOR,

    ITEM_COUNT
} ItemType;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Item stack - represents a quantity of items in a slot
 */
typedef struct {
    ItemType type;
    uint8_t count;           // Stack size (1-64 for blocks, 1 for tools)
    uint16_t durability;     // Current durability (for tools)
    uint16_t max_durability; // Maximum durability
} ItemStack;

/**
 * Item properties - static data for each item type
 */
typedef struct {
    const char* name;
    uint8_t max_stack_size;  // Usually 64, tools = 1
    bool is_placeable;       // Can be placed as block
    BlockType places_as;     // Which block type it becomes when placed
    bool is_tool;
    uint16_t durability;     // Initial durability for tools
    int atlas_tile_x;        // Texture atlas coordinates
    int atlas_tile_y;
    // Tool mining properties
    ToolType tool_type;      // TOOL_PICKAXE, TOOL_AXE, etc.
    float dig_speed;         // Multiplier (1.0 = hand, 2.0 = wooden, 4.0 = stone)
} ItemProperties;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize item system
 */
void item_system_init(void);

/**
 * Get properties for an item type
 */
const ItemProperties* item_get_properties(ItemType type);

/**
 * Get the item dropped when a block is mined
 * Returns an ItemStack with type ITEM_NONE if block doesn't drop anything
 */
ItemStack item_get_block_drop(BlockType block_type);

/**
 * Check if two item stacks can be merged (same type, not tools)
 */
bool item_can_stack(const ItemStack* a, const ItemStack* b);

/**
 * Get the name of an item type
 */
const char* item_get_name(ItemType type);

/**
 * Calculate dig time for a block with a given tool
 * Returns time in seconds, 0 for instant, -1 for unbreakable
 */
float item_calculate_dig_time(BlockType block, ItemType tool);

/**
 * Check if a tool can harvest a block (get drops)
 * Some blocks require specific tools to drop items
 */
bool item_can_harvest_block(BlockType block, ItemType tool);

#endif // VOXEL_ITEM_H
