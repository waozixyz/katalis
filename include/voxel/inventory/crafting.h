/**
 * Crafting System
 *
 * Implements recipe matching and crafting mechanics with shaped and shapeless recipes.
 */

#ifndef VOXEL_CRAFTING_H
#define VOXEL_CRAFTING_H

#include <stdbool.h>
#include "voxel/core/item.h"
#include "inventory.h"

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_RECIPES 32

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Recipe types
 */
typedef enum {
    RECIPE_SHAPED,      // Exact pattern required (position matters)
    RECIPE_SHAPELESS    // Just needs ingredients (order doesn't matter)
} RecipeType;

/**
 * Crafting recipe structure
 */
typedef struct {
    RecipeType type;
    ItemType inputs[9];      // 3x3 grid pattern (row-major order)
    ItemType output;         // Result item type
    uint8_t output_count;    // Number of items produced
} CraftingRecipe;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize crafting system and load all recipes
 */
void crafting_init(void);

/**
 * Find a recipe that matches the given crafting grid
 * Returns pointer to recipe if found, NULL otherwise
 */
const CraftingRecipe* crafting_find_match(const ItemType grid[9]);

/**
 * Update the crafting output slot based on current crafting grid
 * Should be called whenever the crafting grid changes
 */
void crafting_update_output(Inventory* inv);

/**
 * Attempt to craft from the current grid
 * Consumes input items and produces output if recipe matches
 * Returns true if crafting succeeded
 */
bool crafting_try_craft(Inventory* inv);

/**
 * Calculate how many times we can craft the current recipe
 * Returns 0 if no valid recipe
 */
int crafting_get_max_craft_count(Inventory* inv);

/**
 * Craft as many items as possible and return them as ItemStack
 * Used for shift-click on crafting output
 * Returns the crafted items (caller must add to inventory)
 */
ItemStack crafting_craft_all(Inventory* inv);

// ============================================================================
// RECIPE QUERY API (for crafting guide)
// ============================================================================

/**
 * Get the total number of registered recipes
 */
int crafting_get_recipe_count(void);

/**
 * Get a recipe by index (0 to count-1)
 * Returns NULL if index is out of bounds
 */
const CraftingRecipe* crafting_get_recipe(int index);

/**
 * Check if player has all ingredients in their inventory to craft a recipe
 * Checks both hotbar and main inventory
 */
bool crafting_can_craft_recipe(Inventory* inv, const CraftingRecipe* recipe);

#endif // VOXEL_CRAFTING_H
