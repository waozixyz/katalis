/**
 * Crafting System Implementation
 */

#include "voxel/inventory/crafting.h"
#include "voxel/core/item.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// RECIPE DATABASE
// ============================================================================

static CraftingRecipe g_recipes[MAX_RECIPES];
static int g_recipe_count = 0;

/**
 * Helper to add a recipe to the database
 */
static void add_recipe(RecipeType type, const ItemType inputs[9],
                      ItemType output, uint8_t output_count) {
    if (g_recipe_count >= MAX_RECIPES) {
        printf("[CRAFTING] ERROR: Recipe limit reached!\n");
        return;
    }

    CraftingRecipe* recipe = &g_recipes[g_recipe_count++];
    recipe->type = type;
    for (int i = 0; i < 9; i++) {
        recipe->inputs[i] = inputs[i];
    }
    recipe->output = output;
    recipe->output_count = output_count;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void crafting_init(void) {
    g_recipe_count = 0;

    printf("[CRAFTING] Initializing crafting system...\n");

    // Recipe 1: Wood Log → 4 Wood Planks (shapeless)
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOD_LOG, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_WOOD_PLANKS, 4);

    // Recipe 2: 1 Wood Plank → 4 Sticks (shapeless, Luanti style)
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_STICK, 4);

    // Recipe 3: Wooden Pickaxe (shaped)
    // Pattern:
    // P P P
    // . S .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_WOOD_PLANKS, ITEM_WOOD_PLANKS, ITEM_WOOD_PLANKS,
            ITEM_NONE, ITEM_STICK, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_WOODEN_PICKAXE, 1);

    // Recipe 4: Stone Pickaxe (shaped)
    // Pattern:
    // C C C
    // . S .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_COBBLESTONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_STONE_PICKAXE, 1);

    // Recipe 5: Wooden Axe (shaped)
    // Pattern:
    // P P .
    // P S .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_WOOD_PLANKS, ITEM_WOOD_PLANKS, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_STICK, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_WOODEN_AXE, 1);

    // Recipe 6: Stone Axe (shaped)
    // Pattern:
    // C C .
    // C S .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_COBBLESTONE, ITEM_COBBLESTONE, ITEM_NONE,
            ITEM_COBBLESTONE, ITEM_STICK, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_STONE_AXE, 1);

    // Recipe 7: Wooden Shovel (shaped)
    // Pattern:
    // . P .
    // . S .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_NONE, ITEM_WOOD_PLANKS, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_WOODEN_SHOVEL, 1);

    // Recipe 8: Stone Shovel (shaped)
    // Pattern:
    // . C .
    // . S .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_NONE, ITEM_COBBLESTONE, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_STONE_SHOVEL, 1);

    // Recipe 9: Wooden Sword (shaped)
    // Pattern:
    // . P .
    // . P .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_NONE, ITEM_WOOD_PLANKS, ITEM_NONE,
            ITEM_NONE, ITEM_WOOD_PLANKS, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_WOODEN_SWORD, 1);

    // Recipe 10: Stone Sword (shaped)
    // Pattern:
    // . C .
    // . C .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_NONE, ITEM_COBBLESTONE, ITEM_NONE,
            ITEM_NONE, ITEM_COBBLESTONE, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_STONE_SWORD, 1);

    // Recipe 11: Iron Sword (shaped)
    // Pattern:
    // . I .
    // . I .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_NONE, ITEM_IRON_BLOCK, ITEM_NONE,
            ITEM_NONE, ITEM_IRON_BLOCK, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_IRON_SWORD, 1);

    // Recipe 12: Diamond Sword (shaped)
    // Pattern:
    // . D .
    // . D .
    // . S .
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_NONE, ITEM_DIAMOND_BLOCK, ITEM_NONE,
            ITEM_NONE, ITEM_DIAMOND_BLOCK, ITEM_NONE,
            ITEM_NONE, ITEM_STICK, ITEM_NONE
        },
        ITEM_DIAMOND_SWORD, 1);

    // Recipe 13-28: Beds (16 colors) - shapeless
    // White Bed: 2 White Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_WHITE, ITEM_WOOL_WHITE, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_WHITE_BED, 1);

    // Light Gray Bed: 2 Light Gray Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_LIGHT_GRAY, ITEM_WOOL_LIGHT_GRAY, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_LIGHT_GRAY_BED, 1);

    // Gray Bed: 2 Gray Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_GRAY, ITEM_WOOL_GRAY, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_GRAY_BED, 1);

    // Black Bed: 2 Black Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_BLACK, ITEM_WOOL_BLACK, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_BLACK_BED, 1);

    // Brown Bed: 2 Brown Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_BROWN, ITEM_WOOL_BROWN, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_BROWN_BED, 1);

    // Red Bed: 2 Red Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_RED, ITEM_WOOL_RED, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_RED_BED, 1);

    // Orange Bed: 2 Orange Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_ORANGE, ITEM_WOOL_ORANGE, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_ORANGE_BED, 1);

    // Yellow Bed: 2 Yellow Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_YELLOW, ITEM_WOOL_YELLOW, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_YELLOW_BED, 1);

    // Lime Bed: 2 Lime Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_LIME, ITEM_WOOL_LIME, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_LIME_BED, 1);

    // Green Bed: 2 Green Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_GREEN, ITEM_WOOL_GREEN, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_GREEN_BED, 1);

    // Cyan Bed: 2 Cyan Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_CYAN, ITEM_WOOL_CYAN, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_CYAN_BED, 1);

    // Light Blue Bed: 2 Light Blue Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_LIGHT_BLUE, ITEM_WOOL_LIGHT_BLUE, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_LIGHT_BLUE_BED, 1);

    // Blue Bed: 2 Blue Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_BLUE, ITEM_WOOL_BLUE, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_BLUE_BED, 1);

    // Purple Bed: 2 Purple Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_PURPLE, ITEM_WOOL_PURPLE, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_PURPLE_BED, 1);

    // Magenta Bed: 2 Magenta Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_MAGENTA, ITEM_WOOL_MAGENTA, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_MAGENTA_BED, 1);

    // Pink Bed: 2 Pink Wool + Planks
    add_recipe(RECIPE_SHAPELESS,
        (ItemType[9]){
            ITEM_WOOL_PINK, ITEM_WOOL_PINK, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_NONE, ITEM_NONE,
            ITEM_NONE, ITEM_NONE, ITEM_NONE
        },
        ITEM_PINK_BED, 1);

    // Recipe 45: Wooden Door (shaped)
    // Pattern:
    // P P
    // P P
    // P P
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_WOOD_PLANKS, ITEM_WOOD_PLANKS, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_WOOD_PLANKS, ITEM_NONE,
            ITEM_WOOD_PLANKS, ITEM_WOOD_PLANKS, ITEM_NONE
        },
        ITEM_WOOD_DOOR, 3);

    // Recipe 46: Iron Door (shaped)
    // Pattern:
    // I I
    // I I
    // I I
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_IRON_BLOCK, ITEM_IRON_BLOCK, ITEM_NONE,
            ITEM_IRON_BLOCK, ITEM_IRON_BLOCK, ITEM_NONE,
            ITEM_IRON_BLOCK, ITEM_IRON_BLOCK, ITEM_NONE
        },
        ITEM_IRON_DOOR, 3);

    printf("[CRAFTING] Loaded %d recipes\n", g_recipe_count);
}

