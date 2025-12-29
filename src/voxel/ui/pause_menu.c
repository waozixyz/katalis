/**
 * Pause Menu Implementation
 *
 * Extended pause menu with LAN multiplayer Host/Join options.
 * Includes text input for IP/port and connection management.
 */

#include "voxel/ui/pause_menu.h"
#include "voxel/network/network.h"
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

// Random player name generation
static const char* ADJECTIVES[] = {
    "Swift", "Brave", "Clever", "Silent", "Mighty", "Wild", "Dark", "Bright",
    "Fierce", "Noble", "Mystic", "Shadow", "Storm", "Iron", "Golden", "Crystal"
};
static const char* NOUNS[] = {
    "Wolf", "Hawk", "Bear", "Dragon", "Phoenix", "Tiger", "Raven", "Viper",
    "Knight", "Mage", "Hunter", "Warrior", "Wizard", "Ninja", "Sage", "Ranger"
};
#define NUM_ADJECTIVES (sizeof(ADJECTIVES) / sizeof(ADJECTIVES[0]))
#define NUM_NOUNS (sizeof(NOUNS) / sizeof(NOUNS[0]))

static void generate_random_name(char* buffer, size_t size) {
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    int adj_idx = rand() % NUM_ADJECTIVES;
    int noun_idx = rand() % NUM_NOUNS;
    int num = rand() % 100;
    snprintf(buffer, size, "%s%s%d", ADJECTIVES[adj_idx], NOUNS[noun_idx], num);
}

// ============================================================================
// CONSTANTS
// ============================================================================

// Screen dimensions (must match window size)
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// Button dimensions
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define BUTTON_GAP 15
#define SMALL_BUTTON_WIDTH 120

// Panel dimensions
#define PANEL_WIDTH 450
#define PANEL_HEIGHT 400

// Input field dimensions
#define INPUT_WIDTH 250
#define INPUT_HEIGHT 35

// Colors
#define COLOR_OVERLAY ((Color){0, 0, 0, 150})
#define COLOR_PANEL ((Color){40, 40, 40, 240})
#define COLOR_BUTTON_NORMAL ((Color){60, 60, 60, 255})
#define COLOR_BUTTON_HOVER ((Color){80, 80, 80, 255})
#define COLOR_BUTTON_BORDER ((Color){100, 100, 100, 255})
#define COLOR_INPUT_BG ((Color){30, 30, 30, 255})
#define COLOR_INPUT_ACTIVE ((Color){50, 50, 70, 255})
#define COLOR_INPUT_BORDER ((Color){80, 80, 120, 255})
#define COLOR_STATUS_OK ((Color){100, 200, 100, 255})
#define COLOR_STATUS_ERROR ((Color){200, 100, 100, 255})
#define COLOR_LABEL ((Color){180, 180, 180, 255})

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static bool is_point_in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

static void draw_button(int x, int y, int w, int h, const char* text, bool is_hovered) {
    Color bg_color = is_hovered ? COLOR_BUTTON_HOVER : COLOR_BUTTON_NORMAL;
    DrawRectangle(x, y, w, h, bg_color);
    DrawRectangleLines(x, y, w, h, COLOR_BUTTON_BORDER);

    int font_size = 18;
    int text_width = MeasureText(text, font_size);
    int text_x = x + (w - text_width) / 2;
    int text_y = y + (h - font_size) / 2;
    DrawText(text, text_x, text_y, font_size, WHITE);
}

static void draw_input_field(int x, int y, int w, int h, const char* text,
                             const char* placeholder, bool is_active, int cursor_pos) {
    Color bg_color = is_active ? COLOR_INPUT_ACTIVE : COLOR_INPUT_BG;
    Color border_color = is_active ? COLOR_INPUT_BORDER : COLOR_BUTTON_BORDER;

    DrawRectangle(x, y, w, h, bg_color);
    DrawRectangleLines(x, y, w, h, border_color);

    int font_size = 16;
    int text_y = y + (h - font_size) / 2;
    int text_x = x + 8;

    if (text[0] != '\0') {
        DrawText(text, text_x, text_y, font_size, WHITE);

        // Draw cursor if active
        if (is_active) {
            int cursor_x = text_x + MeasureText(text, font_size);
            // Blink cursor
            if ((int)(GetTime() * 2) % 2 == 0) {
                DrawRectangle(cursor_x + 2, y + 5, 2, h - 10, WHITE);
            }
        }
    } else {
        // Draw placeholder
        DrawText(placeholder, text_x, text_y, font_size, (Color){100, 100, 100, 255});

        // Draw cursor at start if active
        if (is_active && (int)(GetTime() * 2) % 2 == 0) {
            DrawRectangle(text_x, y + 5, 2, h - 10, WHITE);
        }
    }
}

