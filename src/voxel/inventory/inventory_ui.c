/**
 * Inventory UI Rendering Implementation
 */

#include "voxel/inventory/inventory_ui.h"
#include "voxel/core/item.h"
#include "voxel/inventory/inventory_input.h"
#include <raylib.h>
#include <stdio.h>

// ============================================================================
// CONSTANTS
// ============================================================================

// Hotbar layout (bottom of screen)
#define HOTBAR_SLOT_SIZE 48
#define HOTBAR_GAP 4
#define HOTBAR_PADDING_BOTTOM 60

// Item icon size (centered in slot)
#define ITEM_ICON_SIZE 32

// Texture atlas layout
#define ATLAS_SIZE 256
#define TILE_SIZE 16
#define TILES_PER_ROW 16

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Draw a single inventory slot
 */
static void draw_slot(int x, int y, int size) {
    DrawRectangle(x, y, size, size, (Color){50, 50, 50, 200});
    DrawRectangleLines(x, y, size, size, (Color){80, 80, 80, 255});
}

/**
 * Draw a hotbar slot with optional selection highlight
 */
static void draw_hotbar_slot(int x, int y, int size, bool selected) {
    DrawRectangle(x, y, size, size, (Color){50, 50, 50, 200});
    if (selected) {
        DrawRectangleLines(x, y, size, size, WHITE);
        DrawRectangleLines(x + 1, y + 1, size - 2, size - 2, WHITE);
    } else {
        DrawRectangleLines(x, y, size, size, (Color){80, 80, 80, 255});
    }
}

/**
 * Draw item count in bottom-right of slot
 */
static void draw_item_count(int x, int y, int slot_size, uint8_t count) {
    if (count <= 1) return;

    const char* count_text = TextFormat("%d", count);
    int font_size = 12;
    int text_width = MeasureText(count_text, font_size);
    int text_x = x + slot_size - text_width - 2;
    int text_y = y + slot_size - font_size - 2;

    // Simple shadow for readability
    DrawText(count_text, text_x + 1, text_y + 1, font_size, BLACK);
    DrawText(count_text, text_x, text_y, font_size, WHITE);
}

// ============================================================================
// PUBLIC API
// ============================================================================

void inventory_ui_draw_item_icon(ItemType type, int x, int y, int size, Texture2D atlas) {
    if (type == ITEM_NONE) return;

    const ItemProperties* props = item_get_properties(type);

    // Calculate source rectangle from texture atlas
    float tile_uv_size = 1.0f / (float)TILES_PER_ROW;
    float padding = 0.001f;  // Prevent bleeding

    Rectangle source = {
        (float)(props->atlas_tile_x * TILE_SIZE),
        (float)(props->atlas_tile_y * TILE_SIZE),
        (float)TILE_SIZE,
        (float)TILE_SIZE
    };

    Rectangle dest = {
        (float)x,
        (float)y,
        (float)size,
        (float)size
    };

    DrawTexturePro(atlas, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
}

void inventory_ui_draw_hotbar(Inventory* inv, Texture2D atlas) {
    if (!inv) return;

    // Calculate hotbar position (centered at bottom of screen)
    int screen_width = 800;
    int total_width = (HOTBAR_SLOT_SIZE * HOTBAR_SIZE) + (HOTBAR_GAP * (HOTBAR_SIZE - 1));
    int start_x = (screen_width - total_width) / 2;
    int start_y = 600 - HOTBAR_PADDING_BOTTOM;

    // Draw all 9 hotbar slots
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        int x = start_x + i * (HOTBAR_SLOT_SIZE + HOTBAR_GAP);
        int y = start_y;
        bool selected = (i == inv->selected_hotbar_slot);

        draw_hotbar_slot(x, y, HOTBAR_SLOT_SIZE, selected);

        // Draw item icon if slot has an item
        ItemStack* slot = &inv->hotbar[i];
        if (slot->type != ITEM_NONE) {
            // Center icon in slot
            int icon_x = x + (HOTBAR_SLOT_SIZE - ITEM_ICON_SIZE) / 2;
            int icon_y = y + (HOTBAR_SLOT_SIZE - ITEM_ICON_SIZE) / 2;

            inventory_ui_draw_item_icon(slot->type, icon_x, icon_y, ITEM_ICON_SIZE, atlas);

            // Draw item count with outline
            draw_item_count(x, y, HOTBAR_SLOT_SIZE, slot->count);

            // Draw durability bar for tools
            const ItemProperties* props = item_get_properties(slot->type);
            if (props->is_tool && slot->max_durability > 0) {
                int bar_width = HOTBAR_SLOT_SIZE - 8;
                int bar_height = 3;
                int bar_x = x + 4;
                int bar_y = y + HOTBAR_SLOT_SIZE - bar_height - 4;

                // Calculate durability percentage
                float durability_percent = (float)slot->durability / (float)slot->max_durability;

                // Choose color based on durability
                Color bar_color = GREEN;
                if (durability_percent < 0.25f) {
                    bar_color = RED;
                } else if (durability_percent < 0.5f) {
                    bar_color = ORANGE;
                } else if (durability_percent < 0.75f) {
                    bar_color = YELLOW;
                }

                // Draw background
                DrawRectangle(bar_x, bar_y, bar_width, bar_height, (Color){40, 40, 40, 200});

                // Draw durability bar
                int filled_width = (int)(bar_width * durability_percent);
                DrawRectangle(bar_x, bar_y, filled_width, bar_height, bar_color);
            }
        }
    }
}