// ============================================================================
// RECIPE MATCHING
// ============================================================================

/**
 * Get bounding box of non-empty items in a 3x3 grid
 * Returns min_row, max_row, min_col, max_col
 */
static void get_pattern_bounds(const ItemType items[9], int* min_row, int* max_row, int* min_col, int* max_col) {
    *min_row = 3; *max_row = -1;
    *min_col = 3; *max_col = -1;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int idx = row * 3 + col;
            if (items[idx] != ITEM_NONE) {
                if (row < *min_row) *min_row = row;
                if (row > *max_row) *max_row = row;
                if (col < *min_col) *min_col = col;
                if (col > *max_col) *max_col = col;
            }
        }
    }
}

/**
 * Check if a shaped recipe matches the grid with position-shifting
 * The pattern can be placed anywhere in the grid as long as it fits
 */
static bool match_shaped_recipe(const CraftingRecipe* recipe, const ItemType grid[9]) {
    // Get bounds of recipe pattern
    int r_min_row, r_max_row, r_min_col, r_max_col;
    get_pattern_bounds(recipe->inputs, &r_min_row, &r_max_row, &r_min_col, &r_max_col);

    // Get bounds of grid items
    int g_min_row, g_max_row, g_min_col, g_max_col;
    get_pattern_bounds(grid, &g_min_row, &g_max_row, &g_min_col, &g_max_col);

    // Check if both are empty
    if (r_max_row < 0 && g_max_row < 0) return true;
    if (r_max_row < 0 || g_max_row < 0) return false;

    // Calculate pattern dimensions
    int r_height = r_max_row - r_min_row + 1;
    int r_width = r_max_col - r_min_col + 1;
    int g_height = g_max_row - g_min_row + 1;
    int g_width = g_max_col - g_min_col + 1;

    // Dimensions must match
    if (r_height != g_height || r_width != g_width) {
        return false;
    }

    // Compare pattern at the normalized positions
    for (int row = 0; row < r_height; row++) {
        for (int col = 0; col < r_width; col++) {
            int r_idx = (r_min_row + row) * 3 + (r_min_col + col);
            int g_idx = (g_min_row + row) * 3 + (g_min_col + col);
            if (recipe->inputs[r_idx] != grid[g_idx]) {
                return false;
            }
        }
    }

    return true;
}

