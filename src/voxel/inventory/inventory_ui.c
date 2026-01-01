/**
 * Inventory UI Rendering Implementation
 */

#include "voxel/inventory/inventory_ui.h"
#include "voxel/inventory/crafting.h"
#include "voxel/core/item.h"
#include "voxel/core/texture_atlas.h"
#include "voxel/inventory/inventory_input.h"
#include "voxel/world/chest.h"
#include "voxel/player/player.h"
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// ============================================================================
// CONSTANTS
// ============================================================================

// Hotbar layout (bottom of screen)
#define HOTBAR_SLOT_SIZE 48
#define HOTBAR_GAP 4
#define HOTBAR_PADDING_BOTTOM 60

// Item icon size (centered in slot)
#define ITEM_ICON_SIZE 32

// Crafting guide sidebar constants (Luanti-style)
#define GUIDE_X 520
#define GUIDE_Y 100
#define GUIDE_WIDTH 220
#define GUIDE_HEIGHT 480

// Item browser grid
#define BROWSER_ITEM_SIZE 28
#define BROWSER_COLS 6
#define BROWSER_ROWS 5
#define ITEMS_PER_PAGE (BROWSER_COLS * BROWSER_ROWS)  // 30 items

// Recipe preview
#define PREVIEW_SLOT_SIZE 24
#define PREVIEW_GAP 2

// ============================================================================
// CRAFTING GUIDE STATE
// ============================================================================

static int guide_current_page = 0;
static ItemType guide_selected_item = ITEM_NONE;
static char guide_search_text[64] = "";
static bool guide_search_active = false;
static ItemType guide_filtered_items[ITEM_COUNT];
static int guide_filtered_count = 0;
static bool guide_initialized = false;

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
 * Case-insensitive substring search
 */
