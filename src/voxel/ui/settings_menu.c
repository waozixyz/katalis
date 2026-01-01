/**
 * Settings Menu - In-game tunable parameters UI
 *
 * Implementation of sliders, toggles, and spinners for game settings.
 */

#include "voxel/ui/settings_menu.h"
#include "voxel/core/settings_constants.h"
#include "voxel/world/world.h"
#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// UI LAYOUT CONSTANTS
// ============================================================================

#define MENU_WIDTH 500
#define MENU_HEIGHT 400
#define MARGIN 20
#define ITEM_HEIGHT 35
#define LABEL_WIDTH 200
#define CONTROL_WIDTH 200
#define CATEGORY_HEIGHT 30
#define SLIDER_HEIGHT 20
#define TOGGLE_SIZE 20

// Colors
#define COLOR_BG (Color){30, 30, 35, 240}
#define COLOR_HEADER (Color){50, 50, 60, 255}
#define COLOR_SELECTED (Color){80, 100, 140, 255}
#define COLOR_SLIDER_BG (Color){60, 60, 70, 255}
#define COLOR_SLIDER_FILL (Color){100, 150, 200, 255}
#define COLOR_TOGGLE_ON (Color){80, 180, 80, 255}
#define COLOR_TOGGLE_OFF (Color){100, 60, 60, 255}
#define COLOR_TEXT WHITE
#define COLOR_TEXT_DIM (Color){180, 180, 180, 255}

// ============================================================================
// CATEGORY ITEM DEFINITIONS
// ============================================================================

static const char* graphics_items[] = {
    "View Distance",
    "LOD Distance",
    "Batch Rebuilds/Frame"
};
#define GRAPHICS_ITEM_COUNT 3

static const char* world_items[] = {
    "Day Speed",
    "Time Paused"
};
#define WORLD_ITEM_COUNT 2

static const char* performance_items[] = {
    "Max Uploads/Frame",
    "Show Debug Info"
};
#define PERFORMANCE_ITEM_COUNT 2

