/**
 * Inventory Input System
 *
 * Handles mouse interaction with inventory UI (clicking, dragging items between slots).
 */

#ifndef VOXEL_INVENTORY_INPUT_H
#define VOXEL_INVENTORY_INPUT_H

#include <stdbool.h>
#include "voxel/inventory/inventory.h"

// ============================================================================
// CONSTANTS
// ============================================================================

// Inventory sections for slot identification
typedef enum {
    SECTION_NONE = -1,
    SECTION_CRAFTING_GRID = 0,
    SECTION_CRAFTING_OUTPUT = 1,
    SECTION_MAIN_INVENTORY = 2,
    SECTION_HOTBAR = 3
} InventorySection;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Handle left-click on inventory
 * - Click on empty slot with held item: place entire stack
 * - Click on item slot with empty hand: pick up entire stack
 * - Click on item slot with held item: swap stacks
 */
void inventory_input_handle_left_click(Inventory* inv, int mouse_x, int mouse_y);

/**
 * Handle right-click on inventory
 * - Click on empty slot with held item: place 1 item
 * - Click on item slot with empty hand: pick up half stack (rounded up)
 * - Click on item slot with held item: place 1 item if same type
 */
void inventory_input_handle_right_click(Inventory* inv, int mouse_x, int mouse_y);

/**
 * Handle shift+left-click on inventory
 * - Quick transfer items to/from hotbar and main inventory
 */
void inventory_input_handle_shift_click(Inventory* inv, int mouse_x, int mouse_y);

/**
 * Get the slot index and section from mouse coordinates
 * Returns slot index within that section, or -1 if no slot clicked
 * Sets section to the inventory section that was clicked
 */
int inventory_input_get_clicked_slot(int mouse_x, int mouse_y, InventorySection* section);

#endif // VOXEL_INVENTORY_INPUT_H
