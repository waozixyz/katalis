/**
 * Inventory Input System Implementation
 */

#include "voxel/inventory/inventory_input.h"
#include "voxel/core/item.h"
#include "voxel/inventory/crafting.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// CONSTANTS (must match inventory_ui.c layout)
// ============================================================================

#define PANEL_X 50   // Adjusted to make room for crafting guide sidebar
#define PANEL_Y 100

#define SLOT_SIZE 40
#define SLOT_GAP 2

// Crafting section
#define CRAFT_X (PANEL_X + 20)
#define CRAFT_Y (PANEL_Y + 50)
#define CRAFT_OUTPUT_X (CRAFT_X + 3 * (SLOT_SIZE + SLOT_GAP) + 40)
#define CRAFT_OUTPUT_Y (CRAFT_Y + SLOT_SIZE - SLOT_SIZE / 2)

// Main inventory section
#define INV_X (PANEL_X + 20)
#define INV_Y (PANEL_Y + 200)

// Hotbar section
#define HOTBAR_X INV_X
#define HOTBAR_Y (INV_Y + 3 * (SLOT_SIZE + SLOT_GAP) + 10)

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Check if mouse is within a rectangular slot area
 */
static bool is_mouse_in_slot(int mouse_x, int mouse_y, int slot_x, int slot_y) {
    return (mouse_x >= slot_x && mouse_x < slot_x + SLOT_SIZE &&
            mouse_y >= slot_y && mouse_y < slot_y + SLOT_SIZE);
}

/**
 * Get pointer to a slot by section and index
 */
static ItemStack* get_slot_pointer(Inventory* inv, InventorySection section, int index) {
    if (!inv) return NULL;

    switch (section) {
        case SECTION_CRAFTING_GRID:
            if (index >= 0 && index < 9) return &inv->crafting_grid[index];
            break;
        case SECTION_CRAFTING_OUTPUT:
            if (index == 0) return &inv->crafting_output[0];
            break;
        case SECTION_MAIN_INVENTORY:
            if (index >= 0 && index < 27) return &inv->main_inventory[index];
            break;
        case SECTION_HOTBAR:
            if (index >= 0 && index < 9) return &inv->hotbar[index];
            break;
        default:
            return NULL;
    }

    return NULL;
}

// ============================================================================
// PUBLIC API
// ============================================================================

int inventory_input_get_clicked_slot(int mouse_x, int mouse_y, InventorySection* section) {
    *section = SECTION_NONE;

    // Check crafting grid (3x3)
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int x = CRAFT_X + col * (SLOT_SIZE + SLOT_GAP);
            int y = CRAFT_Y + row * (SLOT_SIZE + SLOT_GAP);

            if (is_mouse_in_slot(mouse_x, mouse_y, x, y)) {
                *section = SECTION_CRAFTING_GRID;
                return row * 3 + col;
            }
        }
    }

    // Check crafting output slot
    if (is_mouse_in_slot(mouse_x, mouse_y, CRAFT_OUTPUT_X, CRAFT_OUTPUT_Y)) {
        *section = SECTION_CRAFTING_OUTPUT;
        return 0;
    }

    // Check main inventory (3x9)
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 9; col++) {
            int x = INV_X + col * (SLOT_SIZE + SLOT_GAP);
            int y = INV_Y + row * (SLOT_SIZE + SLOT_GAP);

            if (is_mouse_in_slot(mouse_x, mouse_y, x, y)) {
                *section = SECTION_MAIN_INVENTORY;
                return row * 9 + col;
            }
        }
    }

    // Check hotbar (1x9)
    for (int i = 0; i < 9; i++) {
        int x = HOTBAR_X + i * (SLOT_SIZE + SLOT_GAP);
        int y = HOTBAR_Y;

        if (is_mouse_in_slot(mouse_x, mouse_y, x, y)) {
            *section = SECTION_HOTBAR;
            return i;
        }
    }

    return -1;
}