static const char* category_names[] = {
    "Graphics",
    "World",
    "Performance"
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static void draw_slider(int x, int y, int width, float value, float min, float max, bool selected) {
    // Background
    DrawRectangle(x, y, width, SLIDER_HEIGHT, COLOR_SLIDER_BG);

    // Fill
    float pct = (value - min) / (max - min);
    if (pct < 0) pct = 0;
    if (pct > 1) pct = 1;
    int fill_width = (int)(pct * (width - 4));
    DrawRectangle(x + 2, y + 2, fill_width, SLIDER_HEIGHT - 4,
                  selected ? COLOR_SLIDER_FILL : (Color){80, 120, 160, 255});

    // Border
    if (selected) {
        DrawRectangleLines(x, y, width, SLIDER_HEIGHT, WHITE);
    }
}

static void draw_toggle(int x, int y, bool value, bool selected) {
    Color color = value ? COLOR_TOGGLE_ON : COLOR_TOGGLE_OFF;
    DrawRectangle(x, y, TOGGLE_SIZE, TOGGLE_SIZE, color);
    if (selected) {
        DrawRectangleLines(x, y, TOGGLE_SIZE, TOGGLE_SIZE, WHITE);
    }
    if (value) {
        // Draw checkmark
        DrawLine(x + 4, y + TOGGLE_SIZE/2, x + TOGGLE_SIZE/3, y + TOGGLE_SIZE - 4, WHITE);
        DrawLine(x + TOGGLE_SIZE/3, y + TOGGLE_SIZE - 4, x + TOGGLE_SIZE - 4, y + 4, WHITE);
    }
}

static void draw_spinner_value(int x, int y, int width, int value, bool selected) {
    char buf[32];
    snprintf(buf, sizeof(buf), "< %d >", value);
    int text_width = MeasureText(buf, 16);
    DrawText(buf, x + (width - text_width) / 2, y + 2, 16,
             selected ? WHITE : COLOR_TEXT_DIM);
    if (selected) {
        DrawRectangleLines(x, y, width, SLIDER_HEIGHT, WHITE);
    }
}

static void draw_float_value(int x, int y, int width, float value, bool selected) {
    char buf[32];
    snprintf(buf, sizeof(buf), "< %.2f >", value);
    int text_width = MeasureText(buf, 16);
    DrawText(buf, x + (width - text_width) / 2, y + 2, 16,
             selected ? WHITE : COLOR_TEXT_DIM);
    if (selected) {
        DrawRectangleLines(x, y, width, SLIDER_HEIGHT, WHITE);
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

SettingsMenu* settings_menu_create(GameSettings* live_settings) {
    SettingsMenu* menu = (SettingsMenu*)calloc(1, sizeof(SettingsMenu));
    if (!menu) return NULL;

    menu->live_settings = live_settings;
    menu->is_open = false;
    menu->selected_category = SETTINGS_CATEGORY_GRAPHICS;
    menu->selected_item = 0;
    menu->editing_value = false;

    if (live_settings) {
        menu->working_copy = *live_settings;
    }

    return menu;
}

void settings_menu_destroy(SettingsMenu* menu) {
    if (menu) {
        free(menu);
    }
}

void settings_menu_open(SettingsMenu* menu) {
    if (!menu) return;
    menu->is_open = true;
    menu->selected_category = SETTINGS_CATEGORY_GRAPHICS;
    menu->selected_item = 0;
    menu->editing_value = false;

    if (menu->live_settings) {
        menu->working_copy = *menu->live_settings;
    }
}

void settings_menu_close(SettingsMenu* menu) {
    if (!menu) return;
    menu->is_open = false;
    menu->editing_value = false;
}

bool settings_menu_is_open(SettingsMenu* menu) {
    return menu && menu->is_open;
}

int settings_menu_get_item_count(SettingsCategory category) {
    switch (category) {
        case SETTINGS_CATEGORY_GRAPHICS: return GRAPHICS_ITEM_COUNT;
        case SETTINGS_CATEGORY_WORLD: return WORLD_ITEM_COUNT;
        case SETTINGS_CATEGORY_PERFORMANCE: return PERFORMANCE_ITEM_COUNT;
        default: return 0;
    }
}

const char* settings_menu_get_item_label(SettingsCategory category, int item) {
    switch (category) {
        case SETTINGS_CATEGORY_GRAPHICS:
            if (item >= 0 && item < GRAPHICS_ITEM_COUNT) return graphics_items[item];
            break;
        case SETTINGS_CATEGORY_WORLD:
            if (item >= 0 && item < WORLD_ITEM_COUNT) return world_items[item];
            break;
        case SETTINGS_CATEGORY_PERFORMANCE:
            if (item >= 0 && item < PERFORMANCE_ITEM_COUNT) return performance_items[item];
            break;
        default:
            break;
    }
    return "";
}

int settings_menu_handle_input(SettingsMenu* menu) {
    if (!menu || !menu->is_open) return 0;

    int item_count = settings_menu_get_item_count(menu->selected_category);

    // Escape to close
    if (IsKeyPressed(KEY_ESCAPE)) {
        return 1;  // Close without saving
    }

    // Enter to apply and close
    if (IsKeyPressed(KEY_ENTER)) {
        return 2;  // Apply and close
    }

    // Tab to switch categories
    if (IsKeyPressed(KEY_TAB)) {
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            menu->selected_category = (menu->selected_category + SETTINGS_CATEGORY_COUNT - 1) % SETTINGS_CATEGORY_COUNT;
        } else {
            menu->selected_category = (menu->selected_category + 1) % SETTINGS_CATEGORY_COUNT;
        }
        menu->selected_item = 0;
        return 0;
    }

    // Up/Down to select items
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        menu->selected_item--;
        if (menu->selected_item < 0) {
            menu->selected_item = item_count - 1;
        }
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        menu->selected_item++;
        if (menu->selected_item >= item_count) {
            menu->selected_item = 0;
        }
    }

    // Left/Right to adjust values
    int delta = 0;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) delta = -1;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) delta = 1;

    if (delta != 0) {
        GameSettings* s = &menu->working_copy;

        switch (menu->selected_category) {
            case SETTINGS_CATEGORY_GRAPHICS:
                if (menu->selected_item == 0) {  // View Distance
                    s->view_distance += delta * 2;
                    if (s->view_distance < SETTING_VIEW_DIST_MIN) s->view_distance = SETTING_VIEW_DIST_MIN;
                    if (s->view_distance > SETTING_VIEW_DIST_MAX) s->view_distance = SETTING_VIEW_DIST_MAX;
                } else if (menu->selected_item == 1) {  // LOD Distance
                    s->lod_distance += delta * 2;
                    if (s->lod_distance < SETTING_LOD_DIST_MIN) s->lod_distance = SETTING_LOD_DIST_MIN;
                    if (s->lod_distance > SETTING_LOD_DIST_MAX) s->lod_distance = SETTING_LOD_DIST_MAX;
                } else if (menu->selected_item == 2) {  // Batch Rebuilds
                    s->batch_rebuilds += delta * 4;
                    if (s->batch_rebuilds < SETTING_BATCH_REBUILD_MIN) s->batch_rebuilds = SETTING_BATCH_REBUILD_MIN;
                    if (s->batch_rebuilds > SETTING_BATCH_REBUILD_MAX) s->batch_rebuilds = SETTING_BATCH_REBUILD_MAX;
                }
                break;

            case SETTINGS_CATEGORY_WORLD:
                if (menu->selected_item == 0) {  // Day Speed
                    s->day_speed += delta * 0.05f;
                    if (s->day_speed < SETTING_DAY_SPEED_MIN) s->day_speed = SETTING_DAY_SPEED_MIN;
                    if (s->day_speed > SETTING_DAY_SPEED_MAX) s->day_speed = SETTING_DAY_SPEED_MAX;
                } else if (menu->selected_item == 1) {  // Time Paused
                    s->time_paused = !s->time_paused;
                }
                break;

            case SETTINGS_CATEGORY_PERFORMANCE:
                if (menu->selected_item == 0) {  // Max Uploads
                    s->max_uploads_per_frame += delta * 8;
                    if (s->max_uploads_per_frame < SETTING_MAX_UPLOADS_MIN) s->max_uploads_per_frame = SETTING_MAX_UPLOADS_MIN;
                    if (s->max_uploads_per_frame > SETTING_MAX_UPLOADS_MAX) s->max_uploads_per_frame = SETTING_MAX_UPLOADS_MAX;
                } else if (menu->selected_item == 1) {  // Show Debug
                    s->show_debug_info = !s->show_debug_info;
                }
                break;

            default:
                break;
        }
    }

    return 0;
}

