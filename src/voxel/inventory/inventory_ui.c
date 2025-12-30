/**
 * Inventory UI Rendering Implementation
 */

#include "voxel/inventory/inventory_ui.h"
#include "voxel/inventory/crafting.h"
#include "voxel/core/item.h"
#include "voxel/inventory/inventory_input.h"
#include "voxel/world/chest.h"
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

// Crafting guide sidebar constants
#define GUIDE_X 440
#define GUIDE_Y 100
#define GUIDE_WIDTH 180
#define GUIDE_HEIGHT 400
#define RECIPE_ENTRY_HEIGHT 24
#define RECIPE_ICON_SIZE 16

// ============================================================================
// CRAFTING GUIDE STATE
// ============================================================================

static int recipe_scroll_offset = 0;

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

/**
 * Draw a mini item icon with color tint (for crafting guide)
 */
static void draw_mini_item_icon(ItemType type, int x, int y, int size, Texture2D atlas, Color tint) {
    if (type == ITEM_NONE) return;

    const ItemProperties* props = item_get_properties(type);

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

    DrawTexturePro(atlas, source, dest, (Vector2){0, 0}, 0.0f, tint);
}

/**
 * Draw a single recipe entry in the crafting guide
 */
static void draw_recipe_entry(const CraftingRecipe* recipe, int x, int y,
                               Inventory* inv, Texture2D atlas) {
    if (!recipe) return;

    // Collect unique ingredients (up to 3 for display)
    ItemType ingredients[3] = {ITEM_NONE, ITEM_NONE, ITEM_NONE};
    int ingredient_count = 0;

    for (int i = 0; i < 9 && ingredient_count < 3; i++) {
        if (recipe->inputs[i] != ITEM_NONE) {
            // Check if already in list
            bool found = false;
            for (int j = 0; j < ingredient_count; j++) {
                if (ingredients[j] == recipe->inputs[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                ingredients[ingredient_count++] = recipe->inputs[i];
            }
        }
    }

    // Check if craftable (has all ingredients in inventory)
    bool can_craft = crafting_can_craft_recipe(inv, recipe);
    Color tint = can_craft ? WHITE : (Color){100, 100, 100, 180};
    Color text_color = can_craft ? LIGHTGRAY : (Color){80, 80, 80, 180};

    // Draw ingredient icons
    int icon_x = x;
    for (int i = 0; i < ingredient_count; i++) {
        draw_mini_item_icon(ingredients[i], icon_x, y + 4, RECIPE_ICON_SIZE, atlas, tint);
        icon_x += RECIPE_ICON_SIZE + 2;
    }

    // Draw arrow
    DrawText("->", icon_x + 2, y + 6, 10, text_color);
    icon_x += 20;

    // Draw output icon
    draw_mini_item_icon(recipe->output, icon_x, y + 4, RECIPE_ICON_SIZE, atlas, tint);

    // Draw output count if > 1
    if (recipe->output_count > 1) {
        DrawText(TextFormat("x%d", recipe->output_count),
                 icon_x + RECIPE_ICON_SIZE + 2, y + 7, 10, text_color);
    }
}

/**
 * Draw the crafting guide sidebar
 */
static void draw_crafting_guide(Inventory* inv, Texture2D atlas) {
    // Draw sidebar panel background
    DrawRectangle(GUIDE_X, GUIDE_Y, GUIDE_WIDTH, GUIDE_HEIGHT, (Color){40, 40, 40, 240});
    DrawRectangleLines(GUIDE_X, GUIDE_Y, GUIDE_WIDTH, GUIDE_HEIGHT, (Color){150, 150, 150, 255});

    // Draw "Recipes" title
    DrawText("Recipes", GUIDE_X + 10, GUIDE_Y + 10, 18, WHITE);

    // Divider line under title
    DrawLine(GUIDE_X + 10, GUIDE_Y + 32, GUIDE_X + GUIDE_WIDTH - 10, GUIDE_Y + 32,
             (Color){100, 100, 100, 255});

    // Get recipe list info
    int recipe_count = crafting_get_recipe_count();
    int visible_count = (GUIDE_HEIGHT - 50) / RECIPE_ENTRY_HEIGHT;
    int start_y = GUIDE_Y + 40;

    // Clamp scroll offset
    int max_scroll = recipe_count - visible_count;
    if (max_scroll < 0) max_scroll = 0;
    if (recipe_scroll_offset > max_scroll) recipe_scroll_offset = max_scroll;
    if (recipe_scroll_offset < 0) recipe_scroll_offset = 0;

    // Draw recipe entries
    for (int i = 0; i < visible_count && i + recipe_scroll_offset < recipe_count; i++) {
        int recipe_idx = i + recipe_scroll_offset;
        const CraftingRecipe* recipe = crafting_get_recipe(recipe_idx);

        if (recipe) {
            draw_recipe_entry(recipe, GUIDE_X + 8, start_y + i * RECIPE_ENTRY_HEIGHT,
                              inv, atlas);
        }
    }

    // Draw scroll indicators if needed
    if (recipe_scroll_offset > 0) {
        DrawText("^", GUIDE_X + GUIDE_WIDTH - 18, GUIDE_Y + 38, 14, GRAY);
    }
    if (recipe_scroll_offset + visible_count < recipe_count) {
        DrawText("v", GUIDE_X + GUIDE_WIDTH - 18, GUIDE_Y + GUIDE_HEIGHT - 18, 14, GRAY);
    }

    // Show hint at bottom
    DrawText("Scroll to see more", GUIDE_X + 10, GUIDE_Y + GUIDE_HEIGHT - 18, 10, DARKGRAY);
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

    // Draw inventory panel (adjusted to make room for crafting guide)
    int panel_x = 50;
    int panel_y = 100;
    int panel_w = 380;
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

    // Draw crafting guide sidebar
    draw_crafting_guide(inv, atlas);
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

// ============================================================================
// CHEST UI
// ============================================================================

// Chest UI layout constants
#define CHEST_PANEL_X 150
#define CHEST_PANEL_Y 80
#define CHEST_SLOT_SIZE 40
#define CHEST_SLOT_GAP 2

void inventory_ui_draw_chest(ChestData* chest, Inventory* inv, Texture2D atlas) {
    if (!chest || !inv) return;

    const int SLOT_SIZE = CHEST_SLOT_SIZE;
    const int SLOT_GAP = CHEST_SLOT_GAP;

    // Draw semi-transparent background overlay
    DrawRectangle(0, 0, 800, 600, (Color){0, 0, 0, 150});

    // Calculate panel dimensions
    int panel_x = CHEST_PANEL_X;
    int panel_y = CHEST_PANEL_Y;
    int panel_w = 500;
    int panel_h = 450;

    // Draw panel background
    DrawRectangle(panel_x, panel_y, panel_w, panel_h, (Color){40, 40, 40, 240});
    DrawRectangleLines(panel_x, panel_y, panel_w, panel_h, (Color){150, 150, 150, 255});

    // Title
    DrawText("Chest", panel_x + 20, panel_y + 10, 24, WHITE);

    // Section 1: Chest contents (3 rows x 9 columns = 27 slots)
    int chest_x = panel_x + 20;
    int chest_y = panel_y + 50;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 9; col++) {
            int x = chest_x + col * (SLOT_SIZE + SLOT_GAP);
            int y = chest_y + row * (SLOT_SIZE + SLOT_GAP);
            int slot_index = row * 9 + col;

            draw_slot(x, y, SLOT_SIZE);

            // Draw item if present
            if (slot_index < CHEST_SLOTS) {
                ItemStack* slot = &chest->slots[slot_index];
                if (slot->type != ITEM_NONE && slot->count > 0) {
                    int icon_x = x + (SLOT_SIZE - 28) / 2;
                    int icon_y = y + (SLOT_SIZE - 28) / 2;
                    inventory_ui_draw_item_icon(slot->type, icon_x, icon_y, 28, atlas);
                    draw_item_count(x, y, SLOT_SIZE, slot->count);
                }
            }
        }
    }

    // Divider line
    int div_y = chest_y + 3 * (SLOT_SIZE + SLOT_GAP) + 10;
    DrawLine(panel_x + 20, div_y, panel_x + panel_w - 20, div_y, (Color){100, 100, 100, 255});

    // Section 2: Player main inventory (3 rows x 9 columns)
    int inv_x = panel_x + 20;
    int inv_y = div_y + 20;

    DrawText("Inventory", inv_x, inv_y - 15, 14, LIGHTGRAY);

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 9; col++) {
            int x = inv_x + col * (SLOT_SIZE + SLOT_GAP);
            int y = inv_y + row * (SLOT_SIZE + SLOT_GAP);
            int slot_index = row * 9 + col;

            draw_slot(x, y, SLOT_SIZE);

            // Draw item if present
            ItemStack* slot = &inv->main_inventory[slot_index];
            if (slot->type != ITEM_NONE && slot->count > 0) {
                int icon_x = x + (SLOT_SIZE - 28) / 2;
                int icon_y = y + (SLOT_SIZE - 28) / 2;
                inventory_ui_draw_item_icon(slot->type, icon_x, icon_y, 28, atlas);
                draw_item_count(x, y, SLOT_SIZE, slot->count);
            }
        }
    }

    // Section 3: Player hotbar (1 row x 9 columns)
    int hotbar_x = inv_x;
    int hotbar_y = inv_y + 3 * (SLOT_SIZE + SLOT_GAP) + 10;

    for (int i = 0; i < 9; i++) {
        int x = hotbar_x + i * (SLOT_SIZE + SLOT_GAP);
        int y = hotbar_y;

        draw_slot(x, y, SLOT_SIZE);

        // Draw item if present
        ItemStack* slot = &inv->hotbar[i];
        if (slot->type != ITEM_NONE && slot->count > 0) {
            int icon_x = x + (SLOT_SIZE - 28) / 2;
            int icon_y = y + (SLOT_SIZE - 28) / 2;
            inventory_ui_draw_item_icon(slot->type, icon_x, icon_y, 28, atlas);
            draw_item_count(x, y, SLOT_SIZE, slot->count);
        }
    }

    // Instructions
    DrawText("Click items to transfer. Press E or ESC to close.", panel_x + 20, panel_y + panel_h - 25, 14, GRAY);
}