void inventory_ui_draw_full_screen(Inventory* inv, Texture2D atlas) {
    if (!inv) return;

    const int SLOT_SIZE = 40;
    const int SLOT_GAP = 2;

    // Draw semi-transparent background overlay
    DrawRectangle(0, 0, 800, 600, (Color){0, 0, 0, 150});

    // Draw inventory panel
    int panel_x = 150;
    int panel_y = 100;
    int panel_w = 500;
    int panel_h = 400;

    DrawRectangle(panel_x, panel_y, panel_w, panel_h, (Color){40, 40, 40, 240});
    DrawRectangleLines(panel_x, panel_y, panel_w, panel_h, (Color){150, 150, 150, 255});

    // Title
    DrawText("Inventory", panel_x + 20, panel_y + 10, 24, WHITE);

    // Section 1: Crafting Grid (3x3 + output)
    int craft_x = panel_x + 20;
    int craft_y = panel_y + 50;

    DrawText("Crafting", craft_x, craft_y - 20, 16, LIGHTGRAY);

    // Draw 3x3 crafting grid
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int x = craft_x + col * (SLOT_SIZE + SLOT_GAP);
            int y = craft_y + row * (SLOT_SIZE + SLOT_GAP);
            int slot_index = row * 3 + col;

            draw_slot(x, y, SLOT_SIZE);

            // Draw item if present
            ItemStack* slot = &inv->crafting_grid[slot_index];
            if (slot->type != ITEM_NONE) {
                int icon_x = x + (SLOT_SIZE - 28) / 2;
                int icon_y = y + (SLOT_SIZE - 28) / 2;
                inventory_ui_draw_item_icon(slot->type, icon_x, icon_y, 28, atlas);
                draw_item_count(x, y, SLOT_SIZE, slot->count);
            }
        }
    }

    // Draw output slot (arrow + output)
    int arrow_x = craft_x + 3 * (SLOT_SIZE + SLOT_GAP) + 10;
    int arrow_y = craft_y + SLOT_SIZE;
    DrawText("=>", arrow_x, arrow_y, 20, WHITE);

    int output_x = arrow_x + 30;
    int output_y = craft_y + SLOT_SIZE - SLOT_SIZE/2;
    draw_slot(output_x, output_y, SLOT_SIZE);

    ItemStack* output_slot = &inv->crafting_output[0];
    if (output_slot->type != ITEM_NONE) {
        int icon_x = output_x + (SLOT_SIZE - 28) / 2;
        int icon_y = output_y + (SLOT_SIZE - 28) / 2;
        inventory_ui_draw_item_icon(output_slot->type, icon_x, icon_y, 28, atlas);
        draw_item_count(output_x, output_y, SLOT_SIZE, output_slot->count);
    }

    // Section 2: Main Inventory (3 rows x 9 columns)
    int inv_x = panel_x + 20;
    int inv_y = panel_y + 200;

    DrawText("Storage", inv_x, inv_y - 20, 16, LIGHTGRAY);

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 9; col++) {
            int x = inv_x + col * (SLOT_SIZE + SLOT_GAP);
            int y = inv_y + row * (SLOT_SIZE + SLOT_GAP);
            int slot_index = row * 9 + col;

            draw_slot(x, y, SLOT_SIZE);

            // Draw item if present
            ItemStack* slot = &inv->main_inventory[slot_index];
            if (slot->type != ITEM_NONE) {
                int icon_x = x + (SLOT_SIZE - 28) / 2;
                int icon_y = y + (SLOT_SIZE - 28) / 2;
                inventory_ui_draw_item_icon(slot->type, icon_x, icon_y, 28, atlas);
                draw_item_count(x, y, SLOT_SIZE, slot->count);
            }
        }
    }

    // Section 3: Hotbar Mirror (1 row x 9 columns)
    int hotbar_x = inv_x;
    int hotbar_y = inv_y + 3 * (SLOT_SIZE + SLOT_GAP) + 10;

    for (int i = 0; i < 9; i++) {
        int x = hotbar_x + i * (SLOT_SIZE + SLOT_GAP);
        int y = hotbar_y;

        draw_slot(x, y, SLOT_SIZE);

        // Draw item if present
        ItemStack* slot = &inv->hotbar[i];
        if (slot->type != ITEM_NONE) {
            int icon_x = x + (SLOT_SIZE - 28) / 2;
            int icon_y = y + (SLOT_SIZE - 28) / 2;
            inventory_ui_draw_item_icon(slot->type, icon_x, icon_y, 28, atlas);
            draw_item_count(x, y, SLOT_SIZE, slot->count);
        }
    }

}

