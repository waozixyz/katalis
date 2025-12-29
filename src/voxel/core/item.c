/**
 * Item System Implementation
 */

#include "voxel/core/item.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// DROP TABLE
// ============================================================================

typedef struct {
    BlockType block;
    ItemType drop;
    uint8_t min_count;
    uint8_t max_count;
} DropEntry;

static const DropEntry g_drop_table[] = {
    {BLOCK_GRASS,       ITEM_DIRT,         1, 1},
    {BLOCK_DIRT,        ITEM_DIRT,         1, 1},
    {BLOCK_STONE,       ITEM_COBBLESTONE,  1, 1},
    {BLOCK_WOOD,        ITEM_WOOD_LOG,     1, 1},
    {BLOCK_LEAVES,      ITEM_LEAVES,       1, 1},  // Drops leaves
    {BLOCK_SAND,        ITEM_SAND,         1, 1},
    {BLOCK_WATER,       ITEM_NONE,         0, 0},  // No drop
    {BLOCK_COBBLESTONE, ITEM_COBBLESTONE,  1, 1},
    {BLOCK_BEDROCK,     ITEM_NONE,         0, 0},  // Unbreakable
};

// ============================================================================
// ITEM PROPERTIES TABLE
// ============================================================================

static const ItemProperties g_item_properties[ITEM_COUNT] = {
    [ITEM_NONE] = {
        .name = "Air",
        .max_stack_size = 0,
        .is_placeable = false,
        .places_as = BLOCK_AIR,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 0,
        .atlas_tile_y = 0,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },

    // Block items
    [ITEM_DIRT] = {
        .name = "Dirt",
        .max_stack_size = 64,
        .is_placeable = true,
        .places_as = BLOCK_DIRT,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 0,
        .atlas_tile_y = 1,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },
    [ITEM_GRASS_BLOCK] = {
        .name = "Grass Block",
        .max_stack_size = 64,
        .is_placeable = true,
        .places_as = BLOCK_GRASS,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 0,
        .atlas_tile_y = 0,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },
    [ITEM_STONE] = {
        .name = "Stone",
        .max_stack_size = 64,
        .is_placeable = true,
        .places_as = BLOCK_STONE,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 0,
        .atlas_tile_y = 2,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },
    [ITEM_COBBLESTONE] = {
        .name = "Cobblestone",
        .max_stack_size = 64,
        .is_placeable = true,
        .places_as = BLOCK_COBBLESTONE,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 0,
        .atlas_tile_y = 7,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },
    [ITEM_SAND] = {
        .name = "Sand",
        .max_stack_size = 64,
        .is_placeable = true,
        .places_as = BLOCK_SAND,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 0,
        .atlas_tile_y = 5,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },
    [ITEM_WOOD_LOG] = {
        .name = "Oak Log",
        .max_stack_size = 64,
        .is_placeable = true,
        .places_as = BLOCK_WOOD,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 0,
        .atlas_tile_y = 3,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },
    [ITEM_WOOD_PLANKS] = {
        .name = "Oak Planks",
        .max_stack_size = 64,
        .is_placeable = false,  // Crafting material only
        .places_as = BLOCK_AIR,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 1,
        .atlas_tile_y = 3,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },
    [ITEM_LEAVES] = {
        .name = "Leaves",
        .max_stack_size = 64,
        .is_placeable = true,
        .places_as = BLOCK_LEAVES,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 0,
        .atlas_tile_y = 4,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },
    [ITEM_BEDROCK] = {
        .name = "Bedrock",
        .max_stack_size = 64,
        .is_placeable = true,
        .places_as = BLOCK_BEDROCK,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 0,
        .atlas_tile_y = 8,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },

    // Crafting materials
    [ITEM_STICK] = {
        .name = "Stick",
        .max_stack_size = 64,
        .is_placeable = false,
        .places_as = BLOCK_AIR,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 2,
        .atlas_tile_y = 3,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },

    // Tools
    [ITEM_WOODEN_PICKAXE] = {
        .name = "Wooden Pickaxe",
        .max_stack_size = 1,
        .is_placeable = false,
        .places_as = BLOCK_AIR,
        .is_tool = true,
        .durability = 60,
        .atlas_tile_x = 3,
        .atlas_tile_y = 0,
        .tool_type = TOOL_PICKAXE,
        .dig_speed = 2.0f,
    },
    [ITEM_STONE_PICKAXE] = {
        .name = "Stone Pickaxe",
        .max_stack_size = 1,
        .is_placeable = false,
        .places_as = BLOCK_AIR,
        .is_tool = true,
        .durability = 132,
        .atlas_tile_x = 3,
        .atlas_tile_y = 1,
        .tool_type = TOOL_PICKAXE,
        .dig_speed = 4.0f,
    },
    [ITEM_WOODEN_SHOVEL] = {
        .name = "Wooden Shovel",
        .max_stack_size = 1,
        .is_placeable = false,
        .places_as = BLOCK_AIR,
        .is_tool = true,
        .durability = 60,
        .atlas_tile_x = 4,
        .atlas_tile_y = 0,
        .tool_type = TOOL_SHOVEL,
        .dig_speed = 2.0f,
    },
    [ITEM_STONE_SHOVEL] = {
        .name = "Stone Shovel",
        .max_stack_size = 1,
        .is_placeable = false,
        .places_as = BLOCK_AIR,
        .is_tool = true,
        .durability = 132,
        .atlas_tile_x = 4,
        .atlas_tile_y = 1,
        .tool_type = TOOL_SHOVEL,
        .dig_speed = 4.0f,
    },
    [ITEM_WOODEN_AXE] = {
        .name = "Wooden Axe",
        .max_stack_size = 1,
        .is_placeable = false,
        .places_as = BLOCK_AIR,
        .is_tool = true,
        .durability = 60,
        .atlas_tile_x = 5,
        .atlas_tile_y = 0,
        .tool_type = TOOL_AXE,
        .dig_speed = 2.0f,
    },
    [ITEM_STONE_AXE] = {
        .name = "Stone Axe",
        .max_stack_size = 1,
        .is_placeable = false,
        .places_as = BLOCK_AIR,
        .is_tool = true,
        .durability = 132,
        .atlas_tile_x = 5,
        .atlas_tile_y = 1,
        .tool_type = TOOL_AXE,
        .dig_speed = 4.0f,
    },
    [ITEM_MEAT] = {
        .name = "Raw Meat",
        .max_stack_size = 64,
        .is_placeable = false,
        .places_as = BLOCK_AIR,
        .is_tool = false,
        .durability = 0,
        .atlas_tile_x = 6,
        .atlas_tile_y = 0,
        .tool_type = TOOL_NONE,
        .dig_speed = 1.0f,
    },
};

