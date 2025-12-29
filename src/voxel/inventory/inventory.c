/**
 * Inventory System Implementation
 */

#include "voxel/inventory/inventory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

Inventory* inventory_create(void) {
    Inventory* inv = (Inventory*)calloc(1, sizeof(Inventory));
    if (!inv) return NULL;

    // Initialize all slots to empty
    memset(inv->hotbar, 0, sizeof(inv->hotbar));
    memset(inv->main_inventory, 0, sizeof(inv->main_inventory));
    memset(inv->crafting_grid, 0, sizeof(inv->crafting_grid));
    memset(inv->crafting_output, 0, sizeof(inv->crafting_output));

    inv->selected_hotbar_slot = 0;
    inv->is_open = false;
    inv->is_holding_item = false;
    memset(&inv->held_item, 0, sizeof(ItemStack));

    printf("[INVENTORY] Inventory created (%d hotbar + %d main + %d crafting = %d total slots)\n",
           HOTBAR_SIZE, MAIN_INVENTORY_SIZE, CRAFTING_GRID_SIZE + CRAFTING_OUTPUT_SIZE,
           HOTBAR_SIZE + MAIN_INVENTORY_SIZE + CRAFTING_GRID_SIZE + CRAFTING_OUTPUT_SIZE);

    return inv;
}

void inventory_destroy(Inventory* inv) {
    if (inv) {
        free(inv);
    }
}

void inventory_clear(Inventory* inv) {
    if (!inv) return;

    memset(inv->hotbar, 0, sizeof(inv->hotbar));
    memset(inv->main_inventory, 0, sizeof(inv->main_inventory));
    memset(inv->crafting_grid, 0, sizeof(inv->crafting_grid));
    memset(inv->crafting_output, 0, sizeof(inv->crafting_output));
    memset(&inv->held_item, 0, sizeof(ItemStack));
    inv->is_holding_item = false;
}

// ============================================================================
// SLOT ACCESS
// ============================================================================

ItemStack* inventory_get_slot(Inventory* inv, int index) {
    if (!inv) return NULL;

    if (index >= 0 && index < HOTBAR_SIZE) {
        return &inv->hotbar[index];
    } else if (index >= HOTBAR_SIZE && index < HOTBAR_SIZE + MAIN_INVENTORY_SIZE) {
        return &inv->main_inventory[index - HOTBAR_SIZE];
    } else if (index >= HOTBAR_SIZE + MAIN_INVENTORY_SIZE &&
               index < HOTBAR_SIZE + MAIN_INVENTORY_SIZE + CRAFTING_GRID_SIZE) {
        return &inv->crafting_grid[index - HOTBAR_SIZE - MAIN_INVENTORY_SIZE];
    } else if (index == HOTBAR_SIZE + MAIN_INVENTORY_SIZE + CRAFTING_GRID_SIZE) {
        return &inv->crafting_output[0];
    }

    return NULL;
}

int inventory_get_total_slots(Inventory* inv) {
    (void)inv;
    return HOTBAR_SIZE + MAIN_INVENTORY_SIZE + CRAFTING_GRID_SIZE + CRAFTING_OUTPUT_SIZE;
}

ItemStack* inventory_get_selected_hotbar_item(Inventory* inv) {
    if (!inv) return NULL;
    if (inv->selected_hotbar_slot < 0 || inv->selected_hotbar_slot >= HOTBAR_SIZE) {
        return NULL;
    }
    return &inv->hotbar[inv->selected_hotbar_slot];
}

void inventory_set_selected_slot(Inventory* inv, int slot) {
    if (!inv) return;
    if (slot < 0 || slot >= HOTBAR_SIZE) return;
    inv->selected_hotbar_slot = slot;
}

// ============================================================================
// ITEM MANAGEMENT
// ============================================================================

