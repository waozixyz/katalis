/**
 * Crafting System Implementation
 */

#include "voxel/crafting.h"
#include "voxel/item.h"
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

    // Recipe 2: 2 Wood Planks → 4 Sticks (shaped, vertical)
    add_recipe(RECIPE_SHAPED,
        (ItemType[9]){
            ITEM_NONE, ITEM_WOOD_PLANKS, ITEM_NONE,
            ITEM_NONE, ITEM_WOOD_PLANKS, ITEM_NONE,
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

    printf("[CRAFTING] Loaded %d recipes\n", g_recipe_count);
}

// ============================================================================
// RECIPE MATCHING
// ============================================================================

/**
 * Check if a shaped recipe matches the grid exactly
 */
static bool match_shaped_recipe(const CraftingRecipe* recipe, const ItemType grid[9]) {
    for (int i = 0; i < 9; i++) {
        if (recipe->inputs[i] != grid[i]) {
            return false;
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