/**
 * Check if a shapeless recipe matches the grid (order doesn't matter)
 */
static bool match_shapeless_recipe(const CraftingRecipe* recipe, const ItemType grid[9]) {
    // Count required ingredients
    int required_counts[ITEM_COUNT] = {0};
    for (int i = 0; i < 9; i++) {
        if (recipe->inputs[i] != ITEM_NONE) {
            required_counts[recipe->inputs[i]]++;
        }
    }

    // Count grid ingredients
    int grid_counts[ITEM_COUNT] = {0};
    for (int i = 0; i < 9; i++) {
        if (grid[i] != ITEM_NONE) {
            grid_counts[grid[i]]++;
        }
    }

    // Compare counts for all item types
    for (int type = 0; type < ITEM_COUNT; type++) {
        if (required_counts[type] != grid_counts[type]) {
            return false;
        }
    }

    return true;
}

const CraftingRecipe* crafting_find_match(const ItemType grid[9]) {
    for (int i = 0; i < g_recipe_count; i++) {
        const CraftingRecipe* recipe = &g_recipes[i];

        if (recipe->type == RECIPE_SHAPED) {
            if (match_shaped_recipe(recipe, grid)) {
                return recipe;
            }
        } else {  // RECIPE_SHAPELESS
            if (match_shapeless_recipe(recipe, grid)) {
                return recipe;
            }
        }
    }

    return NULL;  // No matching recipe
}

// ============================================================================
// CRAFTING OPERATIONS
// ============================================================================

void crafting_update_output(Inventory* inv) {
    if (!inv) return;

    // Extract grid item types
    ItemType grid[9];
    for (int i = 0; i < 9; i++) {
        grid[i] = inv->crafting_grid[i].type;
    }

    // Find matching recipe
    const CraftingRecipe* recipe = crafting_find_match(grid);

    if (recipe) {
        // Set output slot
        inv->crafting_output[0].type = recipe->output;
        inv->crafting_output[0].count = recipe->output_count;

        const ItemProperties* props = item_get_properties(recipe->output);
        inv->crafting_output[0].durability = props->durability;
        inv->crafting_output[0].max_durability = props->durability;
    } else {
        // Clear output slot
        inv->crafting_output[0].type = ITEM_NONE;
        inv->crafting_output[0].count = 0;
        inv->crafting_output[0].durability = 0;
        inv->crafting_output[0].max_durability = 0;
    }
}