void inventory_input_handle_left_click(Inventory* inv, int mouse_x, int mouse_y) {
    if (!inv) return;

    InventorySection section;
    int slot_index = inventory_input_get_clicked_slot(mouse_x, mouse_y, &section);

    if (slot_index == -1 || section == SECTION_NONE) {
        return;  // No slot clicked
    }

    // Special case: clicking crafting output
    if (section == SECTION_CRAFTING_OUTPUT) {
        ItemStack* output_slot = &inv->crafting_output[0];

        // Can only pick up if output has an item
        if (output_slot->type == ITEM_NONE) {
            return;
        }

        // Can only pick up if hand is empty or holding same item type
        if (inv->is_holding_item && inv->held_item.type != output_slot->type) {
            return;  // Can't swap with crafting output
        }

        // Pick up crafted item
        if (!inv->is_holding_item) {
            // Empty hand - pick up output
            inv->held_item = *output_slot;
            inv->is_holding_item = true;
        } else {
            // Holding same item - try to stack
            const ItemProperties* props = item_get_properties(output_slot->type);
            uint8_t space = props->max_stack_size - inv->held_item.count;

            if (space < output_slot->count) {
                return;  // Not enough space in held stack
            }

            inv->held_item.count += output_slot->count;
        }

        // Consume crafting inputs
        crafting_try_craft(inv);

        return;
    }

    ItemStack* clicked_slot = get_slot_pointer(inv, section, slot_index);
    if (!clicked_slot) return;

    // Case 1: Empty hand, empty slot - do nothing
    if (!inv->is_holding_item && clicked_slot->type == ITEM_NONE) {
        return;
    }

    // Case 2: Empty hand, item in slot - pick up entire stack
    if (!inv->is_holding_item && clicked_slot->type != ITEM_NONE) {
        inv->held_item = *clicked_slot;
        inv->is_holding_item = true;
        clicked_slot->type = ITEM_NONE;
        clicked_slot->count = 0;
        clicked_slot->durability = 0;
        clicked_slot->max_durability = 0;

        if (section == SECTION_CRAFTING_GRID) {
            crafting_update_output(inv);
        }
        return;
    }

    // Case 3: Holding item, empty slot - place entire stack
    if (inv->is_holding_item && clicked_slot->type == ITEM_NONE) {
        *clicked_slot = inv->held_item;
        inv->held_item.type = ITEM_NONE;
        inv->held_item.count = 0;
        inv->held_item.durability = 0;
        inv->held_item.max_durability = 0;
        inv->is_holding_item = false;

        if (section == SECTION_CRAFTING_GRID) {
            crafting_update_output(inv);
        }
        return;
    }

    // Case 4: Holding item, item in slot - swap stacks
    if (inv->is_holding_item && clicked_slot->type != ITEM_NONE) {
        // Same item type - try to merge
        if (inv->held_item.type == clicked_slot->type) {
            const ItemProperties* props = item_get_properties(clicked_slot->type);
            uint8_t space_in_slot = props->max_stack_size - clicked_slot->count;

            if (space_in_slot > 0) {
                uint8_t transfer_amount = (inv->held_item.count <= space_in_slot)
                    ? inv->held_item.count
                    : space_in_slot;

                clicked_slot->count += transfer_amount;
                inv->held_item.count -= transfer_amount;

                if (inv->held_item.count == 0) {
                    inv->held_item.type = ITEM_NONE;
                    inv->held_item.durability = 0;
                    inv->held_item.max_durability = 0;
                    inv->is_holding_item = false;
                }

                if (section == SECTION_CRAFTING_GRID) {
                    crafting_update_output(inv);
                }
                return;
            }
        }

        // Different types or no space - swap stacks
        ItemStack temp = *clicked_slot;
        *clicked_slot = inv->held_item;
        inv->held_item = temp;

        if (section == SECTION_CRAFTING_GRID) {
            crafting_update_output(inv);
        }
        return;
    }
}