void inventory_ui_draw_tooltip(Inventory* inv, int mouse_x, int mouse_y) {
    if (!inv) return;

    // Get the slot being hovered
    InventorySection section;
    int slot_index = inventory_input_get_clicked_slot(mouse_x, mouse_y, &section);

    if (slot_index == -1 || section == SECTION_NONE) {
        return;  // No slot hovered
    }

    // Get the item in the hovered slot
    ItemStack* slot = NULL;

    switch (section) {
        case SECTION_CRAFTING_GRID:
            if (slot_index >= 0 && slot_index < 9) {
                slot = &inv->crafting_grid[slot_index];
            }
            break;
        case SECTION_CRAFTING_OUTPUT:
            slot = &inv->crafting_output[0];
            break;
        case SECTION_MAIN_INVENTORY:
            if (slot_index >= 0 && slot_index < 27) {
                slot = &inv->main_inventory[slot_index];
            }
            break;
        case SECTION_HOTBAR:
            if (slot_index >= 0 && slot_index < 9) {
                slot = &inv->hotbar[slot_index];
            }
            break;
        default:
            return;
    }

    if (!slot || slot->type == ITEM_NONE) {
        return;  // Empty slot
    }

    // Get item name
    const char* item_name = item_get_name(slot->type);

    // Measure text size for background
    int font_size = 16;
    int text_width = MeasureText(item_name, font_size);
    int padding = 6;

    // Position tooltip near cursor (offset to avoid covering item)
    int tooltip_x = mouse_x + 12;
    int tooltip_y = mouse_y + 12;

    // Keep tooltip on screen
    if (tooltip_x + text_width + padding * 2 > 800) {
        tooltip_x = mouse_x - text_width - padding * 2 - 12;
    }
    if (tooltip_y + font_size + padding * 2 > 600) {
        tooltip_y = mouse_y - font_size - padding * 2 - 12;
    }

    // Draw tooltip background
    DrawRectangle(tooltip_x, tooltip_y, text_width + padding * 2, font_size + padding * 2,
                   (Color){40, 40, 40, 240});
    DrawRectangleLines(tooltip_x, tooltip_y, text_width + padding * 2, font_size + padding * 2,
                       (Color){150, 150, 150, 255});

    // Draw item name
    DrawText(item_name, tooltip_x + padding, tooltip_y + padding, font_size, WHITE);
}