bool crafting_try_craft(Inventory* inv) {
    if (!inv) return false;

    // Check if output slot has an item
    if (inv->crafting_output[0].type == ITEM_NONE) {
        return false;  // No valid recipe
    }

    // Extract grid item types
    ItemType grid[9];
    for (int i = 0; i < 9; i++) {
        grid[i] = inv->crafting_grid[i].type;
    }

    // Find matching recipe
    const CraftingRecipe* recipe = crafting_find_match(grid);
    if (!recipe) {
        return false;  // Shouldn't happen if output is set, but safety check
    }

    // Consume input items from crafting grid
    for (int i = 0; i < 9; i++) {
        if (inv->crafting_grid[i].type != ITEM_NONE) {
            inv->crafting_grid[i].count--;
            if (inv->crafting_grid[i].count == 0) {
                inv->crafting_grid[i].type = ITEM_NONE;
                inv->crafting_grid[i].durability = 0;
                inv->crafting_grid[i].max_durability = 0;
            }
        }
    }

    // Update output (might be empty now if inputs are consumed)
    crafting_update_output(inv);

    printf("[CRAFTING] Crafted %d x %s\n",
           recipe->output_count,
           item_get_name(recipe->output));

    return true;
}

int crafting_get_max_craft_count(Inventory* inv) {
    if (!inv) return 0;

    // Extract grid item types
    ItemType grid[9];
    for (int i = 0; i < 9; i++) {
        grid[i] = inv->crafting_grid[i].type;
    }

    // Find matching recipe
    const CraftingRecipe* recipe = crafting_find_match(grid);
    if (!recipe) return 0;

    // Calculate max crafts based on ingredient counts
    int max_crafts = 999;  // Start high

    for (int i = 0; i < 9; i++) {
        if (recipe->inputs[i] != ITEM_NONE) {
            // Find how many of this ingredient we have
            int have = inv->crafting_grid[i].count;
            if (have < max_crafts) {
                max_crafts = have;
            }
        }
    }

    // Also limit by stack size of output (and prevent uint8_t overflow)
    const ItemProperties* props = item_get_properties(recipe->output);
    int max_output = props->max_stack_size / recipe->output_count;
    if (max_output < max_crafts) {
        max_crafts = max_output;
    }

    // Ensure we don't overflow uint8_t (max 255)
    int max_items = max_crafts * recipe->output_count;
    if (max_items > 255) {
        max_crafts = 255 / recipe->output_count;
    }

    return max_crafts > 0 ? max_crafts : 0;
}

ItemStack crafting_craft_all(Inventory* inv) {
    ItemStack result = {ITEM_NONE, 0, 0, 0};
    if (!inv) return result;

    // Check if output slot has an item
    if (inv->crafting_output[0].type == ITEM_NONE) {
        return result;
    }

    // Get max craft count
    int max_crafts = crafting_get_max_craft_count(inv);
    if (max_crafts <= 0) return result;

    // Find the recipe
    ItemType grid[9];
    for (int i = 0; i < 9; i++) {
        grid[i] = inv->crafting_grid[i].type;
    }
    const CraftingRecipe* recipe = crafting_find_match(grid);
    if (!recipe) return result;

    // Set up result
    result.type = recipe->output;
    result.count = max_crafts * recipe->output_count;

    const ItemProperties* props = item_get_properties(recipe->output);
    result.durability = props->durability;
    result.max_durability = props->durability;

    // Consume input items (for all crafts)
    for (int i = 0; i < 9; i++) {
        if (inv->crafting_grid[i].type != ITEM_NONE) {
            inv->crafting_grid[i].count -= max_crafts;
            if (inv->crafting_grid[i].count <= 0) {
                inv->crafting_grid[i].type = ITEM_NONE;
                inv->crafting_grid[i].count = 0;
                inv->crafting_grid[i].durability = 0;
                inv->crafting_grid[i].max_durability = 0;
            }
        }
    }

    // Update output
    crafting_update_output(inv);

    printf("[CRAFTING] Craft all: %d x %s\n",
           result.count, item_get_name(result.type));

    return result;
}

