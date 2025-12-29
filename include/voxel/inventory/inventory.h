/**
 * Inventory System
 *
 * Manages player inventory with hotbar, main inventory, and crafting grid.
 */

#ifndef VOXEL_INVENTORY_H
#define VOXEL_INVENTORY_H

#include <stdbool.h>
#include "voxel/core/item.h"

// ============================================================================
// CONSTANTS
// ============================================================================

#define HOTBAR_SIZE 9
#define MAIN_INVENTORY_SIZE 27
#define CRAFTING_GRID_SIZE 9
#define CRAFTING_OUTPUT_SIZE 1

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Inventory structure
 */
typedef struct Inventory {
    ItemStack hotbar[HOTBAR_SIZE];                    // Quick access slots (1-9 keys)
    ItemStack main_inventory[MAIN_INVENTORY_SIZE];    // 3 rows x 9 columns
    ItemStack crafting_grid[CRAFTING_GRID_SIZE];      // 3x3 crafting input
    ItemStack crafting_output[CRAFTING_OUTPUT_SIZE];  // 1 crafting result

    int selected_hotbar_slot;  // 0-8, which hotbar slot is active
    bool is_open;              // Is full inventory UI visible?

    ItemStack held_item;       // Item being dragged by cursor
    bool is_holding_item;      // Is player holding an item?
} Inventory;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Create and initialize a new inventory
 */
Inventory* inventory_create(void);

/**
 * Destroy inventory and free memory
 */
void inventory_destroy(Inventory* inv);

/**
 * Add items to inventory (tries hotbar first, then main inventory)
 * Returns true if all items were added, false if inventory is full
 */
bool inventory_add_item(Inventory* inv, ItemType type, uint8_t count);

/**
 * Remove items from a specific slot
 * Returns true if successful
 */
bool inventory_remove_item(Inventory* inv, int slot_index, uint8_t count);

/**
 * Check if inventory can fit more items
 */
bool inventory_can_add_item(Inventory* inv, ItemType type, uint8_t count);

/**
 * Get pointer to selected hotbar item
 */
ItemStack* inventory_get_selected_hotbar_item(Inventory* inv);

/**
 * Set selected hotbar slot (0-8)
 */
void inventory_set_selected_slot(Inventory* inv, int slot);

/**
 * Get pointer to a slot by global index
 * Index mapping:
 *   0-8: Hotbar
 *   9-35: Main inventory
 *   36-44: Crafting grid
 *   45: Crafting output
 */
ItemStack* inventory_get_slot(Inventory* inv, int index);

/**
 * Get total number of slots (for iteration)
 */
int inventory_get_total_slots(Inventory* inv);

/**
 * Clear all items from inventory
 */
void inventory_clear(Inventory* inv);

#endif // VOXEL_INVENTORY_H