static void draw_label(int x, int y, const char* text) {
    DrawText(text, x, y, 14, COLOR_LABEL);
}

// Handle text input for a field
// allow_hostname: true allows letters, dots, colons, dashes (for IP/hostname)
//                 false allows only digits (for port)
static void handle_text_input(char* buffer, int max_len, bool allow_hostname) {
    // Get character input
    int key = GetCharPressed();
    while (key > 0) {
        // Only allow valid characters
        bool valid = false;
        if (key >= '0' && key <= '9') valid = true;
        if (allow_hostname) {
            if (key == '.' || key == ':' || key == '-') valid = true;
            if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) valid = true;
        }

        if (valid) {
            int len = strlen(buffer);
            if (len < max_len - 1) {
                buffer[len] = (char)key;
                buffer[len + 1] = '\0';
            }
        }
        key = GetCharPressed();
    }

    // Handle backspace
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        int len = strlen(buffer);
        if (len > 0) {
            buffer[len - 1] = '\0';
        }
    }
}

// ============================================================================
// DRAW FUNCTIONS FOR EACH STATE
// ============================================================================

static void draw_main_menu(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    // Title
    const char* title = "PAUSED";
    int title_font_size = 36;
    int title_width = MeasureText(title, title_font_size);
    DrawText(title, panel_x + (PANEL_WIDTH - title_width) / 2, panel_y + 30, title_font_size, WHITE);

    // Buttons
    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
    int y = panel_y + 90;

    bool hovered;

    // Resume
    hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Resume Game", hovered);
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Host Game
    hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Host Game", hovered);
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Join Game
    hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Join Game", hovered);
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Exit Game
    hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Exit Game", hovered);
}

static void draw_host_setup(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    // Title
    const char* title = "HOST GAME";
    int title_font_size = 32;
    int title_width = MeasureText(title, title_font_size);
    DrawText(title, panel_x + (PANEL_WIDTH - title_width) / 2, panel_y + 30, title_font_size, WHITE);

    int y = panel_y + 100;
    int label_x = panel_x + 50;
    int input_x = panel_x + (PANEL_WIDTH - INPUT_WIDTH) / 2;

    // Port label and input
    draw_label(label_x, y, "Port:");
    y += 20;
    draw_input_field(input_x, y, INPUT_WIDTH, INPUT_HEIGHT, menu->port_input, "7777",
                     menu->active_input == INPUT_FIELD_PORT, 0);
    y += INPUT_HEIGHT + 30;

    // Info text
    const char* info = "Other players can join using your IP address";
    int info_width = MeasureText(info, 12);
    DrawText(info, panel_x + (PANEL_WIDTH - info_width) / 2, y, 12, COLOR_LABEL);
    y += 40;

    // Buttons
    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;

    // Start Server
    bool hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Start Server", hovered);
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Back
    hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Back", hovered);

    // Status message
    if (menu->status_message[0] != '\0') {
        Color status_color = menu->status_is_error ? COLOR_STATUS_ERROR : COLOR_STATUS_OK;
        int msg_width = MeasureText(menu->status_message, 14);
        DrawText(menu->status_message, panel_x + (PANEL_WIDTH - msg_width) / 2,
                 panel_y + PANEL_HEIGHT - 40, 14, status_color);
    }
}

