/**
 * Crafting System
 *
 * Implements recipe matching and crafting mechanics with shaped and shapeless recipes.
 */

#ifndef VOXEL_CRAFTING_H
#define VOXEL_CRAFTING_H

#include <stdbool.h>
#include "item.h"
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

#endif // VOXEL_CRAFTING_H