// ============================================================================
// PUBLIC API
// ============================================================================

void item_system_init(void) {
    printf("[ITEM] Item system initialized with %d item types\n", ITEM_COUNT - 1);
}

const ItemProperties* item_get_properties(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) {
        return &g_item_properties[ITEM_NONE];
    }
    return &g_item_properties[type];
}

ItemStack item_get_block_drop(BlockType block_type) {
    ItemStack drop = {ITEM_NONE, 0, 0, 0};

    for (size_t i = 0; i < sizeof(g_drop_table) / sizeof(DropEntry); i++) {
        if (g_drop_table[i].block == block_type) {
            drop.type = g_drop_table[i].drop;
            drop.count = g_drop_table[i].min_count;

            if (drop.type != ITEM_NONE) {
                const ItemProperties* props = item_get_properties(drop.type);
                drop.max_durability = props->durability;
                drop.durability = props->durability;
            }

            return drop;
        }
    }

    return drop;
}

bool item_can_stack(const ItemStack* a, const ItemStack* b) {
    if (!a || !b) return false;
    if (a->type != b->type) return false;
    if (a->type == ITEM_NONE) return false;

    const ItemProperties* props = item_get_properties(a->type);

    // Tools can't stack
    if (props->is_tool) return false;

    return true;
}

const char* item_get_name(ItemType type) {
    const ItemProperties* props = item_get_properties(type);
    return props->name;
}

float item_calculate_dig_time(BlockType block, ItemType tool) {
    const BlockProperties* bp = block_get_properties(block);
    const ItemProperties* ip = item_get_properties(tool);

    // Unbreakable blocks (bedrock)
    if (bp->hardness < 0) return -1.0f;

    // Instant break (air, water)
    if (bp->hardness == 0) return 0.0f;

    float base_time = bp->hardness;
    float speed = 1.0f;  // Hand speed

    // Check if tool matches preferred type
    if (ip && ip->is_tool && ip->tool_type == bp->preferred_tool) {
        speed = ip->dig_speed;
    }

    return base_time / speed;
}

bool item_can_harvest_block(BlockType block, ItemType tool) {
    const BlockProperties* bp = block_get_properties(block);

    // Blocks that don't require tools always drop
    if (!bp->requires_tool) return true;

    const ItemProperties* ip = item_get_properties(tool);

    // Must have a matching tool
    return (ip && ip->is_tool && ip->tool_type == bp->preferred_tool);
}