void settings_menu_draw(SettingsMenu* menu) {
    if (!menu || !menu->is_open) return;

    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();

    int menu_x = (screen_w - MENU_WIDTH) / 2;
    int menu_y = (screen_h - MENU_HEIGHT) / 2;

    // Background
    DrawRectangle(menu_x, menu_y, MENU_WIDTH, MENU_HEIGHT, COLOR_BG);
    DrawRectangleLines(menu_x, menu_y, MENU_WIDTH, MENU_HEIGHT, WHITE);

    // Title
    const char* title = "SETTINGS";
    int title_width = MeasureText(title, 24);
    DrawText(title, menu_x + (MENU_WIDTH - title_width) / 2, menu_y + 10, 24, WHITE);

    // Category tabs
    int tab_y = menu_y + 45;
    int tab_width = (MENU_WIDTH - MARGIN * 2) / SETTINGS_CATEGORY_COUNT;
    for (int i = 0; i < SETTINGS_CATEGORY_COUNT; i++) {
        int tab_x = menu_x + MARGIN + i * tab_width;
        Color tab_color = (i == (int)menu->selected_category) ? COLOR_SELECTED : COLOR_HEADER;
        DrawRectangle(tab_x, tab_y, tab_width - 4, CATEGORY_HEIGHT, tab_color);

        int text_w = MeasureText(category_names[i], 14);
        DrawText(category_names[i], tab_x + (tab_width - 4 - text_w) / 2, tab_y + 8, 14, WHITE);
    }

    // Items
    int items_y = tab_y + CATEGORY_HEIGHT + 15;
    int item_count = settings_menu_get_item_count(menu->selected_category);
    GameSettings* s = &menu->working_copy;

    for (int i = 0; i < item_count; i++) {
        int item_y = items_y + i * ITEM_HEIGHT;
        bool selected = (i == menu->selected_item);

        if (selected) {
            DrawRectangle(menu_x + MARGIN - 5, item_y - 3, MENU_WIDTH - MARGIN * 2 + 10, ITEM_HEIGHT - 2,
                          (Color){60, 70, 90, 200});
        }

        // Label
        const char* label = settings_menu_get_item_label(menu->selected_category, i);
        DrawText(label, menu_x + MARGIN, item_y + 5, 16, selected ? WHITE : COLOR_TEXT_DIM);

        // Control
        int ctrl_x = menu_x + MARGIN + LABEL_WIDTH;
        int ctrl_y = item_y + 5;

        switch (menu->selected_category) {
            case SETTINGS_CATEGORY_GRAPHICS:
                if (i == 0) {  // View Distance
                    draw_slider(ctrl_x, ctrl_y, CONTROL_WIDTH, (float)s->view_distance,
                               SETTING_VIEW_DIST_MIN, SETTING_VIEW_DIST_MAX, selected);
                    char val[16];
                    snprintf(val, sizeof(val), "%d", s->view_distance);
                    DrawText(val, ctrl_x + CONTROL_WIDTH + 10, ctrl_y + 2, 14, COLOR_TEXT_DIM);
                } else if (i == 1) {  // LOD Distance
                    draw_slider(ctrl_x, ctrl_y, CONTROL_WIDTH, (float)s->lod_distance,
                               SETTING_LOD_DIST_MIN, SETTING_LOD_DIST_MAX, selected);
                    char val[16];
                    snprintf(val, sizeof(val), "%d", s->lod_distance);
                    DrawText(val, ctrl_x + CONTROL_WIDTH + 10, ctrl_y + 2, 14, COLOR_TEXT_DIM);
                } else if (i == 2) {  // Batch Rebuilds
                    draw_spinner_value(ctrl_x, ctrl_y, CONTROL_WIDTH, s->batch_rebuilds, selected);
                }
                break;

            case SETTINGS_CATEGORY_WORLD:
                if (i == 0) {  // Day Speed
                    draw_float_value(ctrl_x, ctrl_y, CONTROL_WIDTH, s->day_speed, selected);
                } else if (i == 1) {  // Time Paused
                    draw_toggle(ctrl_x, ctrl_y, s->time_paused, selected);
                }
                break;

            case SETTINGS_CATEGORY_PERFORMANCE:
                if (i == 0) {  // Max Uploads
                    draw_spinner_value(ctrl_x, ctrl_y, CONTROL_WIDTH, s->max_uploads_per_frame, selected);
                } else if (i == 1) {  // Show Debug
                    draw_toggle(ctrl_x, ctrl_y, s->show_debug_info, selected);
                }
                break;

            default:
                break;
        }
    }

    // Footer help
    const char* help = "Tab: Category | Arrows: Navigate/Adjust | Enter: Apply | Esc: Cancel";
    int help_w = MeasureText(help, 12);
    DrawText(help, menu_x + (MENU_WIDTH - help_w) / 2, menu_y + MENU_HEIGHT - 25, 12, COLOR_TEXT_DIM);
}

void settings_menu_apply(SettingsMenu* menu, World* world) {
    if (!menu || !menu->live_settings) return;

    // Copy working settings to live settings
    *menu->live_settings = menu->working_copy;

    // Apply settings to world
    if (world) {
        world_set_view_distance(world, menu->working_copy.view_distance);
        world_set_batch_rebuilds(world, menu->working_copy.batch_rebuilds);
        world_set_max_uploads(world, menu->working_copy.max_uploads_per_frame);
    }

    printf("[SETTINGS] Applied: view=%d, lod=%d, batch=%d, uploads=%d, day_speed=%.2f\n",
           menu->working_copy.view_distance,
           menu->working_copy.lod_distance,
           menu->working_copy.batch_rebuilds,
           menu->working_copy.max_uploads_per_frame,
           menu->working_copy.day_speed);
}