void inventory_ui_handle_chest_click(ChestData* chest, Inventory* inv, int mouse_x, int mouse_y) {
    if (!chest || !inv) return;

    const int SLOT_SIZE = CHEST_SLOT_SIZE;
    const int SLOT_GAP = CHEST_SLOT_GAP;
    int panel_x = CHEST_PANEL_X;
    int panel_y = CHEST_PANEL_Y;

    // Check chest slots (3 rows x 9 columns)
    int chest_x = panel_x + 20;
    int chest_y = panel_y + 50;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 9; col++) {
            int x = chest_x + col * (SLOT_SIZE + SLOT_GAP);
            int y = chest_y + row * (SLOT_SIZE + SLOT_GAP);
            int slot_index = row * 9 + col;

            if (mouse_x >= x && mouse_x < x + SLOT_SIZE &&
                mouse_y >= y && mouse_y < y + SLOT_SIZE) {
                // Clicked on chest slot - transfer to player inventory
                if (slot_index < CHEST_SLOTS && chest->slots[slot_index].type != ITEM_NONE) {
                    ItemStack item = chest->slots[slot_index];

                    // Try to add to player inventory
                    if (inventory_add_item(inv, item.type, item.count)) {
                        // Remove from chest
                        chest->slots[slot_index] = (ItemStack){ITEM_NONE, 0, 0, 0};
                        printf("[CHEST] Took %d %s\n", item.count, item_get_name(item.type));
                    }
                }
                return;
            }
        }
    }

    // Check player inventory slots
    int div_y = chest_y + 3 * (SLOT_SIZE + SLOT_GAP) + 10;
    int inv_x = panel_x + 20;
    int inv_y = div_y + 20;

    // Main inventory (3 rows x 9)
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 9; col++) {
            int x = inv_x + col * (SLOT_SIZE + SLOT_GAP);
            int y = inv_y + row * (SLOT_SIZE + SLOT_GAP);
            int slot_index = row * 9 + col;

            if (mouse_x >= x && mouse_x < x + SLOT_SIZE &&
                mouse_y >= y && mouse_y < y + SLOT_SIZE) {
                // Clicked on inventory slot - transfer to chest
                if (inv->main_inventory[slot_index].type != ITEM_NONE) {
                    ItemStack item = inv->main_inventory[slot_index];

                    // Try to add to chest
                    if (chest_add_item(chest, item)) {
                        // Remove from inventory
                        inv->main_inventory[slot_index] = (ItemStack){ITEM_NONE, 0, 0, 0};
                        printf("[CHEST] Stored %d %s\n", item.count, item_get_name(item.type));
                    }
                }
                return;
            }
        }
    }

    // Hotbar (1 row x 9)
    int hotbar_x = inv_x;
    int hotbar_y = inv_y + 3 * (SLOT_SIZE + SLOT_GAP) + 10;

    for (int i = 0; i < 9; i++) {
        int x = hotbar_x + i * (SLOT_SIZE + SLOT_GAP);
        int y = hotbar_y;

        if (mouse_x >= x && mouse_x < x + SLOT_SIZE &&
            mouse_y >= y && mouse_y < y + SLOT_SIZE) {
            // Clicked on hotbar slot - transfer to chest
            if (inv->hotbar[i].type != ITEM_NONE) {
                ItemStack item = inv->hotbar[i];

                // Try to add to chest
                if (chest_add_item(chest, item)) {
                    // Remove from hotbar
                    inv->hotbar[i] = (ItemStack){ITEM_NONE, 0, 0, 0};
                    printf("[CHEST] Stored %d %s\n", item.count, item_get_name(item.type));
                }
            }
            return;
        }
    }
}

// ============================================================================
// CRAFTING GUIDE SCROLL HANDLING
// ============================================================================

void inventory_ui_handle_scroll(int scroll_delta) {
    int recipe_count = crafting_get_recipe_count();
    int visible_count = (GUIDE_HEIGHT - 50) / RECIPE_ENTRY_HEIGHT;
    int max_scroll = recipe_count - visible_count;
    if (max_scroll < 0) max_scroll = 0;

    // Scroll up = positive delta, scroll down = negative delta
    recipe_scroll_offset -= scroll_delta;

    // Clamp to valid range
    if (recipe_scroll_offset < 0) recipe_scroll_offset = 0;
    if (recipe_scroll_offset > max_scroll) recipe_scroll_offset = max_scroll;
}
