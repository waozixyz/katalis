/**
 * Game - Love2D-Style Lifecycle
 *
 * Implementation of game-specific logic with init/update/draw hooks
 */

#include "game.h"
#include "voxel/block.h"
#include "voxel/world.h"
#include "voxel/noise.h"
#include "voxel/terrain.h"
#include "voxel/player.h"
#include "voxel/texture_atlas.h"
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

// ============================================================================
// GAME STATE
// ============================================================================

static bool g_initialized = false;

typedef struct {
    World* world;
    Player* player;
} GameState;

static GameState g_state;

// ============================================================================
// LIFECYCLE HOOKS (Internal)
// ============================================================================

/**
 * Initialize game - called once on first frame
 */
static void game_init(void) {
    printf("[GAME] Initializing voxel world...\n");

    // Initialize block system
    block_system_init();

    // Initialize texture atlas
    texture_atlas_init();

    // Initialize noise with random seed
    uint32_t seed = (uint32_t)time(NULL);
    noise_init(seed);
    printf("[GAME] Using world seed: %u\n", seed);

    // Create world
    g_state.world = world_create();

    // Setup terrain parameters
    TerrainParams terrain_params = terrain_default_params();
    terrain_params.height_scale = 24.0f;        // Moderate hills
    terrain_params.height_offset = 32.0f;       // Sea level at y=32
    terrain_params.generate_caves = true;       // Enable caves
    terrain_params.cave_threshold = 0.35f;      // Cave density

    // Generate procedural terrain
    printf("[GAME] Generating procedural terrain...\n");
    for (int cx = -3; cx <= 3; cx++) {
        for (int cz = -3; cz <= 3; cz++) {
            // Get or create chunk
            Chunk* chunk = world_get_or_create_chunk(g_state.world, cx, cz);

            // Generate terrain using noise
            terrain_generate_chunk(chunk, terrain_params);

            // Generate mesh for this chunk
            chunk_generate_mesh(chunk);
        }
    }

    // Create player at spawn position
    Vector3 spawn_position = {0.0f, 60.0f, 0.0f};  // Above terrain
    g_state.player = player_create(spawn_position);

    // Enable mouse cursor lock for FPS controls
    DisableCursor();

    printf("[GAME] Procedural world initialized with %d chunks!\n", g_state.world->chunks->chunk_count);
}

/**
 * Update game logic - called every frame with delta time
 */
static void game_update(float dt) {
    // Update player (handles input, movement, and camera)
    player_update(g_state.player, dt);

    // Update world (chunk loading/unloading based on player position)
    int player_chunk_x, player_chunk_z;
    world_to_chunk_coords((int)g_state.player->position.x, (int)g_state.player->position.z,
                          &player_chunk_x, &player_chunk_z);
    world_update(g_state.world, player_chunk_x, player_chunk_z);

    // ESC to unlock cursor (for debugging/menu)
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (IsCursorHidden()) {
            EnableCursor();
        } else {
            DisableCursor();
        }
    }
}

/**
 * Render game - renderer-agnostic (works on Raylib + SDL3)
 */
static void game_draw(void) {
    // Clear background
    ClearBackground(SKYBLUE);

    // 3D rendering with player camera
    Camera3D camera = player_get_camera(g_state.player);
    BeginMode3D(camera);

    // Draw grid for reference
    DrawGrid(100, 1.0f);

    // Draw all chunks in the world
    world_render(g_state.world);

    EndMode3D();

    // 2D UI overlay
    DrawText("Voxel Engine - Phase 5: First-Person Camera", 10, 10, 20, WHITE);
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 40, 20, LIME);
    DrawText(TextFormat("Chunks: %d", g_state.world ? g_state.world->chunks->chunk_count : 0), 10, 70, 20, WHITE);
    DrawText(TextFormat("Position: (%.1f, %.1f, %.1f)",
             g_state.player->position.x,
             g_state.player->position.y,
             g_state.player->position.z), 10, 100, 20, WHITE);
    DrawText(TextFormat("Mode: %s", g_state.player->is_flying ? "Flying" : "Walking"), 10, 130, 20, YELLOW);

    // Controls
    DrawText("WASD: Move | Mouse: Look | Space/Ctrl: Up/Down | Shift: Sprint", 10, 540, 16, GRAY);
    DrawText("F: Toggle Flying | ESC: Toggle Cursor", 10, 560, 16, GRAY);
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
