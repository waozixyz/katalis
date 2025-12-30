/**
 * Inventory UI Rendering
 *
 * Draws hotbar and full inventory screen using Raylib primitives.
 */

#ifndef VOXEL_INVENTORY_UI_H
#define VOXEL_INVENTORY_UI_H

#include <raylib.h>
#include "voxel/inventory/inventory.h"

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Draw the hotbar at the bottom of the screen
 * Always visible during gameplay
 */
void inventory_ui_draw_hotbar(Inventory* inv, Texture2D atlas);

/**
 * Draw the full inventory screen (main inventory + crafting)
 * Only visible when inventory is open (E key)
 */
void inventory_ui_draw_full_screen(Inventory* inv, Texture2D atlas);

/**
 * Draw an item icon from the texture atlas
 * Used for rendering items in slots
 */
void inventory_ui_draw_item_icon(ItemType type, int x, int y, int size, Texture2D atlas);

/**
 * Draw tooltip for hovered item in inventory
 * Shows item name near the cursor
 */
void inventory_ui_draw_tooltip(Inventory* inv, int mouse_x, int mouse_y);

// Forward declaration for chest
typedef struct ChestData ChestData;

/**
 * Draw chest UI with player inventory below
 */
void inventory_ui_draw_chest(ChestData* chest, Inventory* inv, Texture2D atlas);

/**
 * Handle click on chest UI
 * Transfers items between chest and player inventory
 */
void inventory_ui_handle_chest_click(ChestData* chest, Inventory* inv, int mouse_x, int mouse_y);

/**
 * Handle scroll input for crafting guide
 * scroll_delta: positive = scroll up, negative = scroll down
 */
void inventory_ui_handle_scroll(int scroll_delta);

#endif // VOXEL_INVENTORY_UI_H