void inventory_input_handle_right_click(Inventory* inv, int mouse_x, int mouse_y) {
    if (!inv) return;

    InventorySection section;
    int slot_index = inventory_input_get_clicked_slot(mouse_x, mouse_y, &section);

    if (slot_index == -1 || section == SECTION_NONE) {
        return;
    }

    // Crafting output not supported for right-click
    if (section == SECTION_CRAFTING_OUTPUT) {
        return;
    }

    ItemStack* clicked_slot = get_slot_pointer(inv, section, slot_index);
    if (!clicked_slot) return;

    // Case 1: Empty hand, item in slot - pick up half (rounded up)
    if (!inv->is_holding_item && clicked_slot->type != ITEM_NONE) {
        uint8_t half = (clicked_slot->count + 1) / 2;  // Round up

        inv->held_item.type = clicked_slot->type;
        inv->held_item.count = half;
        inv->held_item.durability = clicked_slot->durability;
        inv->held_item.max_durability = clicked_slot->max_durability;
        inv->is_holding_item = true;

        clicked_slot->count -= half;
        if (clicked_slot->count == 0) {
            clicked_slot->type = ITEM_NONE;
            clicked_slot->durability = 0;
            clicked_slot->max_durability = 0;
        }

        if (section == SECTION_CRAFTING_GRID) {
            crafting_update_output(inv);
        }
        return;
    }

    // Case 2: Holding item, empty slot - place 1 item
    if (inv->is_holding_item && clicked_slot->type == ITEM_NONE) {
        clicked_slot->type = inv->held_item.type;
        clicked_slot->count = 1;
        clicked_slot->durability = inv->held_item.durability;
        clicked_slot->max_durability = inv->held_item.max_durability;

        inv->held_item.count--;
        if (inv->held_item.count == 0) {
            inv->held_item.type = ITEM_NONE;
            inv->held_item.durability = 0;
            inv->held_item.max_durability = 0;
            inv->is_holding_item = false;
        }

        if (section == SECTION_CRAFTING_GRID) {
            crafting_update_output(inv);
        }
        return;
    }

    // Case 3: Holding item, same item in slot - place 1 item
    if (inv->is_holding_item &&
        clicked_slot->type == inv->held_item.type &&
        clicked_slot->type != ITEM_NONE) {

        const ItemProperties* props = item_get_properties(clicked_slot->type);

        if (clicked_slot->count < props->max_stack_size) {
            clicked_slot->count++;
            inv->held_item.count--;

            if (inv->held_item.count == 0) {
                inv->held_item.type = ITEM_NONE;
                inv->held_item.durability = 0;
                inv->held_item.max_durability = 0;
                inv->is_holding_item = false;
            }

            if (section == SECTION_CRAFTING_GRID) {
                crafting_update_output(inv);
            }
        }
        return;
    }
}