static void draw_join_setup(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    // Title
    const char* title = "JOIN GAME";
    int title_font_size = 32;
    int title_width = MeasureText(title, title_font_size);
    DrawText(title, panel_x + (PANEL_WIDTH - title_width) / 2, panel_y + 30, title_font_size, WHITE);

    int y = panel_y + 80;
    int label_x = panel_x + 50;
    int input_x = panel_x + (PANEL_WIDTH - INPUT_WIDTH) / 2;

    // IP address
    draw_label(label_x, y, "Server IP:");
    y += 20;
    draw_input_field(input_x, y, INPUT_WIDTH, INPUT_HEIGHT, menu->ip_input, "192.168.1.100",
                     menu->active_input == INPUT_FIELD_IP, 0);
    y += INPUT_HEIGHT + 20;

    // Port
    draw_label(label_x, y, "Port:");
    y += 20;
    draw_input_field(input_x, y, INPUT_WIDTH, INPUT_HEIGHT, menu->port_input, "7777",
                     menu->active_input == INPUT_FIELD_PORT, 0);
    y += INPUT_HEIGHT + 30;

    // Buttons
    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;

    // Connect
    bool hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Connect", hovered);
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Back
    hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Back", hovered);

    // Status message
    if (menu->status_message[0] != '\0') {
        Color status_color = menu->status_is_error ? COLOR_STATUS_ERROR : COLOR_STATUS_OK;
        int msg_width = MeasureText(menu->status_message, 14);
        DrawText(menu->status_message, panel_x + (PANEL_WIDTH - msg_width) / 2,
                 panel_y + PANEL_HEIGHT - 40, 14, status_color);
    }
}

static void draw_connecting(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    // Title
    const char* title = "CONNECTING";
    int title_font_size = 32;
    int title_width = MeasureText(title, title_font_size);
    DrawText(title, panel_x + (PANEL_WIDTH - title_width) / 2, panel_y + 50, title_font_size, WHITE);

    // Animated dots
    int dot_count = ((int)(GetTime() * 3)) % 4;
    char dots[8] = "";
    for (int i = 0; i < dot_count; i++) strcat(dots, ".");

    char connecting_text[128];
    snprintf(connecting_text, sizeof(connecting_text), "Connecting to %s:%s%s",
             menu->ip_input, menu->port_input, dots);
    int text_width = MeasureText(connecting_text, 16);
    DrawText(connecting_text, panel_x + (PANEL_WIDTH - text_width) / 2, panel_y + 150, 16, COLOR_LABEL);

    // Cancel button
    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
    int button_y = panel_y + 250;
    bool hovered = is_point_in_rect(mouse_x, mouse_y, button_x, button_y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, button_y, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel", hovered);

    // Status message
    if (menu->status_message[0] != '\0') {
        Color status_color = menu->status_is_error ? COLOR_STATUS_ERROR : COLOR_STATUS_OK;
        int msg_width = MeasureText(menu->status_message, 14);
        DrawText(menu->status_message, panel_x + (PANEL_WIDTH - msg_width) / 2,
                 panel_y + PANEL_HEIGHT - 40, 14, status_color);
    }
}

static void draw_hosting(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    // Title
    const char* title = "HOSTING";
    int title_font_size = 32;
    int title_width = MeasureText(title, title_font_size);
    DrawText(title, panel_x + (PANEL_WIDTH - title_width) / 2, panel_y + 30, title_font_size, WHITE);

    int y = panel_y + 80;

    // Server info
    char port_text[64];
    snprintf(port_text, sizeof(port_text), "Server running on port %s", menu->port_input);
    int port_width = MeasureText(port_text, 14);
    DrawText(port_text, panel_x + (PANEL_WIDTH - port_width) / 2, y, 14, COLOR_STATUS_OK);
    y += 30;

    // Player list header
    DrawText("Players:", panel_x + 50, y, 16, WHITE);
    y += 25;

    // Draw player list
    DrawText("  * You (Host)", panel_x + 60, y, 14, COLOR_LABEL);
    y += 20;

    // Show connected clients from network context
    if (menu->network) {
        int player_count = network_get_player_count(menu->network);
        for (int i = 1; i < NET_MAX_CLIENTS; i++) {
            if (network_is_player_active(menu->network, i)) {
                const char* name = network_get_player_name(menu->network, i);
                char player_text[64];
                snprintf(player_text, sizeof(player_text), "  * %s", name ? name : "Player");
                DrawText(player_text, panel_x + 60, y, 14, COLOR_LABEL);
                y += 20;
            }
        }
    }

    // Buttons
    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
    y = panel_y + 280;

    // Resume
    bool hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Resume Game", hovered);
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Stop Server
    hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Stop Server", hovered);
}

static void draw_connected(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    // Title
    const char* title = "CONNECTED";
    int title_font_size = 32;
    int title_width = MeasureText(title, title_font_size);
    DrawText(title, panel_x + (PANEL_WIDTH - title_width) / 2, panel_y + 30, title_font_size, WHITE);

    int y = panel_y + 80;

    // Connection info
    char conn_text[128];
    snprintf(conn_text, sizeof(conn_text), "Connected to %s:%s", menu->ip_input, menu->port_input);
    int conn_width = MeasureText(conn_text, 14);
    DrawText(conn_text, panel_x + (PANEL_WIDTH - conn_width) / 2, y, 14, COLOR_STATUS_OK);
    y += 30;

    // Player list header
    DrawText("Players:", panel_x + 50, y, 16, WHITE);
    y += 25;

    // Show all players from network context
    if (menu->network) {
        for (int i = 0; i < NET_MAX_CLIENTS; i++) {
            if (network_is_player_active(menu->network, i)) {
                const char* name = network_get_player_name(menu->network, i);
                char player_text[64];
                if (i == 0) {
                    snprintf(player_text, sizeof(player_text), "  * %s (Host)", name ? name : "Host");
                } else {
                    snprintf(player_text, sizeof(player_text), "  * %s", name ? name : "Player");
                }
                DrawText(player_text, panel_x + 60, y, 14, COLOR_LABEL);
                y += 20;
            }
        }
    }

    // Buttons
    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
    y = panel_y + 280;

    // Resume
    bool hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Resume Game", hovered);
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Disconnect
    hovered = is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    draw_button(button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Disconnect", hovered);
}

// ============================================================================
// INPUT HANDLING FOR EACH STATE
// ============================================================================

static int handle_main_menu_input(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return 0;

    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
    int y = panel_y + 90;

    // Resume
    if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 1;
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Host Game
    if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 3;
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Join Game
    if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 4;
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Exit Game
    if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 2;

    return 0;
}

static int handle_host_setup_input(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    int y = panel_y + 100 + 20;
    int input_x = panel_x + (PANEL_WIDTH - INPUT_WIDTH) / 2;

    // Check if clicking on port input field
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (is_point_in_rect(mouse_x, mouse_y, input_x, y, INPUT_WIDTH, INPUT_HEIGHT)) {
            menu->active_input = INPUT_FIELD_PORT;
        } else {
            menu->active_input = INPUT_FIELD_NONE;
        }
    }

    // Handle text input for port
    if (menu->active_input == INPUT_FIELD_PORT) {
        handle_text_input(menu->port_input, sizeof(menu->port_input), false);
    }

    // Button clicks
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
        y = panel_y + 100 + 20 + INPUT_HEIGHT + 30 + 40;

        // Start Server
        if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 5;
        y += BUTTON_HEIGHT + BUTTON_GAP;

        // Back
        if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 6;
    }

    // Escape to go back
    if (IsKeyPressed(KEY_ESCAPE)) return 6;

    return 0;
}

