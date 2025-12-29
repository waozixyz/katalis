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
#include "voxel/item.h"
#include "voxel/inventory_ui.h"
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
    BlockFace target_face;
} GameState;

static GameState g_state;

// ============================================================================
// RAYCASTING FOR BLOCK SELECTION
// ============================================================================

/**
 * Raycast to find the block the player is looking at
 * Returns true if a block is found within max_distance
 * Uses proper DDA algorithm for voxel traversal
 * Also determines which face was hit
 */
static bool raycast_block(World* world, Vector3 origin, Vector3 direction, float max_distance,
                         Vector3* hit_block, BlockFace* hit_face) {
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
    BlockFace face = FACE_TOP;  // Default

    while (t < max_distance) {
        // Check current block
        Block block = world_get_block(world, x, y, z);
        if (block_is_solid(block)) {
            hit_block->x = (float)x;
            hit_block->y = (float)y;
            hit_block->z = (float)z;
            if (hit_face) *hit_face = face;
            return true;
        }

        // Step to next voxel and track which face we crossed
        if (t_max_x < t_max_y && t_max_x < t_max_z) {
            x += step_x;
            t = t_max_x;
            t_max_x += t_delta_x;
            face = (step_x > 0) ? FACE_LEFT : FACE_RIGHT;  // LEFT=-X, RIGHT=+X
        } else if (t_max_y < t_max_z) {
            y += step_y;
            t = t_max_y;
            t_max_y += t_delta_y;
            face = (step_y > 0) ? FACE_BOTTOM : FACE_TOP;
        } else {
            z += step_z;
            t = t_max_z;
            t_max_z += t_delta_z;
            face = (step_z > 0) ? FACE_BACK : FACE_FRONT;  // BACK=-Z, FRONT=+Z
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

    // Initialize item system
    item_system_init();

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
    // Toggle inventory with E key
    if (IsKeyPressed(KEY_E)) {
        g_state.player->inventory->is_open = !g_state.player->inventory->is_open;
    }

    // Hotbar selection (number keys 1-9) - always active
    for (int i = 0; i < 9; i++) {
        if (IsKeyPressed(KEY_ONE + i)) {
            inventory_set_selected_slot(g_state.player->inventory, i);
        }
    }

    // Hotbar selection (mouse scroll wheel) - only when inventory closed
    if (!g_state.player->inventory->is_open) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            int current = g_state.player->inventory->selected_hotbar_slot;
            current -= (int)wheel;  // Scroll down = next slot
            if (current < 0) current = 8;
            if (current > 8) current = 0;
            inventory_set_selected_slot(g_state.player->inventory, current);
        }
    }

    // Update player (handles input, movement, collision, and camera)
    // Only update when inventory is closed
    if (!g_state.player->inventory->is_open) {
        player_update(g_state.player, g_state.world, dt);
    }

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
        &g_state.target_block_pos,
        &g_state.target_face
    );

    // Mine/delete block on left click (only when inventory closed)
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !g_state.player->inventory->is_open) {
        if (g_state.has_target_block) {
            int x = (int)g_state.target_block_pos.x;
            int y = (int)g_state.target_block_pos.y;
            int z = (int)g_state.target_block_pos.z;

            // Get the block being mined
            Block block = world_get_block(g_state.world, x, y, z);

            // Calculate drops
            ItemStack drop = item_get_block_drop(block.type);

            if (drop.type != ITEM_NONE) {
                // Try to add to inventory
                if (inventory_add_item(g_state.player->inventory, drop.type, drop.count)) {
                    // Success - remove block
                    Block air_block = {BLOCK_AIR, 0, 0};
                    world_set_block(g_state.world, x, y, z, air_block);

                    const char* item_name = item_get_name(drop.type);
                    printf("[MINED] Block at (%d, %d, %d) - Got %d x %s\n",
                           x, y, z, drop.count, item_name);
                } else {
                    printf("[INVENTORY FULL] Cannot mine block - inventory is full\n");
                }
            } else {
                // Block has no drop (leaves, water, etc.) - still remove it
                Block air_block = {BLOCK_AIR, 0, 0};
                world_set_block(g_state.world, x, y, z, air_block);
                printf("[MINED] Block at (%d, %d, %d) - No drop\n", x, y, z);
            }
        } else {
            printf("[MISS] No block in crosshair\n");
        }
    }

    // Place block on right click (only when inventory closed)
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !g_state.player->inventory->is_open) {
        if (g_state.has_target_block) {
            ItemStack* selected = inventory_get_selected_hotbar_item(g_state.player->inventory);

            if (selected && selected->type != ITEM_NONE) {
                const ItemProperties* props = item_get_properties(selected->type);

                if (props->is_placeable) {
                    // Calculate placement position (adjacent to hit face)
                    Vector3 place_pos = g_state.target_block_pos;

                    switch (g_state.target_face) {
                        case FACE_TOP:    place_pos.y += 1.0f; break;
                        case FACE_BOTTOM: place_pos.y -= 1.0f; break;
                        case FACE_FRONT:  place_pos.z += 1.0f; break;
                        case FACE_BACK:   place_pos.z -= 1.0f; break;
                        case FACE_RIGHT:  place_pos.x += 1.0f; break;
                        case FACE_LEFT:   place_pos.x -= 1.0f; break;
                        default: break;
                    }

                    // Check if placement position collides with player
                    if (!player_collides_with_position(g_state.player, place_pos)) {
                        // Place block
                        Block new_block = {props->places_as, 0, 0};
                        world_set_block(g_state.world,
                            (int)place_pos.x,
                            (int)place_pos.y,
                            (int)place_pos.z,
                            new_block);

                        // Consume item from inventory
                        int slot_index = g_state.player->inventory->selected_hotbar_slot;
                        inventory_remove_item(g_state.player->inventory, slot_index, 1);

                        printf("[PLACED] %s at (%d, %d, %d)\n",
                            props->name,
                            (int)place_pos.x,
                            (int)place_pos.y,
                            (int)place_pos.z);
                    } else {
                        printf("[BLOCKED] Cannot place block inside player\n");
                    }
                }
            }
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

    // Draw hotbar (always visible)
    Texture2D atlas = texture_atlas_get_texture();
    inventory_ui_draw_hotbar(g_state.player->inventory, atlas);
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