void inventory_input_handle_shift_click(Inventory* inv, int mouse_x, int mouse_y) {
    if (!inv) return;

    InventorySection section;
    int slot_index = inventory_input_get_clicked_slot(mouse_x, mouse_y, &section);

    printf("[INPUT] Shift-click at (%d, %d) -> section=%d, slot=%d\n", mouse_x, mouse_y, section, slot_index);

    if (slot_index == -1 || section == SECTION_NONE) {
        return;
    }

    // Special case: Shift-click on crafting output = CRAFT ALL
    if (section == SECTION_CRAFTING_OUTPUT) {
        printf("[INPUT] Detected crafting output shift-click!\n");
        ItemStack* output_slot = &inv->crafting_output[0];
        if (output_slot->type == ITEM_NONE) {
            printf("[INPUT] Output slot is empty, nothing to craft\n");
            return;
        }

        printf("[INPUT] Output has %d x item type %d\n", output_slot->count, output_slot->type);

        // Craft all possible items
        ItemStack crafted = crafting_craft_all(inv);
        printf("[INPUT] crafting_craft_all returned %d x item type %d\n", crafted.count, crafted.type);
        if (crafted.type == ITEM_NONE || crafted.count == 0) {
            return;
        }

        // Try to add to hotbar first, then main inventory
        const ItemProperties* props = item_get_properties(crafted.type);
        uint8_t remaining = crafted.count;

        // Phase 1: Stack with existing items in hotbar
        for (int i = 0; i < 9 && remaining > 0; i++) {
            if (inv->hotbar[i].type == crafted.type) {
                uint8_t space = props->max_stack_size - inv->hotbar[i].count;
                uint8_t transfer = (remaining <= space) ? remaining : space;
                inv->hotbar[i].count += transfer;
                remaining -= transfer;
            }
        }

        // Phase 2: Stack with existing items in main inventory
        for (int i = 0; i < 27 && remaining > 0; i++) {
            if (inv->main_inventory[i].type == crafted.type) {
                uint8_t space = props->max_stack_size - inv->main_inventory[i].count;
                uint8_t transfer = (remaining <= space) ? remaining : space;
                inv->main_inventory[i].count += transfer;
                remaining -= transfer;
            }
        }

        // Phase 3: Create new stacks in empty hotbar slots
        for (int i = 0; i < 9 && remaining > 0; i++) {
            if (inv->hotbar[i].type == ITEM_NONE) {
                uint8_t transfer = (remaining <= props->max_stack_size)
                    ? remaining : props->max_stack_size;
                inv->hotbar[i].type = crafted.type;
                inv->hotbar[i].count = transfer;
                inv->hotbar[i].durability = crafted.durability;
                inv->hotbar[i].max_durability = crafted.max_durability;
                remaining -= transfer;
            }
        }

        // Phase 4: Create new stacks in empty main inventory slots
        for (int i = 0; i < 27 && remaining > 0; i++) {
            if (inv->main_inventory[i].type == ITEM_NONE) {
                uint8_t transfer = (remaining <= props->max_stack_size)
                    ? remaining : props->max_stack_size;
                inv->main_inventory[i].type = crafted.type;
                inv->main_inventory[i].count = transfer;
                inv->main_inventory[i].durability = crafted.durability;
                inv->main_inventory[i].max_durability = crafted.max_durability;
                remaining -= transfer;
            }
        }

        printf("[INPUT] Shift-click craft all: got %d items, %d couldn't fit\n",
               crafted.count - remaining, remaining);
        return;
    }

    ItemStack* clicked_slot = get_slot_pointer(inv, section, slot_index);
    if (!clicked_slot || clicked_slot->type == ITEM_NONE) {
        return;
    }

    // Quick transfer logic:
    // - Hotbar → Main Inventory
    // - Main Inventory → Hotbar
    // - Crafting Grid → Try hotbar first, then main inventory

    ItemType item_type = clicked_slot->type;
    uint8_t count = clicked_slot->count;

    if (section == SECTION_HOTBAR) {
        // Try to add to main inventory
        if (inventory_add_item(inv, item_type, count)) {
            clicked_slot->type = ITEM_NONE;
            clicked_slot->count = 0;
            clicked_slot->durability = 0;
            clicked_slot->max_durability = 0;
            printf("[INPUT] Quick transfer: Hotbar → Main Inventory\n");
        }
    } else if (section == SECTION_MAIN_INVENTORY || section == SECTION_CRAFTING_GRID) {
        // Try to add to hotbar first, then main inventory
        // We'll manually try to add to hotbar slots
        const ItemProperties* props = item_get_properties(item_type);
        uint8_t remaining = count;

        // Phase 1: Stack with existing items in hotbar
        for (int i = 0; i < 9 && remaining > 0; i++) {
            if (inv->hotbar[i].type == item_type) {
                uint8_t space = props->max_stack_size - inv->hotbar[i].count;
                uint8_t transfer = (remaining <= space) ? remaining : space;
                inv->hotbar[i].count += transfer;
                remaining -= transfer;
            }
        }

        // Phase 2: Create new stacks in empty hotbar slots
        for (int i = 0; i < 9 && remaining > 0; i++) {
            if (inv->hotbar[i].type == ITEM_NONE) {
                uint8_t transfer = (remaining <= props->max_stack_size)
                    ? remaining
                    : props->max_stack_size;

                inv->hotbar[i].type = item_type;
                inv->hotbar[i].count = transfer;
                inv->hotbar[i].durability = clicked_slot->durability;
                inv->hotbar[i].max_durability = clicked_slot->max_durability;
                remaining -= transfer;
            }
        }

        // Update clicked slot
        if (remaining == 0) {
            clicked_slot->type = ITEM_NONE;
            clicked_slot->count = 0;
            clicked_slot->durability = 0;
            clicked_slot->max_durability = 0;
            printf("[INPUT] Quick transfer: Inventory → Hotbar\n");
        } else {
            clicked_slot->count = remaining;
            printf("[INPUT] Quick transfer partial: %d items remaining\n", remaining);
        }
    }
}