static int handle_join_setup_input(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    int y = panel_y + 80 + 20;
    int input_x = panel_x + (PANEL_WIDTH - INPUT_WIDTH) / 2;

    // Check if clicking on input fields
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // IP input
        if (is_point_in_rect(mouse_x, mouse_y, input_x, y, INPUT_WIDTH, INPUT_HEIGHT)) {
            menu->active_input = INPUT_FIELD_IP;
        }
        // Port input
        else if (is_point_in_rect(mouse_x, mouse_y, input_x, y + INPUT_HEIGHT + 40, INPUT_WIDTH, INPUT_HEIGHT)) {
            menu->active_input = INPUT_FIELD_PORT;
        } else {
            menu->active_input = INPUT_FIELD_NONE;
        }
    }

    // Handle text input
    if (menu->active_input == INPUT_FIELD_IP) {
        handle_text_input(menu->ip_input, sizeof(menu->ip_input), true);
    } else if (menu->active_input == INPUT_FIELD_PORT) {
        handle_text_input(menu->port_input, sizeof(menu->port_input), false);
    }

    // Tab to switch fields
    if (IsKeyPressed(KEY_TAB)) {
        if (menu->active_input == INPUT_FIELD_IP) {
            menu->active_input = INPUT_FIELD_PORT;
        } else {
            menu->active_input = INPUT_FIELD_IP;
        }
    }

    // Button clicks
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
        y = panel_y + 80 + 20 + INPUT_HEIGHT + 20 + 20 + INPUT_HEIGHT + 30;

        // Connect
        if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 7;
        y += BUTTON_HEIGHT + BUTTON_GAP;

        // Back
        if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 6;
    }

    // Escape to go back
    if (IsKeyPressed(KEY_ESCAPE)) return 6;

    // Enter to connect
    if (IsKeyPressed(KEY_ENTER)) return 7;

    return 0;
}

