/**
 * Item System
 *
 * Defines item types, properties, and drop tables for the inventory system.
 */

#ifndef VOXEL_ITEM_H
#define VOXEL_ITEM_H

#include <stdint.h>
#include <stdbool.h>
#include "block.h"

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

    // Crafting materials
    ITEM_STICK,

    // Tools
    ITEM_WOODEN_PICKAXE,
    ITEM_STONE_PICKAXE,
    ITEM_WOODEN_SHOVEL,
    ITEM_STONE_SHOVEL,
    ITEM_WOODEN_AXE,
    ITEM_STONE_AXE,

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

#endif // VOXEL_ITEM_H
