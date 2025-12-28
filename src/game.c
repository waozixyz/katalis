/**
 * Game - Love2D-Style Lifecycle
 *
 * Implementation of game-specific logic with init/update/draw hooks
 */

#include "game.h"
#include <raylib.h>
#include <stdbool.h>

// ============================================================================
// GAME STATE
// ============================================================================

static bool g_initialized = false;

typedef struct {
    float player_x;
    float player_y;
    float player_speed;
} GameState;

static GameState g_state;

// ============================================================================
// LIFECYCLE HOOKS (Internal)
// ============================================================================

/**
 * Initialize game - called once on first frame
 */
static void game_init(void) {
    // Initialize game state
    g_state.player_x = 400.0f;
    g_state.player_y = 300.0f;
    g_state.player_speed = 200.0f; // pixels per second

    // Load resources here
    // LoadTexture(), LoadSound(), etc.
}

/**
 * Update game logic - called every frame with delta time
 */
static void game_update(float dt) {
    // Input handling - frame-independent movement
    if (IsKeyDown(KEY_RIGHT)) g_state.player_x += g_state.player_speed * dt;
    if (IsKeyDown(KEY_LEFT))  g_state.player_x -= g_state.player_speed * dt;
    if (IsKeyDown(KEY_DOWN))  g_state.player_y += g_state.player_speed * dt;
    if (IsKeyDown(KEY_UP))    g_state.player_y -= g_state.player_speed * dt;

    // Game logic, physics, AI here
}

/**
 * Render game - renderer-agnostic (works on Raylib + SDL3)
 */
static void game_draw(void) {
    // General drawing that works everywhere
    DrawText("Use arrow keys to move", 50, 50, 20, WHITE);
    DrawCircle((int)g_state.player_x, (int)g_state.player_y, 25, YELLOW);

    // UI overlays
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, LIME);
    DrawText(TextFormat("Pos: (%.0f, %.0f)", g_state.player_x, g_state.player_y),
             10, 550, 20, GRAY);
}

// ============================================================================
// MAIN ENTRY POINT (Public)
// ============================================================================

/**
 * Game entry point - called every frame by render callback
 */
void game_run(uint32_t component_id) {
    (void)component_id; // Unused, suppress warning

    // One-time initialization
    if (!g_initialized) {
        game_init();
        g_initialized = true;
    }

    // Every frame: update then draw
    float dt = GetFrameTime(); // Delta time in seconds
    game_update(dt);
    game_draw();

 
}
