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
#include <float.h>

// ============================================================================
// GAME STATE
// ============================================================================

static bool g_initialized = false;

typedef struct {
    World* world;
    Player* player;
    bool has_target_block;
    Vector3 target_block_pos;
} GameState;

static GameState g_state;

// ============================================================================
// RAYCASTING FOR BLOCK SELECTION
// ============================================================================

/**
 * Raycast to find the block the player is looking at
 * Returns true if a block is found within max_distance
 * Uses proper DDA algorithm for voxel traversal
 */
static bool raycast_block(World* world, Vector3 origin, Vector3 direction, float max_distance, Vector3* hit_block) {
    // Normalize direction
    direction = Vector3Normalize(direction);

    // Current voxel position
    int x = (int)floorf(origin.x);
    int y = (int)floorf(origin.y);
    int z = (int)floorf(origin.z);

    // Direction to step in each axis (-1, 0, or 1)
    int step_x = (direction.x > 0) ? 1 : -1;
    int step_y = (direction.y > 0) ? 1 : -1;
    int step_z = (direction.z > 0) ? 1 : -1;

    // Distance to next voxel boundary in each axis
    float t_delta_x = (direction.x != 0) ? fabsf(1.0f / direction.x) : FLT_MAX;
    float t_delta_y = (direction.y != 0) ? fabsf(1.0f / direction.y) : FLT_MAX;
    float t_delta_z = (direction.z != 0) ? fabsf(1.0f / direction.z) : FLT_MAX;

    // Calculate initial t_max values
    float t_max_x, t_max_y, t_max_z;

    if (direction.x > 0) {
        t_max_x = ((float)(x + 1) - origin.x) / direction.x;
    } else if (direction.x < 0) {
        t_max_x = (origin.x - (float)x) / -direction.x;
    } else {
        t_max_x = FLT_MAX;
    }

    if (direction.y > 0) {
        t_max_y = ((float)(y + 1) - origin.y) / direction.y;
    } else if (direction.y < 0) {
        t_max_y = (origin.y - (float)y) / -direction.y;
    } else {
        t_max_y = FLT_MAX;
    }

    if (direction.z > 0) {
        t_max_z = ((float)(z + 1) - origin.z) / direction.z;
    } else if (direction.z < 0) {
        t_max_z = (origin.z - (float)z) / -direction.z;
    } else {
        t_max_z = FLT_MAX;
    }

    // DDA traversal
    float t = 0.0f;
    while (t < max_distance) {
        // Check current block
        Block block = world_get_block(world, x, y, z);
        if (block_is_solid(block)) {
            hit_block->x = (float)x;
            hit_block->y = (float)y;
            hit_block->z = (float)z;
            return true;
        }

        // Step to next voxel
        if (t_max_x < t_max_y && t_max_x < t_max_z) {
            x += step_x;
            t = t_max_x;
            t_max_x += t_delta_x;
        } else if (t_max_y < t_max_z) {
            y += step_y;
            t = t_max_y;
            t_max_y += t_delta_y;
        } else {
            z += step_z;
            t = t_max_z;
            t_max_z += t_delta_z;
        }
    }

    return false;
}

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

    // Initialize target block state
    g_state.has_target_block = false;
    g_state.target_block_pos = (Vector3){0, 0, 0};

    // Enable mouse cursor lock for FPS controls
    DisableCursor();

    printf("[GAME] Procedural world initialized with %d chunks!\n", g_state.world->chunks->chunk_count);
}

/**
 * Update game logic - called every frame with delta time
 */
