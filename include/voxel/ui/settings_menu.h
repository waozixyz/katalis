/**
 * Settings Menu - In-game tunable parameters UI
 *
 * Provides sliders, toggles, and spinners for adjusting game settings
 * accessible from the pause menu.
 */

#ifndef VOXEL_SETTINGS_MENU_H
#define VOXEL_SETTINGS_MENU_H

#include <stdbool.h>

// Forward declarations
typedef struct World World;

// ============================================================================
// SETTINGS CATEGORIES
// ============================================================================

typedef enum {
    SETTINGS_CATEGORY_GRAPHICS = 0,
    SETTINGS_CATEGORY_WORLD,
    SETTINGS_CATEGORY_PERFORMANCE,
    SETTINGS_CATEGORY_COUNT
} SettingsCategory;

// ============================================================================
// GAME SETTINGS STRUCTURE (matches game.c)
// ============================================================================

typedef struct GameSettings {
    // Graphics
    int view_distance;           // 2-32 chunks
    int lod_distance;            // 4-16 chunks
    int batch_rebuilds;          // 4-64 per frame

    // World
    float day_speed;             // 0.01-1.0 hours/sec
    bool time_paused;

    // Performance
    int max_uploads_per_frame;   // 8-128
    bool show_debug_info;

    // Input
    float mouse_sensitivity;     // 0.001-0.01
} GameSettings;

// ============================================================================
// SETTINGS MENU STATE
// ============================================================================

typedef struct SettingsMenu {
    bool is_open;
    SettingsCategory selected_category;
    int selected_item;                // Which setting in category
    bool editing_value;               // Currently adjusting a slider/spinner
    GameSettings working_copy;        // Copy being edited
    GameSettings* live_settings;      // Pointer to actual game settings
} SettingsMenu;

// ============================================================================
// API
// ============================================================================

/**
 * Create a new settings menu
 * @param live_settings Pointer to the game's settings struct
 */
SettingsMenu* settings_menu_create(GameSettings* live_settings);

/**
 * Destroy the settings menu
 */
void settings_menu_destroy(SettingsMenu* menu);

/**
 * Open the settings menu (copies live settings to working copy)
 */
void settings_menu_open(SettingsMenu* menu);

/**
 * Close the settings menu (discards working copy changes)
 */
void settings_menu_close(SettingsMenu* menu);

/**
 * Check if settings menu is open
 */
bool settings_menu_is_open(SettingsMenu* menu);

/**
 * Handle input for settings menu
 * @return Action code: 0=continue, 1=close menu, 2=apply and close
 */
int settings_menu_handle_input(SettingsMenu* menu);

/**
 * Draw the settings menu UI
 */
void settings_menu_draw(SettingsMenu* menu);

/**
 * Apply working settings to live settings and update game systems
 */
void settings_menu_apply(SettingsMenu* menu, World* world);

/**
 * Get settings item count for a category
 */
int settings_menu_get_item_count(SettingsCategory category);

/**
 * Get settings item label
 */
const char* settings_menu_get_item_label(SettingsCategory category, int item);

#endif // VOXEL_SETTINGS_MENU_H