static int handle_connecting_input(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
    int button_y = panel_y + 250;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (is_point_in_rect(mouse_x, mouse_y, button_x, button_y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
            return 9;  // Cancel
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) return 9;

    return 0;
}

static int handle_hosting_input(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (IsKeyPressed(KEY_ESCAPE)) return 1;  // Resume on ESC
        return 0;
    }

    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
    int y = panel_y + 280;

    // Resume
    if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 1;
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Stop Server
    if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 8;

    return 0;
}

static int handle_connected_input(PauseMenu* menu, int panel_x, int panel_y, int mouse_x, int mouse_y) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (IsKeyPressed(KEY_ESCAPE)) return 1;  // Resume on ESC
        return 0;
    }

    int button_x = panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
    int y = panel_y + 280;

    // Resume
    if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 1;
    y += BUTTON_HEIGHT + BUTTON_GAP;

    // Disconnect
    if (is_point_in_rect(mouse_x, mouse_y, button_x, y, BUTTON_WIDTH, BUTTON_HEIGHT)) return 8;

    return 0;
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

PauseMenu* pause_menu_create(void) {
    PauseMenu* menu = (PauseMenu*)malloc(sizeof(PauseMenu));
    if (!menu) {
        return NULL;
    }

    menu->is_paused = false;
    menu->state = MENU_STATE_MAIN;
    strcpy(menu->ip_input, "localhost");  // Default to localhost
    strcpy(menu->port_input, "7777");     // Default port
    menu->active_input = INPUT_FIELD_NONE;
    menu->input_cursor = 0;
    menu->status_message[0] = '\0';
    menu->status_timer = 0;
    menu->status_is_error = false;
    menu->network = NULL;
    generate_random_name(menu->player_name, sizeof(menu->player_name));

    return menu;
}

void pause_menu_destroy(PauseMenu* menu) {
    if (menu) {
        free(menu);
    }
}

void pause_menu_open(PauseMenu* menu) {
    if (menu) {
        menu->is_paused = true;
    }
}

void pause_menu_close(PauseMenu* menu) {
    if (menu) {
        menu->is_paused = false;
        menu->active_input = INPUT_FIELD_NONE;
    }
}

bool pause_menu_is_open(PauseMenu* menu) {
    return menu && menu->is_paused;
}

void pause_menu_update(PauseMenu* menu, float dt) {
    if (!menu) return;

    // Clear status message after timeout
    if (menu->status_timer > 0) {
        menu->status_timer -= dt;
        if (menu->status_timer <= 0) {
            menu->status_message[0] = '\0';
        }
    }

    // Check network state and update menu state accordingly
    if (menu->network) {
        NetworkMode mode = network_get_mode(menu->network);

        // If we were connecting and now we're connected as client
        if (menu->state == MENU_STATE_CONNECTING && mode == NET_MODE_CLIENT) {
            // Check actual connection state
            if (menu->network->client) {
                NetClientState client_state = net_client_get_state(menu->network->client);
                if (client_state == NET_STATE_CONNECTED) {
                    menu->state = MENU_STATE_CONNECTED;
                    pause_menu_set_status(menu, "Connected!", false);
                } else if (client_state == NET_STATE_ERROR) {
                    menu->state = MENU_STATE_JOIN_SETUP;
                    pause_menu_set_status(menu, net_client_get_error(menu->network->client), true);
                }
            }
        }
    }
}