static bool str_contains_ci(const char* haystack, const char* needle) {
    if (!haystack || !needle || needle[0] == '\0') return true;

    size_t needle_len = strlen(needle);
    size_t haystack_len = strlen(haystack);

    if (needle_len > haystack_len) return false;

    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        bool match = true;
        for (size_t j = 0; j < needle_len; j++) {
            char h = haystack[i + j];
            char n = needle[j];
            // Simple lowercase conversion for ASCII
            if (h >= 'A' && h <= 'Z') h += 32;
            if (n >= 'A' && n <= 'Z') n += 32;
            if (h != n) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

/**
 * Update filtered items list based on search text
 */
static void guide_update_filter(void) {
    guide_filtered_count = 0;

    for (int i = 1; i < ITEM_COUNT; i++) {  // Skip ITEM_NONE
        const char* name = item_get_name(i);
        if (str_contains_ci(name, guide_search_text)) {
            guide_filtered_items[guide_filtered_count++] = (ItemType)i;
        }
    }

    // Reset to first page when filter changes
    guide_current_page = 0;
}

/**
 * Initialize the crafting guide (call once)
 */
static void guide_init_if_needed(void) {
    if (!guide_initialized) {
        guide_update_filter();
        guide_initialized = true;
    }
}

/**
 * Get total pages for current filter
 */
static int guide_get_total_pages(void) {
    return (guide_filtered_count + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
}

/**
 * Draw the search box
 */
static void draw_search_box(int x, int y, int width) {
    // Background
    Color bg = guide_search_active ? (Color){60, 60, 80, 255} : (Color){50, 50, 50, 255};
    DrawRectangle(x, y, width, 20, bg);
    DrawRectangleLines(x, y, width, 20, (Color){100, 100, 100, 255});

    // Search text or placeholder
    if (guide_search_text[0] != '\0') {
        DrawText(guide_search_text, x + 4, y + 4, 12, WHITE);
    } else {
        DrawText("Search...", x + 4, y + 4, 12, GRAY);
    }

    // Cursor if active
    if (guide_search_active) {
        int text_width = MeasureText(guide_search_text, 12);
        DrawRectangle(x + 4 + text_width, y + 3, 1, 14, WHITE);
    }
}

/**
 * Draw the item browser grid
 */
static void draw_item_browser(int x, int y, Texture2D atlas, Inventory* inv) {
    int start_idx = guide_current_page * ITEMS_PER_PAGE;

    for (int row = 0; row < BROWSER_ROWS; row++) {
        for (int col = 0; col < BROWSER_COLS; col++) {
            int idx = start_idx + row * BROWSER_COLS + col;
            int slot_x = x + col * (BROWSER_ITEM_SIZE + 2);
            int slot_y = y + row * (BROWSER_ITEM_SIZE + 2);

            // Draw slot background
            Color bg = (Color){50, 50, 50, 200};
            if (idx < guide_filtered_count) {
                ItemType item = guide_filtered_items[idx];
                if (item == guide_selected_item) {
                    bg = (Color){80, 80, 120, 255};  // Selected highlight
                }
            }
            DrawRectangle(slot_x, slot_y, BROWSER_ITEM_SIZE, BROWSER_ITEM_SIZE, bg);
            DrawRectangleLines(slot_x, slot_y, BROWSER_ITEM_SIZE, BROWSER_ITEM_SIZE,
                               (Color){80, 80, 80, 255});

            // Draw item icon if valid
            if (idx < guide_filtered_count) {
                ItemType item = guide_filtered_items[idx];

                // Check if item has a recipe (tint if not craftable)
                const CraftingRecipe* recipe = crafting_find_recipe_for_output(item);
                bool can_craft = recipe && crafting_can_craft_recipe(inv, recipe);
                Color tint = can_craft ? WHITE : (Color){150, 150, 150, 200};

                // No recipe = darker
                if (!recipe) tint = (Color){100, 100, 100, 150};

                int icon_x = slot_x + (BROWSER_ITEM_SIZE - 24) / 2;
                int icon_y = slot_y + (BROWSER_ITEM_SIZE - 24) / 2;
                draw_mini_item_icon(item, icon_x, icon_y, 24, atlas, tint);
            }
        }
    }
}

/**
 * Draw pagination controls
 */
static void draw_pagination(int x, int y) {
    int total_pages = guide_get_total_pages();
    if (total_pages <= 0) total_pages = 1;

    // Previous button
    Color prev_color = (guide_current_page > 0) ? WHITE : DARKGRAY;
    DrawText("<", x, y, 16, prev_color);

    // Page indicator
    const char* page_text = TextFormat("%d/%d", guide_current_page + 1, total_pages);
    int text_width = MeasureText(page_text, 12);
    DrawText(page_text, x + 60 - text_width / 2, y + 2, 12, LIGHTGRAY);

    // Next button
    Color next_color = (guide_current_page < total_pages - 1) ? WHITE : DARKGRAY;
    DrawText(">", x + 110, y, 16, next_color);
}

/**
 * Draw the recipe preview (3x3 grid showing exact pattern)
 */
static void draw_recipe_preview(int x, int y, Texture2D atlas, Inventory* inv) {
    const CraftingRecipe* recipe = crafting_find_recipe_for_output(guide_selected_item);

    if (!recipe) {
        DrawText("No recipe", x + 20, y + 30, 12, GRAY);
        return;
    }

    // Draw recipe name
    const char* item_name = item_get_name(recipe->output);
    DrawText(item_name, x, y, 12, WHITE);

    int grid_y = y + 18;

    // Draw 3x3 input grid
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int slot_x = x + col * (PREVIEW_SLOT_SIZE + PREVIEW_GAP);
            int slot_y = grid_y + row * (PREVIEW_SLOT_SIZE + PREVIEW_GAP);
            int idx = row * 3 + col;

            // Draw slot
            DrawRectangle(slot_x, slot_y, PREVIEW_SLOT_SIZE, PREVIEW_SLOT_SIZE,
                          (Color){60, 60, 60, 200});
            DrawRectangleLines(slot_x, slot_y, PREVIEW_SLOT_SIZE, PREVIEW_SLOT_SIZE,
                               (Color){90, 90, 90, 255});

            // Draw ingredient icon
            if (recipe->inputs[idx] != ITEM_NONE) {
                int icon_x = slot_x + (PREVIEW_SLOT_SIZE - 20) / 2;
                int icon_y = slot_y + (PREVIEW_SLOT_SIZE - 20) / 2;
                draw_mini_item_icon(recipe->inputs[idx], icon_x, icon_y, 20, atlas, WHITE);
            }
        }
    }

    // Draw arrow
    int arrow_x = x + 3 * (PREVIEW_SLOT_SIZE + PREVIEW_GAP) + 4;
    int arrow_y = grid_y + PREVIEW_SLOT_SIZE;
    DrawText("=>", arrow_x, arrow_y, 14, WHITE);

    // Draw output slot
    int out_x = arrow_x + 25;
    int out_y = grid_y + PREVIEW_SLOT_SIZE - PREVIEW_SLOT_SIZE / 2;
    DrawRectangle(out_x, out_y, PREVIEW_SLOT_SIZE, PREVIEW_SLOT_SIZE, (Color){60, 80, 60, 200});
    DrawRectangleLines(out_x, out_y, PREVIEW_SLOT_SIZE, PREVIEW_SLOT_SIZE, (Color){100, 150, 100, 255});

    int icon_x = out_x + (PREVIEW_SLOT_SIZE - 20) / 2;
    int icon_y = out_y + (PREVIEW_SLOT_SIZE - 20) / 2;
    draw_mini_item_icon(recipe->output, icon_x, icon_y, 20, atlas, WHITE);

    // Draw output count
    if (recipe->output_count > 1) {
        DrawText(TextFormat("x%d", recipe->output_count), out_x + PREVIEW_SLOT_SIZE + 2, out_y + 6, 10, WHITE);
    }

    // Draw available crafts count
    int available = crafting_count_available_crafts(inv, recipe);
    DrawText(TextFormat("Can craft: %d", available), x, grid_y + 3 * (PREVIEW_SLOT_SIZE + PREVIEW_GAP) + 4, 10,
             available > 0 ? GREEN : RED);
}

/**
 * Draw the auto-craft buttons
 */
static void draw_craft_buttons(int x, int y, Inventory* inv) {
    const CraftingRecipe* recipe = crafting_find_recipe_for_output(guide_selected_item);
    bool can_craft = recipe && crafting_count_available_crafts(inv, recipe) > 0;

    Color btn_color = can_craft ? (Color){60, 100, 60, 255} : (Color){60, 60, 60, 200};
    Color text_color = can_craft ? WHITE : GRAY;

    // Button dimensions
    int btn_w = 45;
    int btn_h = 22;
    int gap = 5;

    // Craft 1 button
    DrawRectangle(x, y, btn_w, btn_h, btn_color);
    DrawRectangleLines(x, y, btn_w, btn_h, (Color){100, 100, 100, 255});
    DrawText("1", x + btn_w/2 - 3, y + 4, 14, text_color);

    // Craft 10 button
    DrawRectangle(x + btn_w + gap, y, btn_w, btn_h, btn_color);
    DrawRectangleLines(x + btn_w + gap, y, btn_w, btn_h, (Color){100, 100, 100, 255});
    DrawText("10", x + btn_w + gap + btn_w/2 - 8, y + 4, 14, text_color);

    // Craft All button
    DrawRectangle(x + 2*(btn_w + gap), y, btn_w + 10, btn_h, btn_color);
    DrawRectangleLines(x + 2*(btn_w + gap), y, btn_w + 10, btn_h, (Color){100, 100, 100, 255});
    DrawText("All", x + 2*(btn_w + gap) + 12, y + 4, 14, text_color);
}

/**
 * Draw the Luanti-style crafting guide sidebar
 */
static void draw_crafting_guide(Inventory* inv, Texture2D atlas) {
    guide_init_if_needed();

    // Draw sidebar panel background
    DrawRectangle(GUIDE_X, GUIDE_Y, GUIDE_WIDTH, GUIDE_HEIGHT, (Color){40, 40, 40, 240});
    DrawRectangleLines(GUIDE_X, GUIDE_Y, GUIDE_WIDTH, GUIDE_HEIGHT, (Color){150, 150, 150, 255});

    // Draw "Crafting Guide" title
    DrawText("Crafting Guide", GUIDE_X + 10, GUIDE_Y + 8, 14, WHITE);

    // Search box
    draw_search_box(GUIDE_X + 10, GUIDE_Y + 28, GUIDE_WIDTH - 20);

    // Divider
    DrawLine(GUIDE_X + 10, GUIDE_Y + 52, GUIDE_X + GUIDE_WIDTH - 10, GUIDE_Y + 52,
             (Color){100, 100, 100, 255});

    // Item browser grid
    draw_item_browser(GUIDE_X + 10, GUIDE_Y + 58, atlas, inv);

    // Pagination
    int browser_height = BROWSER_ROWS * (BROWSER_ITEM_SIZE + 2);
    draw_pagination(GUIDE_X + 45, GUIDE_Y + 60 + browser_height + 5);

    // Divider before recipe preview
    int preview_y = GUIDE_Y + 60 + browser_height + 28;
    DrawLine(GUIDE_X + 10, preview_y - 3, GUIDE_X + GUIDE_WIDTH - 10, preview_y - 3,
             (Color){100, 100, 100, 255});

    // Recipe preview (only if an item is selected)
    if (guide_selected_item != ITEM_NONE) {
        draw_recipe_preview(GUIDE_X + 10, preview_y, atlas, inv);

        // Craft buttons
        draw_craft_buttons(GUIDE_X + 10, GUIDE_Y + GUIDE_HEIGHT - 32, inv);
    } else {
        DrawText("Click an item above", GUIDE_X + 20, preview_y + 20, 11, GRAY);
        DrawText("to see its recipe", GUIDE_X + 25, preview_y + 35, 11, GRAY);
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

void inventory_ui_draw_item_icon(ItemType type, int x, int y, int size, Texture2D atlas) {
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

    DrawTexturePro(atlas, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
}

void inventory_ui_draw_hotbar(Inventory* inv, Texture2D atlas) {
    if (!inv) return;

    // Calculate hotbar position (centered at bottom of screen)
    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();
    int total_width = (HOTBAR_SLOT_SIZE * HOTBAR_SIZE) + (HOTBAR_GAP * (HOTBAR_SIZE - 1));
    int start_x = (screen_width - total_width) / 2;
    int start_y = screen_height - HOTBAR_PADDING_BOTTOM;

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
    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();

    // Draw semi-transparent background overlay
    DrawRectangle(0, 0, screen_width, screen_height, (Color){0, 0, 0, 150});

    // Draw inventory panel (adjusted to make room for crafting guide)
    int panel_x = 50;
    int panel_y = 100;
    int panel_w = 460;  // Wide enough for 9 columns
    int panel_h = 480;  // Same height as crafting guide

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
    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();
    if (tooltip_x + text_width + padding * 2 > screen_width) {
        tooltip_x = mouse_x - text_width - padding * 2 - 12;
    }
    if (tooltip_y + font_size + padding * 2 > screen_height) {
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
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});

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
// CRAFTING GUIDE INTERACTION HANDLING
// ============================================================================

void inventory_ui_handle_scroll(int scroll_delta) {
    // No longer used for recipe list - now using pagination
    (void)scroll_delta;
}

/**
 * Handle click on crafting guide sidebar
 * Returns true if click was handled
 */
bool inventory_ui_handle_guide_click(Inventory* inv, int mouse_x, int mouse_y) {
    if (!inv) return false;

    // Check if click is in guide area
    if (mouse_x < GUIDE_X || mouse_x > GUIDE_X + GUIDE_WIDTH ||
        mouse_y < GUIDE_Y || mouse_y > GUIDE_Y + GUIDE_HEIGHT) {
        return false;
    }

    // Check search box click
    int search_x = GUIDE_X + 10;
    int search_y = GUIDE_Y + 28;
    if (mouse_x >= search_x && mouse_x < search_x + GUIDE_WIDTH - 20 &&
        mouse_y >= search_y && mouse_y < search_y + 20) {
        guide_search_active = true;
        return true;
    } else {
        guide_search_active = false;
    }

    // Check item browser clicks
    int browser_x = GUIDE_X + 10;
    int browser_y = GUIDE_Y + 58;
    int browser_width = BROWSER_COLS * (BROWSER_ITEM_SIZE + 2);
    int browser_height = BROWSER_ROWS * (BROWSER_ITEM_SIZE + 2);

    if (mouse_x >= browser_x && mouse_x < browser_x + browser_width &&
        mouse_y >= browser_y && mouse_y < browser_y + browser_height) {
        // Calculate which item was clicked
        int col = (mouse_x - browser_x) / (BROWSER_ITEM_SIZE + 2);
        int row = (mouse_y - browser_y) / (BROWSER_ITEM_SIZE + 2);
        int idx = guide_current_page * ITEMS_PER_PAGE + row * BROWSER_COLS + col;

        if (idx < guide_filtered_count) {
            guide_selected_item = guide_filtered_items[idx];
            printf("[GUIDE] Selected item: %s\n", item_get_name(guide_selected_item));
        }
        return true;
    }

    // Check pagination clicks
    int pagination_y = GUIDE_Y + 60 + browser_height + 5;
    int pagination_x = GUIDE_X + 45;

    // Previous button
    if (mouse_x >= pagination_x && mouse_x < pagination_x + 20 &&
        mouse_y >= pagination_y && mouse_y < pagination_y + 20) {
        if (guide_current_page > 0) {
            guide_current_page--;
        }
        return true;
    }

    // Next button
    if (mouse_x >= pagination_x + 110 && mouse_x < pagination_x + 130 &&
        mouse_y >= pagination_y && mouse_y < pagination_y + 20) {
        int total_pages = guide_get_total_pages();
        if (guide_current_page < total_pages - 1) {
            guide_current_page++;
        }
        return true;
    }

    // Check craft button clicks
    if (guide_selected_item != ITEM_NONE) {
        int btn_y = GUIDE_Y + GUIDE_HEIGHT - 32;
        int btn_x = GUIDE_X + 10;
        int btn_w = 45;
        int btn_h = 22;
        int gap = 5;

        // Craft 1 button
        if (mouse_x >= btn_x && mouse_x < btn_x + btn_w &&
            mouse_y >= btn_y && mouse_y < btn_y + btn_h) {
            const CraftingRecipe* recipe = crafting_find_recipe_for_output(guide_selected_item);
            if (recipe) {
                crafting_auto_place_ingredients(inv, recipe, 1);
            }
            return true;
        }

        // Craft 10 button
        btn_x += btn_w + gap;
        if (mouse_x >= btn_x && mouse_x < btn_x + btn_w &&
            mouse_y >= btn_y && mouse_y < btn_y + btn_h) {
            const CraftingRecipe* recipe = crafting_find_recipe_for_output(guide_selected_item);
            if (recipe) {
                crafting_auto_place_ingredients(inv, recipe, 10);
            }
            return true;
        }

        // Craft All button
        btn_x += btn_w + gap;
        if (mouse_x >= btn_x && mouse_x < btn_x + btn_w + 10 &&
            mouse_y >= btn_y && mouse_y < btn_y + btn_h) {
            const CraftingRecipe* recipe = crafting_find_recipe_for_output(guide_selected_item);
            if (recipe) {
                crafting_auto_place_ingredients(inv, recipe, -1);  // -1 = all
            }
            return true;
        }
    }

    return true;  // Click was in guide area but didn't hit anything specific
}

/**
 * Handle keyboard input for crafting guide (search)
 */
void inventory_ui_handle_guide_key(int key) {
    if (!guide_search_active) return;

    if (key == KEY_ESCAPE) {
        guide_search_active = false;
        return;
    }

    if (key == KEY_BACKSPACE) {
        size_t len = strlen(guide_search_text);
        if (len > 0) {
            guide_search_text[len - 1] = '\0';
            guide_update_filter();
        }
        return;
    }

    // Add printable characters
    if (key >= 32 && key < 127) {
        size_t len = strlen(guide_search_text);
        if (len < sizeof(guide_search_text) - 1) {
            guide_search_text[len] = (char)key;
            guide_search_text[len + 1] = '\0';
            guide_update_filter();
        }
    }
}

/**
 * Check if search input is active
 */
bool inventory_ui_is_search_active(void) {
    return guide_search_active;
}

// ============================================================================
// HELD ITEM DISPLAY (First-Person View, 3D Luanti-Style)
// ============================================================================

// Colors for 3D rendering
static const Color SKIN_COLOR = {220, 180, 140, 255};
static const Color SKIN_DARK = {180, 140, 100, 255};
static const Color SLEEVE_COLOR = {70, 110, 170, 255};
static const Color SLEEVE_DARK = {50, 80, 130, 255};
static const Color WOOD_COLOR = {139, 90, 43, 255};
static const Color WOOD_DARK = {100, 65, 30, 255};
static const Color STONE_COLOR = {140, 140, 140, 255};
static const Color STONE_DARK = {100, 100, 100, 255};

/**
 * Calculate swing animation parameters for 3D
 * Returns swing angle and forward offset
 */
static void calculate_3d_swing(float swing_progress, float* out_angle, float* out_forward, float* out_up) {
    if (swing_progress <= 0.0f) {
        // Rest position
        *out_angle = 0.0f;
        *out_forward = 0.0f;
        *out_up = 0.0f;
        return;
    }

    float t = swing_progress;

    if (t < 0.15f) {
        // Wind-up: pull back slightly
        float p = t / 0.15f;
        *out_angle = -15.0f * p;
        *out_forward = -0.05f * p;
        *out_up = -0.02f * p;
    }
    else if (t < 0.5f) {
        // Strike: fast forward arc
        float p = (t - 0.15f) / 0.35f;
        *out_angle = -15.0f + 85.0f * p;  // -15 to +70
        *out_forward = -0.05f + 0.35f * p;
        *out_up = -0.02f + 0.15f * p;
    }
    else {
        // Follow-through: smooth return
        float p = (t - 0.5f) / 0.5f;
        float ease = 1.0f - (1.0f - p) * (1.0f - p);
        *out_angle = 70.0f * (1.0f - ease);
        *out_forward = 0.3f * (1.0f - ease);
        *out_up = 0.13f * (1.0f - ease);
    }
}

/**
 * Draw a textured cube using texture atlas
 */
static void draw_textured_block(BlockType block_type, float size, Texture2D atlas) {
    float half = size / 2.0f;

    // Get UV coords for each face from texture atlas
    TextureCoords top_coords = texture_atlas_get_coords(block_type, 0);    // Top
    TextureCoords bottom_coords = texture_atlas_get_coords(block_type, 1); // Bottom
    TextureCoords side_coords = texture_atlas_get_coords(block_type, 2);   // Sides

    rlSetTexture(atlas.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);

    // TOP face (+Y) - brightest
    rlColor4ub(255, 255, 255, 255);
    rlTexCoord2f(top_coords.u_min, top_coords.v_min); rlVertex3f(-half, half, -half);
    rlTexCoord2f(top_coords.u_max, top_coords.v_min); rlVertex3f(half, half, -half);
    rlTexCoord2f(top_coords.u_max, top_coords.v_max); rlVertex3f(half, half, half);
    rlTexCoord2f(top_coords.u_min, top_coords.v_max); rlVertex3f(-half, half, half);

    // BOTTOM face (-Y)
    rlColor4ub(180, 180, 180, 255);
    rlTexCoord2f(bottom_coords.u_min, bottom_coords.v_min); rlVertex3f(-half, -half, half);
    rlTexCoord2f(bottom_coords.u_max, bottom_coords.v_min); rlVertex3f(half, -half, half);
    rlTexCoord2f(bottom_coords.u_max, bottom_coords.v_max); rlVertex3f(half, -half, -half);
    rlTexCoord2f(bottom_coords.u_min, bottom_coords.v_max); rlVertex3f(-half, -half, -half);

    // FRONT face (+Z)
    rlColor4ub(220, 220, 220, 255);
    rlTexCoord2f(side_coords.u_min, side_coords.v_min); rlVertex3f(-half, half, half);
    rlTexCoord2f(side_coords.u_max, side_coords.v_min); rlVertex3f(half, half, half);
    rlTexCoord2f(side_coords.u_max, side_coords.v_max); rlVertex3f(half, -half, half);
    rlTexCoord2f(side_coords.u_min, side_coords.v_max); rlVertex3f(-half, -half, half);

    // BACK face (-Z)
    rlColor4ub(200, 200, 200, 255);
    rlTexCoord2f(side_coords.u_min, side_coords.v_min); rlVertex3f(half, half, -half);
    rlTexCoord2f(side_coords.u_max, side_coords.v_min); rlVertex3f(-half, half, -half);
    rlTexCoord2f(side_coords.u_max, side_coords.v_max); rlVertex3f(-half, -half, -half);
    rlTexCoord2f(side_coords.u_min, side_coords.v_max); rlVertex3f(half, -half, -half);

    // RIGHT face (+X)
    rlColor4ub(230, 230, 230, 255);
    rlTexCoord2f(side_coords.u_min, side_coords.v_min); rlVertex3f(half, half, half);
    rlTexCoord2f(side_coords.u_max, side_coords.v_min); rlVertex3f(half, half, -half);
    rlTexCoord2f(side_coords.u_max, side_coords.v_max); rlVertex3f(half, -half, -half);
    rlTexCoord2f(side_coords.u_min, side_coords.v_max); rlVertex3f(half, -half, half);

    // LEFT face (-X)
    rlColor4ub(190, 190, 190, 255);
    rlTexCoord2f(side_coords.u_min, side_coords.v_min); rlVertex3f(-half, half, -half);
    rlTexCoord2f(side_coords.u_max, side_coords.v_min); rlVertex3f(-half, half, half);
    rlTexCoord2f(side_coords.u_max, side_coords.v_max); rlVertex3f(-half, -half, half);
    rlTexCoord2f(side_coords.u_min, side_coords.v_max); rlVertex3f(-half, -half, -half);

    rlEnd();
    rlSetTexture(0);
}

/**
 * Draw a 3D pickaxe model
 */
static void draw_pickaxe_3d(bool is_stone) {
    Color head_color = is_stone ? STONE_COLOR : WOOD_COLOR;
    Color head_dark = is_stone ? STONE_DARK : WOOD_DARK;

    // Handle (vertical, thin)
    DrawCube((Vector3){0, -0.15f, 0}, 0.04f, 0.35f, 0.04f, WOOD_COLOR);
    DrawCube((Vector3){0.02f, -0.15f, 0}, 0.01f, 0.35f, 0.04f, WOOD_DARK);

    // Pickaxe head (horizontal bar at top)
    DrawCube((Vector3){0, 0.08f, 0}, 0.25f, 0.05f, 0.06f, head_color);
    DrawCube((Vector3){0, 0.1f, 0}, 0.25f, 0.02f, 0.06f, head_dark);

    // Pointed ends
    DrawCube((Vector3){-0.14f, 0.06f, 0}, 0.04f, 0.08f, 0.04f, head_color);
    DrawCube((Vector3){0.14f, 0.06f, 0}, 0.04f, 0.08f, 0.04f, head_color);
}

/**
 * Draw a 3D shovel model
 */
static void draw_shovel_3d(bool is_stone) {
    Color blade_color = is_stone ? STONE_COLOR : WOOD_COLOR;
    Color blade_dark = is_stone ? STONE_DARK : WOOD_DARK;

    // Handle (long, vertical)
    DrawCube((Vector3){0, -0.1f, 0}, 0.04f, 0.4f, 0.04f, WOOD_COLOR);
    DrawCube((Vector3){0.02f, -0.1f, 0}, 0.01f, 0.4f, 0.04f, WOOD_DARK);

    // Blade (rectangle at top)
    DrawCube((Vector3){0, 0.15f, 0}, 0.1f, 0.12f, 0.02f, blade_color);
    DrawCube((Vector3){0, 0.19f, 0}, 0.1f, 0.04f, 0.02f, blade_dark);
}

/**
 * Draw a 3D axe model
 */
static void draw_axe_3d(bool is_stone) {
    Color head_color = is_stone ? STONE_COLOR : WOOD_COLOR;
    Color head_dark = is_stone ? STONE_DARK : WOOD_DARK;

    // Handle (vertical)
    DrawCube((Vector3){0, -0.15f, 0}, 0.04f, 0.35f, 0.04f, WOOD_COLOR);
    DrawCube((Vector3){0.02f, -0.15f, 0}, 0.01f, 0.35f, 0.04f, WOOD_DARK);

    // Axe head (wedge - approximated with cubes)
    DrawCube((Vector3){0.06f, 0.06f, 0}, 0.1f, 0.12f, 0.04f, head_color);
    DrawCube((Vector3){0.1f, 0.06f, 0}, 0.04f, 0.1f, 0.03f, head_dark);
}

/**
 * Draw 3D fist/arm when no item selected
 */
static void draw_fist_3d(float swing_angle, float forward_offset, float up_offset) {
    // The arm extends from bottom-right, fist punches forward during swing

    rlPushMatrix();

    // Apply swing rotation (around X axis - punching motion)
    rlRotatef(swing_angle * 0.7f, 1, 0, 0);

    // Translate for swing motion
    rlTranslatef(0, up_offset * 0.5f, forward_offset);

    // SLEEVE (blue, long, extends downward)
    DrawCube((Vector3){0.05f, -0.35f, 0}, 0.12f, 0.3f, 0.08f, SLEEVE_COLOR);
    DrawCube((Vector3){0.1f, -0.35f, 0}, 0.02f, 0.3f, 0.08f, SLEEVE_DARK);

    // FOREARM (skin, above sleeve)
    DrawCube((Vector3){0.05f, -0.15f, 0}, 0.1f, 0.12f, 0.07f, SKIN_COLOR);
    DrawCube((Vector3){0.09f, -0.15f, 0}, 0.02f, 0.12f, 0.07f, SKIN_DARK);

    // FIST (skin cube)
    DrawCube((Vector3){0.05f, -0.02f, 0.02f}, 0.1f, 0.1f, 0.1f, SKIN_COLOR);
    DrawCube((Vector3){0.09f, -0.02f, 0.02f}, 0.02f, 0.1f, 0.1f, SKIN_DARK);

    rlPopMatrix();
}

/**
 * Draw held item in first-person view (3D, Luanti-style)
 * Must be called INSIDE BeginMode3D() block
 */
void inventory_ui_draw_held_item_3d(Player* player, Camera3D camera, Texture2D atlas) {
    (void)camera;  // Reserved for future use
    if (!player || !player->inventory) return;
    if (player->view_mode != VIEW_MODE_FIRST_PERSON) return;

    // Get swing animation progress
    float swing_progress = 0.0f;
    if (player->is_swinging && player->swing_duration > 0) {
        swing_progress = player->swing_time / player->swing_duration;
        if (swing_progress > 1.0f) swing_progress = 1.0f;
    }

    // Calculate animation parameters
    float swing_angle, forward_offset, up_offset;
    calculate_3d_swing(swing_progress, &swing_angle, &forward_offset, &up_offset);

    // Bob only when walking
    float vel_magnitude = sqrtf(player->velocity.x * player->velocity.x +
                                player->velocity.z * player->velocity.z);
    float bob = 0.0f;
    if (vel_magnitude > 0.5f) {
        bob = sinf(player->walk_animation_time * 2.0f) * 0.02f;
    }

    // Get currently selected hotbar item
    ItemStack* held = inventory_get_selected_hotbar_item(player->inventory);
    bool has_item = held && held->type != ITEM_NONE;

    // Screen-space offsets (fixed position on screen)
    // Note: In camera space, -X is RIGHT side of screen
    float swing_left = swing_progress > 0 ? 0.1f * sinf(swing_progress * 3.14159f) : 0.0f;
    float offset_x = -0.5f + swing_left;       // Right side of screen (negative X)
    float offset_y = -0.4f + up_offset + bob;  // Below center
    float offset_z = 1.2f + forward_offset;    // Forward (further away to show full item)

    // HUD camera - fixed position, never rotates with player
    // Depth buffer is cleared before this call, so we're always on top
    Camera3D hud_cam = {0};
    hud_cam.position = (Vector3){0, 0, 0};
    hud_cam.target = (Vector3){0, 0, 1};
    hud_cam.up = (Vector3){0, 1, 0};
    hud_cam.fovy = 50.0f;  // Narrower FOV to see item better
    hud_cam.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(hud_cam);

    // Disable backface culling so we see all sides of the held item
    rlDisableBackfaceCulling();

    // Position item in front of HUD camera (fixed screen position)
    rlPushMatrix();
    rlTranslatef(offset_x, offset_y, offset_z);

    // Apply swing rotation (only during swing animation)
    if (swing_progress > 0.0f) {
        rlRotatef(swing_angle, 1, 0, 0);
    } else {
        // Tilt item at rest (more upright)
        rlRotatef(0.0f, 1, 0, 0);  // Tilted down (less steep)
        rlRotatef(15.0f, 0, 0, 1);   // Tilted right
    }

    if (!has_item) {
        // Draw fist/arm
        draw_fist_3d(swing_angle, forward_offset, up_offset);
    } else {
        const ItemProperties* props = item_get_properties(held->type);

        if (props->is_tool) {
            // Draw 3D tool
            switch (held->type) {
                case ITEM_WOODEN_PICKAXE:
                    draw_pickaxe_3d(false);
                    break;
                case ITEM_STONE_PICKAXE:
                    draw_pickaxe_3d(true);
                    break;
                case ITEM_WOODEN_SHOVEL:
                    draw_shovel_3d(false);
                    break;
                case ITEM_STONE_SHOVEL:
                    draw_shovel_3d(true);
                    break;
                case ITEM_WOODEN_AXE:
                    draw_axe_3d(false);
                    break;
                case ITEM_STONE_AXE:
                    draw_axe_3d(true);
                    break;
                default:
                    // Unknown tool - draw generic cube
                    DrawCube((Vector3){0, 0, 0}, 0.15f, 0.15f, 0.15f, WOOD_COLOR);
                    break;
            }
        } else if (props->is_placeable) {
            // Draw 3D textured block (bigger size for visibility)
            draw_textured_block(props->places_as, 0.35f, atlas);
        } else {
            // Other items (stick, meat, etc) - draw as small cube with item color
            DrawCube((Vector3){0, 0, 0}, 0.15f, 0.15f, 0.15f, WOOD_COLOR);
        }
    }

    rlPopMatrix();
    rlEnableBackfaceCulling();  // Restore culling
    EndMode3D();
}