bool inventory_can_add_item(Inventory* inv, ItemType type, uint8_t count) {
    if (!inv || type == ITEM_NONE || count == 0) return false;

    const ItemProperties* props = item_get_properties(type);
    uint8_t remaining = count;

    // Try to stack with existing items in hotbar
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        if (inv->hotbar[i].type == type && inv->hotbar[i].count < props->max_stack_size) {
            uint8_t space = props->max_stack_size - inv->hotbar[i].count;
            if (space >= remaining) return true;
            remaining -= space;
        }
    }

    // Try to stack with existing items in main inventory
    for (int i = 0; i < MAIN_INVENTORY_SIZE; i++) {
        if (inv->main_inventory[i].type == type && inv->main_inventory[i].count < props->max_stack_size) {
            uint8_t space = props->max_stack_size - inv->main_inventory[i].count;
            if (space >= remaining) return true;
            remaining -= space;
        }
    }

    // Count empty slots in hotbar
    int empty_slots = 0;
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        if (inv->hotbar[i].type == ITEM_NONE) empty_slots++;
    }

    // Count empty slots in main inventory
    for (int i = 0; i < MAIN_INVENTORY_SIZE; i++) {
        if (inv->main_inventory[i].type == ITEM_NONE) empty_slots++;
    }

    // Calculate how many new stacks we need
    int stacks_needed = (remaining + props->max_stack_size - 1) / props->max_stack_size;

    return empty_slots >= stacks_needed;
}

bool inventory_add_item(Inventory* inv, ItemType type, uint8_t count) {
    if (!inv || type == ITEM_NONE || count == 0) return false;

    const ItemProperties* props = item_get_properties(type);
    uint8_t remaining = count;

    // Phase 1: Try to stack with existing items in hotbar
    for (int i = 0; i < HOTBAR_SIZE && remaining > 0; i++) {
        if (inv->hotbar[i].type == type && inv->hotbar[i].count < props->max_stack_size) {
            uint8_t space = props->max_stack_size - inv->hotbar[i].count;
            uint8_t to_add = (remaining < space) ? remaining : space;
            inv->hotbar[i].count += to_add;
            remaining -= to_add;
        }
    }

    // Phase 2: Try to stack with existing items in main inventory
    for (int i = 0; i < MAIN_INVENTORY_SIZE && remaining > 0; i++) {
        if (inv->main_inventory[i].type == type && inv->main_inventory[i].count < props->max_stack_size) {
            uint8_t space = props->max_stack_size - inv->main_inventory[i].count;
            uint8_t to_add = (remaining < space) ? remaining : space;
            inv->main_inventory[i].count += to_add;
            remaining -= to_add;
        }
    }

    // Phase 3: Create new stacks in hotbar empty slots
    for (int i = 0; i < HOTBAR_SIZE && remaining > 0; i++) {
        if (inv->hotbar[i].type == ITEM_NONE) {
            uint8_t to_add = (remaining < props->max_stack_size) ? remaining : props->max_stack_size;
            inv->hotbar[i].type = type;
            inv->hotbar[i].count = to_add;
            inv->hotbar[i].durability = props->durability;
            inv->hotbar[i].max_durability = props->durability;
            remaining -= to_add;
        }
    }

    // Phase 4: Create new stacks in main inventory empty slots
    for (int i = 0; i < MAIN_INVENTORY_SIZE && remaining > 0; i++) {
        if (inv->main_inventory[i].type == ITEM_NONE) {
            uint8_t to_add = (remaining < props->max_stack_size) ? remaining : props->max_stack_size;
            inv->main_inventory[i].type = type;
            inv->main_inventory[i].count = to_add;
            inv->main_inventory[i].durability = props->durability;
            inv->main_inventory[i].max_durability = props->durability;
            remaining -= to_add;
        }
    }

    // Return true if all items were added
    return remaining == 0;
}

bool inventory_remove_item(Inventory* inv, int slot_index, uint8_t count) {
    if (!inv || count == 0) return false;

    ItemStack* slot = inventory_get_slot(inv, slot_index);
    if (!slot || slot->type == ITEM_NONE) return false;

    if (slot->count < count) return false;

    slot->count -= count;

    // If stack is empty, clear the slot
    if (slot->count == 0) {
        memset(slot, 0, sizeof(ItemStack));
    }

    return true;
}
