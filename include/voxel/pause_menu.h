/**
 * Pause Menu System
 *
 * Provides a pause menu overlay with game controls and LAN multiplayer options.
 * Supports hosting games, joining games, and managing network connections.
 */

#ifndef VOXEL_PAUSE_MENU_H
#define VOXEL_PAUSE_MENU_H

#include <stdbool.h>
#include <stdint.h>

// Forward declaration
typedef struct NetworkContext NetworkContext;

// ============================================================================
// MENU STATES
// ============================================================================

typedef enum {
    MENU_STATE_MAIN,            // Main menu: Resume, Host, Join, Exit
    MENU_STATE_HOST_SETUP,      // Host setup: Port input + Start
    MENU_STATE_JOIN_SETUP,      // Join setup: IP + Port input + Connect
    MENU_STATE_CONNECTING,      // Connecting: "Connecting..." + Cancel
    MENU_STATE_HOSTING,         // Hosting: Player list + Stop Server
    MENU_STATE_CONNECTED,       // Connected: Player list + Disconnect
} PauseMenuState;

// ============================================================================
// INPUT FIELD
// ============================================================================

#define PAUSE_MENU_INPUT_MAX 64

typedef enum {
    INPUT_FIELD_NONE,
    INPUT_FIELD_IP,
    INPUT_FIELD_PORT,
} InputFieldType;

// ============================================================================
// PAUSE MENU
// ============================================================================

typedef struct {
    bool is_paused;
    PauseMenuState state;

    // Text input fields
    char ip_input[PAUSE_MENU_INPUT_MAX];
    char port_input[8];
    InputFieldType active_input;
    int input_cursor;

    // Status and error messages
    char status_message[128];
    float status_timer;         // Auto-clear after timeout
    bool status_is_error;

    // Network reference (set by game.c)
    NetworkContext* network;

    // Player name for joining
    char player_name[32];
} PauseMenu;

/**
 * Create a new pause menu
 * Returns a pointer to the pause menu, or NULL on failure
 */
PauseMenu* pause_menu_create(void);

/**
 * Destroy a pause menu and free its resources
 */
void pause_menu_destroy(PauseMenu* menu);

/**
 * Open the pause menu (pause the game)
 */
void pause_menu_open(PauseMenu* menu);

/**
 * Close the pause menu (resume the game)
 */
void pause_menu_close(PauseMenu* menu);

/**
 * Check if the pause menu is currently open
 */
bool pause_menu_is_open(PauseMenu* menu);

/**
 * Handle input for the pause menu (mouse and keyboard)
 * Returns action code:
 *   0 = No action
 *   1 = Resume game
 *   2 = Exit game
 *   3 = Host Game button (go to HOST_SETUP)
 *   4 = Join Game button (go to JOIN_SETUP)
 *   5 = Start Server (begin hosting)
 *   6 = Back (return to main menu)
 *   7 = Connect (attempt connection)
 *   8 = Disconnect/Stop Server
 *   9 = Cancel connecting
 */
int pause_menu_handle_input(PauseMenu* menu, int mouse_x, int mouse_y);

/**
 * Update pause menu (timers, connection status)
 */
void pause_menu_update(PauseMenu* menu, float dt);

/**
 * Draw the pause menu overlay
 */
void pause_menu_draw(PauseMenu* menu);

/**
 * Set network context reference
 */
void pause_menu_set_network(PauseMenu* menu, NetworkContext* network);

/**
 * Set status message (displayed in menu)
 */
void pause_menu_set_status(PauseMenu* menu, const char* message, bool is_error);

/**
 * Get current menu state
 */
PauseMenuState pause_menu_get_state(PauseMenu* menu);

/**
 * Set menu state
 */
void pause_menu_set_state(PauseMenu* menu, PauseMenuState state);

/**
 * Get IP input value
 */
const char* pause_menu_get_ip(PauseMenu* menu);

/**
 * Get port input value as integer
 */
uint16_t pause_menu_get_port(PauseMenu* menu);

/**
 * Get player name
 */
const char* pause_menu_get_player_name(PauseMenu* menu);

#endif // VOXEL_PAUSE_MENU_H