// ============================================================================
// RECIPE QUERY API (for crafting guide)
// ============================================================================

int crafting_get_recipe_count(void) {
    return g_recipe_count;
}

const CraftingRecipe* crafting_get_recipe(int index) {
    if (index < 0 || index >= g_recipe_count) {
        return NULL;
    }
    return &g_recipes[index];
}

bool crafting_can_craft_recipe(Inventory* inv, const CraftingRecipe* recipe) {
    if (!inv || !recipe) return false;

    // Count required ingredients from recipe
    int required[ITEM_COUNT] = {0};
    for (int i = 0; i < 9; i++) {
        if (recipe->inputs[i] != ITEM_NONE) {
            required[recipe->inputs[i]]++;
        }
    }

    // Count available items in inventory (hotbar + main inventory + crafting grid)
    int available[ITEM_COUNT] = {0};
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        if (inv->hotbar[i].type != ITEM_NONE) {
            available[inv->hotbar[i].type] += inv->hotbar[i].count;
        }
    }
    for (int i = 0; i < MAIN_INVENTORY_SIZE; i++) {
        if (inv->main_inventory[i].type != ITEM_NONE) {
            available[inv->main_inventory[i].type] += inv->main_inventory[i].count;
        }
    }
    // Also count items in crafting grid (user may have moved items there)
    for (int i = 0; i < 9; i++) {
        if (inv->crafting_grid[i].type != ITEM_NONE) {
            available[inv->crafting_grid[i].type] += inv->crafting_grid[i].count;
        }
    }
    // Also count held item (cursor item being dragged)
    if (inv->is_holding_item && inv->held_item.type != ITEM_NONE) {
        available[inv->held_item.type] += inv->held_item.count;
    }

    // Check if all required ingredients are available
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (required[i] > available[i]) {
            return false;
        }
    }

    return true;
}

const CraftingRecipe* crafting_find_recipe_for_output(ItemType output) {
    if (output == ITEM_NONE) return NULL;

    for (int i = 0; i < g_recipe_count; i++) {
        if (g_recipes[i].output == output) {
            return &g_recipes[i];
        }
    }
    return NULL;
}

int crafting_count_available_crafts(Inventory* inv, const CraftingRecipe* recipe) {
    if (!inv || !recipe) return 0;

    // Count required ingredients
    int required[ITEM_COUNT] = {0};
    for (int i = 0; i < 9; i++) {
        if (recipe->inputs[i] != ITEM_NONE) {
            required[recipe->inputs[i]]++;
        }
    }

    // Count available items from all sources
    int available[ITEM_COUNT] = {0};
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        if (inv->hotbar[i].type != ITEM_NONE) {
            available[inv->hotbar[i].type] += inv->hotbar[i].count;
        }
    }
    for (int i = 0; i < MAIN_INVENTORY_SIZE; i++) {
        if (inv->main_inventory[i].type != ITEM_NONE) {
            available[inv->main_inventory[i].type] += inv->main_inventory[i].count;
        }
    }
    for (int i = 0; i < 9; i++) {
        if (inv->crafting_grid[i].type != ITEM_NONE) {
            available[inv->crafting_grid[i].type] += inv->crafting_grid[i].count;
        }
    }

    // Find the minimum number of crafts possible
    int max_crafts = 999;
    for (int type = 0; type < ITEM_COUNT; type++) {
        if (required[type] > 0) {
            int possible = available[type] / required[type];
            if (possible < max_crafts) {
                max_crafts = possible;
            }
        }
    }

    return max_crafts == 999 ? 0 : max_crafts;
}