static void game_update(float dt) {
    // Update player (handles input, movement, collision, and camera)
    player_update(g_state.player, g_state.world, dt);

    // Update world (chunk loading/unloading based on player position)
    int player_chunk_x, player_chunk_z;
    world_to_chunk_coords((int)g_state.player->position.x, (int)g_state.player->position.z,
                          &player_chunk_x, &player_chunk_z);
    world_update(g_state.world, player_chunk_x, player_chunk_z);

    // Raycast to find block player is looking at
    Camera3D camera = player_get_camera(g_state.player);
    Vector3 camera_direction = Vector3Subtract(camera.target, camera.position);
    camera_direction = Vector3Normalize(camera_direction);

    g_state.has_target_block = raycast_block(
        g_state.world,
        camera.position,
        camera_direction,
        5.0f,  // Max reach distance
        &g_state.target_block_pos
    );

    // Mine/delete block on left click
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (g_state.has_target_block) {
            int x = (int)g_state.target_block_pos.x;
            int y = (int)g_state.target_block_pos.y;
            int z = (int)g_state.target_block_pos.z;

            // Delete the block (set to air)
            Block air_block = {BLOCK_AIR, 0, 0};
            world_set_block(g_state.world, x, y, z, air_block);

            printf("[MINED] Block at (%d, %d, %d)\n", x, y, z);
        } else {
            printf("[MISS] No block in crosshair (camera pos: %.1f, %.1f, %.1f)\n",
                   camera.position.x, camera.position.y, camera.position.z);
        }
    }

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

    // Draw all chunks in the world
    world_render(g_state.world);

    // Draw wireframe around targeted block
    if (g_state.has_target_block) {
        Vector3 block_pos = g_state.target_block_pos;
        Vector3 cube_center = {block_pos.x + 0.5f, block_pos.y + 0.5f, block_pos.z + 0.5f};
        Vector3 cube_size = {1.01f, 1.01f, 1.01f};  // Slightly larger than block

        DrawCubeWires(cube_center, cube_size.x, cube_size.y, cube_size.z, BLACK);
        DrawCubeWires(cube_center, cube_size.x * 0.99f, cube_size.y * 0.99f, cube_size.z * 0.99f, WHITE);
    }

    EndMode3D();

    // 2D UI overlay
    DrawText("Katalis - Voxel Game", 10, 10, 20, WHITE);
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 40, 20, LIME);
    DrawText(TextFormat("Chunks: %d", g_state.world ? g_state.world->chunks->chunk_count : 0), 10, 70, 20, WHITE);
    DrawText(TextFormat("Position: (%.1f, %.1f, %.1f)",
             g_state.player->position.x,
             g_state.player->position.y,
             g_state.player->position.z), 10, 100, 20, WHITE);
    DrawText(TextFormat("Mode: %s", g_state.player->is_flying ? "Flying" : "Walking"), 10, 130, 20, YELLOW);

    // Target block indicator
    if (g_state.has_target_block) {
        DrawText(TextFormat("Target: [%d, %d, %d]",
                 (int)g_state.target_block_pos.x,
                 (int)g_state.target_block_pos.y,
                 (int)g_state.target_block_pos.z), 10, 160, 20, GREEN);
    } else {
        DrawText("Target: NONE", 10, 160, 20, RED);
    }

    // Controls
    DrawText("WASD: Move | Mouse: Look | Space: Jump | Shift: Sprint", 10, 540, 16, GRAY);
    DrawText("F: Toggle Flying | Left Click: Mine Block | ESC: Toggle Cursor", 10, 560, 16, GRAY);

    // Draw crosshair in center of screen
    int screen_width = 800;   // From window size
    int screen_height = 600;
    int center_x = screen_width / 2;
    int center_y = screen_height / 2;
    int crosshair_size = 10;
    int crosshair_thickness = 2;

    // Horizontal line
    DrawRectangle(center_x - crosshair_size, center_y - crosshair_thickness / 2,
                  crosshair_size * 2, crosshair_thickness, WHITE);
    // Vertical line
    DrawRectangle(center_x - crosshair_thickness / 2, center_y - crosshair_size,
                  crosshair_thickness, crosshair_size * 2, WHITE);

    // Draw small dot in center
    DrawCircle(center_x, center_y, 2, WHITE);
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