int pause_menu_handle_input(PauseMenu* menu, int mouse_x, int mouse_y) {
    if (!menu || !menu->is_paused) {
        return 0;
    }

    int panel_x = (SCREEN_WIDTH - PANEL_WIDTH) / 2;
    int panel_y = (SCREEN_HEIGHT - PANEL_HEIGHT) / 2;

    switch (menu->state) {
        case MENU_STATE_MAIN:
            return handle_main_menu_input(menu, panel_x, panel_y, mouse_x, mouse_y);
        case MENU_STATE_HOST_SETUP:
            return handle_host_setup_input(menu, panel_x, panel_y, mouse_x, mouse_y);
        case MENU_STATE_JOIN_SETUP:
            return handle_join_setup_input(menu, panel_x, panel_y, mouse_x, mouse_y);
        case MENU_STATE_CONNECTING:
            return handle_connecting_input(menu, panel_x, panel_y, mouse_x, mouse_y);
        case MENU_STATE_HOSTING:
            return handle_hosting_input(menu, panel_x, panel_y, mouse_x, mouse_y);
        case MENU_STATE_CONNECTED:
            return handle_connected_input(menu, panel_x, panel_y, mouse_x, mouse_y);
    }

    return 0;
}

void pause_menu_draw(PauseMenu* menu) {
    if (!menu || !menu->is_paused) {
        return;
    }

    // Draw semi-transparent overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_OVERLAY);

    int panel_x = (SCREEN_WIDTH - PANEL_WIDTH) / 2;
    int panel_y = (SCREEN_HEIGHT - PANEL_HEIGHT) / 2;

    // Draw panel background
    DrawRectangle(panel_x, panel_y, PANEL_WIDTH, PANEL_HEIGHT, COLOR_PANEL);
    DrawRectangleLines(panel_x, panel_y, PANEL_WIDTH, PANEL_HEIGHT, COLOR_BUTTON_BORDER);

    // Get mouse position
    Vector2 mouse_pos = GetMousePosition();
    int mouse_x = (int)mouse_pos.x;
    int mouse_y = (int)mouse_pos.y;

    // Draw appropriate state
    switch (menu->state) {
        case MENU_STATE_MAIN:
            draw_main_menu(menu, panel_x, panel_y, mouse_x, mouse_y);
            break;
        case MENU_STATE_HOST_SETUP:
            draw_host_setup(menu, panel_x, panel_y, mouse_x, mouse_y);
            break;
        case MENU_STATE_JOIN_SETUP:
            draw_join_setup(menu, panel_x, panel_y, mouse_x, mouse_y);
            break;
        case MENU_STATE_CONNECTING:
            draw_connecting(menu, panel_x, panel_y, mouse_x, mouse_y);
            break;
        case MENU_STATE_HOSTING:
            draw_hosting(menu, panel_x, panel_y, mouse_x, mouse_y);
            break;
        case MENU_STATE_CONNECTED:
            draw_connected(menu, panel_x, panel_y, mouse_x, mouse_y);
            break;
    }
}

void pause_menu_set_network(PauseMenu* menu, NetworkContext* network) {
    if (menu) {
        menu->network = network;
    }
}

void pause_menu_set_status(PauseMenu* menu, const char* message, bool is_error) {
    if (menu && message) {
        strncpy(menu->status_message, message, sizeof(menu->status_message) - 1);
        menu->status_message[sizeof(menu->status_message) - 1] = '\0';
        menu->status_is_error = is_error;
        menu->status_timer = 5.0f;  // Clear after 5 seconds
    }
}

PauseMenuState pause_menu_get_state(PauseMenu* menu) {
    return menu ? menu->state : MENU_STATE_MAIN;
}

void pause_menu_set_state(PauseMenu* menu, PauseMenuState state) {
    if (menu) {
        menu->state = state;
        menu->active_input = INPUT_FIELD_NONE;
    }
}

const char* pause_menu_get_ip(PauseMenu* menu) {
    return menu ? menu->ip_input : "";
}

uint16_t pause_menu_get_port(PauseMenu* menu) {
    if (!menu || menu->port_input[0] == '\0') {
        return NET_DEFAULT_PORT;
    }
    int port = atoi(menu->port_input);
    if (port <= 0 || port > 65535) {
        return NET_DEFAULT_PORT;
    }
    return (uint16_t)port;
}

const char* pause_menu_get_player_name(PauseMenu* menu) {
    return menu ? menu->player_name : "Player";
}