/**
 * Helper: Take items from inventory sources
 * Returns the number of items actually taken
 */
static int take_items_from_inventory(Inventory* inv, ItemType type, int count) {
    int taken = 0;

    // Take from crafting grid first (items already there)
    for (int i = 0; i < 9 && taken < count; i++) {
        if (inv->crafting_grid[i].type == type) {
            int to_take = count - taken;
            if (to_take > inv->crafting_grid[i].count) {
                to_take = inv->crafting_grid[i].count;
            }
            inv->crafting_grid[i].count -= to_take;
            if (inv->crafting_grid[i].count == 0) {
                inv->crafting_grid[i].type = ITEM_NONE;
            }
            taken += to_take;
        }
    }

    // Take from hotbar
    for (int i = 0; i < HOTBAR_SIZE && taken < count; i++) {
        if (inv->hotbar[i].type == type) {
            int to_take = count - taken;
            if (to_take > inv->hotbar[i].count) {
                to_take = inv->hotbar[i].count;
            }
            inv->hotbar[i].count -= to_take;
            if (inv->hotbar[i].count == 0) {
                inv->hotbar[i].type = ITEM_NONE;
            }
            taken += to_take;
        }
    }

    // Take from main inventory
    for (int i = 0; i < MAIN_INVENTORY_SIZE && taken < count; i++) {
        if (inv->main_inventory[i].type == type) {
            int to_take = count - taken;
            if (to_take > inv->main_inventory[i].count) {
                to_take = inv->main_inventory[i].count;
            }
            inv->main_inventory[i].count -= to_take;
            if (inv->main_inventory[i].count == 0) {
                inv->main_inventory[i].type = ITEM_NONE;
            }
            taken += to_take;
        }
    }

    return taken;
}

bool crafting_auto_place_ingredients(Inventory* inv, const CraftingRecipe* recipe, int count) {
    if (!inv || !recipe) return false;

    // Calculate max crafts if count is -1 (all)
    int max_available = crafting_count_available_crafts(inv, recipe);
    if (max_available == 0) return false;

    int actual_count = count;
    if (count < 0 || count > max_available) {
        actual_count = max_available;
    }

    // Clear crafting grid first
    for (int i = 0; i < 9; i++) {
        if (inv->crafting_grid[i].type != ITEM_NONE) {
            // Return items to inventory
            inventory_add_item(inv, inv->crafting_grid[i].type, inv->crafting_grid[i].count);
            inv->crafting_grid[i].type = ITEM_NONE;
            inv->crafting_grid[i].count = 0;
        }
    }

    // Count how many of each ingredient we need
    int needed[ITEM_COUNT] = {0};
    for (int i = 0; i < 9; i++) {
        if (recipe->inputs[i] != ITEM_NONE) {
            needed[recipe->inputs[i]] += actual_count;
        }
    }

    // Take ingredients from inventory
    for (int type = 0; type < ITEM_COUNT; type++) {
        if (needed[type] > 0) {
            take_items_from_inventory(inv, type, needed[type]);
        }
    }

    // Place ingredients in crafting grid according to recipe pattern
    for (int i = 0; i < 9; i++) {
        if (recipe->inputs[i] != ITEM_NONE) {
            inv->crafting_grid[i].type = recipe->inputs[i];
            inv->crafting_grid[i].count = actual_count;
            inv->crafting_grid[i].durability = 0;
            inv->crafting_grid[i].max_durability = 0;
        }
    }

    // Update output
    crafting_update_output(inv);

    printf("[CRAFTING] Auto-placed %d x recipe ingredients\n", actual_count);
    return true;
}
