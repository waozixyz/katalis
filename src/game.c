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
#include "voxel/inventory_input.h"
#include "voxel/crafting.h"
#include "voxel/pause_menu.h"
#include "voxel/entity.h"
#include "voxel/block_human.h"
#include "voxel/sky.h"
#include "voxel/tree.h"
#include "voxel/network.h"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
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
    EntityManager* entity_manager;
    bool has_target_block;
    Vector3 target_block_pos;
    BlockFace target_face;
    float flying_message_timer;  // Timer for flying mode notification
    float view_mode_message_timer;  // Timer for view mode notification
    PauseMenu* pause_menu;       // Pause menu state
    // Day/night system
    float time_of_day;           // 0.0 to 24.0 hours
    float day_speed;             // Hours per real second (default: 0.5)
    bool time_paused;            // Debug: pause time
    // Network
    NetworkContext* network;     // LAN multiplayer context
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

    // Initialize crafting system
    crafting_init();

    // Initialize noise with random seed
    uint32_t seed = (uint32_t)time(NULL);
    noise_init(seed);
    printf("[GAME] Using world seed: %u\n", seed);

    // Setup terrain parameters
    TerrainParams terrain_params = terrain_default_params();
    terrain_params.height_scale = 24.0f;        // Moderate hills
    terrain_params.height_offset = 32.0f;       // Sea level at y=32
    terrain_params.generate_caves = true;       // Enable caves
    terrain_params.cave_threshold = 0.35f;      // Cave density

    // Create world with terrain parameters
    g_state.world = world_create(terrain_params);

    // Generate procedural terrain
    printf("[GAME] Generating procedural terrain...\n");
    int chunks_generated = 0;
    int chunks_empty = 0;
    for (int cx = -3; cx <= 3; cx++) {
        for (int cz = -3; cz <= 3; cz++) {
            // Get or create chunk
            Chunk* chunk = world_get_or_create_chunk(g_state.world, cx, cz);

            // Generate terrain using noise
            terrain_generate_chunk(chunk, terrain_params);

            // Update empty status after terrain generation
            chunk_update_empty_status(chunk);

            // Generate mesh for this chunk
            chunk_generate_mesh(chunk);

            chunks_generated++;
            if (chunk->is_empty) {
                chunks_empty++;
            } else {
                printf("[GAME] Chunk (%d, %d): mesh has %d vertices\n",
                       cx, cz, chunk->mesh.vertexCount);
            }
        }
    }
    printf("[GAME] Generated %d chunks (%d empty, %d with blocks)\n",
           chunks_generated, chunks_empty, chunks_generated - chunks_empty);

    // Create player at spawn position (high up to see terrain)
    Vector3 spawn_position = {0.0f, 200.0f, 0.0f};  // Very high above terrain
    g_state.player = player_create(spawn_position);

    // Start in walking mode (toggle with Shift)
    g_state.player->is_flying = false;
    printf("[GAME] Player spawned at (%.1f, %.1f, %.1f)\n",
           spawn_position.x, spawn_position.y, spawn_position.z);

    // Give player starting items for testing
    inventory_add_item(g_state.player->inventory, ITEM_WOOD_LOG, 16);
    inventory_add_item(g_state.player->inventory, ITEM_DIRT, 64);
    inventory_add_item(g_state.player->inventory, ITEM_COBBLESTONE, 32);
    printf("[GAME] Added starting items to inventory\n");

    // Initialize target block state
    g_state.has_target_block = false;
    g_state.target_block_pos = (Vector3){0, 0, 0};
    g_state.flying_message_timer = 0.0f;
    g_state.view_mode_message_timer = 0.0f;

    // Enable mouse cursor lock for FPS controls
    DisableCursor();

    // Initialize pause menu
    g_state.pause_menu = pause_menu_create();
    printf("[GAME] Pause menu initialized\n");

    // Initialize network
    g_state.network = network_create();
    pause_menu_set_network(g_state.pause_menu, g_state.network);
    printf("[GAME] Network system initialized\n");

    // Initialize day/night system
    g_state.time_of_day = 6.0f;      // Start at dawn
    g_state.day_speed = 0.5f;        // 48 second full day
    g_state.time_paused = false;
    printf("[GAME] Day/night system initialized (starting at %.1f hours)\n", g_state.time_of_day);

    // Initialize sky rendering
    sky_init();
    printf("[GAME] Sky rendering initialized\n");

    // Initialize leaf decay system
    leaf_decay_init();

    // Initialize entity system
    g_state.entity_manager = entity_manager_create();

    // Spawn test block humans at different positions
    Vector3 human_pos1 = {5.0f, 70.0f, 0.0f};   // To the right
    Vector3 human_pos2 = {-5.0f, 70.0f, 0.0f};  // To the left
    Vector3 human_pos3 = {0.0f, 70.0f, 5.0f};   // In front

    block_human_spawn(g_state.entity_manager, human_pos1);  // Default colors
    block_human_spawn_colored(g_state.entity_manager, human_pos2,
                              (Color){200, 150, 100, 255},  // Different skin tone
                              (Color){180, 50, 50, 255},    // Red shirt
                              (Color){50, 50, 150, 255});   // Blue pants
    block_human_spawn_colored(g_state.entity_manager, human_pos3,
                              (Color){255, 200, 150, 255},  // Light skin
                              (Color){60, 150, 60, 255},    // Green shirt
                              (Color){80, 80, 80, 255});    // Gray pants

    printf("[GAME] Spawned %d entities\n", entity_manager_get_count(g_state.entity_manager));

    printf("[GAME] Procedural world initialized with %d chunks!\n", g_state.world->chunks->chunk_count);
}

/**
 * Update game logic - called every frame with delta time
 */
static void game_update(float dt) {
    // Block ALL game input when pause menu is open (except ESC which is handled separately)
    bool menu_blocking_input = pause_menu_is_open(g_state.pause_menu);

    // Toggle inventory with E key (only when pause menu closed)
    if (!menu_blocking_input && IsKeyPressed(KEY_E)) {
        g_state.player->inventory->is_open = !g_state.player->inventory->is_open;

        // Show cursor when inventory is open, hide when closed
        if (g_state.player->inventory->is_open) {
            EnableCursor();
        } else {
            DisableCursor();
        }
    }

    // Hotbar selection (number keys 1-9) - only when pause menu closed
    if (!menu_blocking_input) {
        for (int i = 0; i < 9; i++) {
            if (IsKeyPressed(KEY_ONE + i)) {
                inventory_set_selected_slot(g_state.player->inventory, i);
            }
        }
    }

    // Hotbar selection (mouse scroll wheel) - only when inventory and pause menu closed
    if (!g_state.player->inventory->is_open && !menu_blocking_input) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            int current = g_state.player->inventory->selected_hotbar_slot;
            current -= (int)wheel;  // Scroll down = next slot
            if (current < 0) current = 8;
            if (current > 8) current = 0;
            inventory_set_selected_slot(g_state.player->inventory, current);
        }
    }

    // Toggle flying mode with Shift key (only when pause menu closed)
    if (!menu_blocking_input && (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT))) {
        g_state.player->is_flying = !g_state.player->is_flying;
        g_state.flying_message_timer = 2.0f;  // Show message for 2 seconds
        printf("[GAME] Flying mode %s\n", g_state.player->is_flying ? "ENABLED" : "DISABLED");
    }

    // Update flying message timer
    if (g_state.flying_message_timer > 0.0f) {
        g_state.flying_message_timer -= dt;
    }

    // Toggle view mode with V key (show notification) - only when pause menu closed
    if (!menu_blocking_input && IsKeyPressed(KEY_V)) {
        g_state.view_mode_message_timer = 2.0f;  // Show message for 2 seconds
    }

    // Update view mode message timer
    if (g_state.view_mode_message_timer > 0.0f) {
        g_state.view_mode_message_timer -= dt;
    }

    // Update time of day
    if (!g_state.time_paused) {
        g_state.time_of_day += g_state.day_speed * dt;
        if (g_state.time_of_day >= 24.0f) {
            g_state.time_of_day -= 24.0f;
        }
    }

    // Debug: Toggle time pause with T key - only when pause menu closed
    if (!menu_blocking_input && IsKeyPressed(KEY_T)) {
        g_state.time_paused = !g_state.time_paused;
        printf("[TIME] %s at %.2f hours\n",
               g_state.time_paused ? "PAUSED" : "RESUMED",
               g_state.time_of_day);
    }

    // Debug: Speed up/slow down time with +/- keys - only when pause menu closed
    if (!menu_blocking_input && IsKeyPressed(KEY_EQUAL)) {  // + key
        g_state.day_speed *= 2.0f;
        printf("[TIME] Speed increased to %.1fx\n", g_state.day_speed);
    }
    if (!menu_blocking_input && IsKeyPressed(KEY_MINUS)) {
        g_state.day_speed /= 2.0f;
        if (g_state.day_speed < 0.1f) g_state.day_speed = 0.1f;
        printf("[TIME] Speed decreased to %.1fx\n", g_state.day_speed);
    }

    // Update player (handles input, movement, collision, and camera)
    // Only full update when inventory is closed AND pause menu is closed
    if (!g_state.player->inventory->is_open && !pause_menu_is_open(g_state.pause_menu)) {
        player_update(g_state.player, g_state.world, dt);
    } else {
        // Menu is open - still apply physics (gravity) but no input
        // Like Minecraft: world keeps running, you can still fall
        player_update_physics(g_state.player, g_state.world, dt);
    }

    // Update world (chunk loading/unloading based on player position)
    int player_chunk_x, player_chunk_z;
    world_to_chunk_coords((int)g_state.player->position.x, (int)g_state.player->position.z,
                          &player_chunk_x, &player_chunk_z);
    world_update(g_state.world, player_chunk_x, player_chunk_z);

    // Update all entities
    entity_manager_update(g_state.entity_manager, (struct World*)g_state.world, dt);

    // Update leaf decay
    leaf_decay_update(g_state.world, dt);

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

    // Mine/delete block on left click (only when inventory closed and not paused)
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !g_state.player->inventory->is_open && !pause_menu_is_open(g_state.pause_menu)) {
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

                    // Broadcast block change to network
                    network_broadcast_block_change(g_state.network, x, y, z, BLOCK_AIR, 0);

                    // If wood was removed, trigger leaf decay
                    if (block.type == BLOCK_WOOD) {
                        leaf_decay_on_wood_removed(g_state.world, x, y, z);
                    }
                }
            } else {
                // Block has no drop (leaves, water, etc.) - still remove it
                Block air_block = {BLOCK_AIR, 0, 0};
                world_set_block(g_state.world, x, y, z, air_block);

                // Broadcast block change to network
                network_broadcast_block_change(g_state.network, x, y, z, BLOCK_AIR, 0);
            }
        }
    }

    // Place block on right click (only when inventory closed and not paused)
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !g_state.player->inventory->is_open && !pause_menu_is_open(g_state.pause_menu)) {
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

                        // Broadcast block change to network
                        network_broadcast_block_change(g_state.network,
                            (int)place_pos.x,
                            (int)place_pos.y,
                            (int)place_pos.z,
                            props->places_as, 0);

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

    // Inventory mouse interaction (only when inventory is open)
    if (g_state.player->inventory->is_open) {
        Vector2 mouse_pos = GetMousePosition();
        int mouse_x = (int)mouse_pos.x;
        int mouse_y = (int)mouse_pos.y;

        // Shift+Left-click: Quick transfer
        if (IsKeyDown(KEY_LEFT_SHIFT) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            inventory_input_handle_shift_click(g_state.player->inventory, mouse_x, mouse_y);
        }
        // Left-click: Pick up/swap items
        else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            inventory_input_handle_left_click(g_state.player->inventory, mouse_x, mouse_y);
        }
        // Right-click: Pick up/place half stack
        else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            inventory_input_handle_right_click(g_state.player->inventory, mouse_x, mouse_y);
        }
    }

    // Update pause menu timers and connection state
    pause_menu_update(g_state.pause_menu, dt);

    // Update network (always, even when paused)
    network_update(g_state.network, dt);

    // Pause menu interaction (only when pause menu is open)
    if (pause_menu_is_open(g_state.pause_menu)) {
        Vector2 mouse_pos = GetMousePosition();
        int action = pause_menu_handle_input(
            g_state.pause_menu,
            (int)mouse_pos.x,
            (int)mouse_pos.y
        );

        switch (action) {
            case 1:  // Resume
                pause_menu_close(g_state.pause_menu);
                DisableCursor();
                break;

            case 2:  // Exit Game
                network_disconnect(g_state.network);
                CloseWindow();
                break;

            case 3:  // Host Game button - go to setup
                pause_menu_set_state(g_state.pause_menu, MENU_STATE_HOST_SETUP);
                break;

            case 4:  // Join Game button - go to setup
                pause_menu_set_state(g_state.pause_menu, MENU_STATE_JOIN_SETUP);
                break;

            case 5: {  // Start Server
                uint16_t port = pause_menu_get_port(g_state.pause_menu);
                const char* host_name = pause_menu_get_player_name(g_state.pause_menu);
                if (network_host(g_state.network, port, host_name, g_state.world, g_state.player,
                                g_state.entity_manager, &g_state.time_of_day, &g_state.day_speed)) {
                    pause_menu_set_state(g_state.pause_menu, MENU_STATE_HOSTING);
                    pause_menu_set_status(g_state.pause_menu, "Server started!", false);
                    printf("[NETWORK] Hosting as '%s' on port %d\n", host_name, port);
                } else {
                    pause_menu_set_status(g_state.pause_menu, "Failed to start server", true);
                }
                break;
            }

            case 6:  // Back
                pause_menu_set_state(g_state.pause_menu, MENU_STATE_MAIN);
                break;

            case 7: {  // Connect
                const char* ip = pause_menu_get_ip(g_state.pause_menu);
                uint16_t port = pause_menu_get_port(g_state.pause_menu);
                const char* name = pause_menu_get_player_name(g_state.pause_menu);

                if (ip[0] == '\0') {
                    pause_menu_set_status(g_state.pause_menu, "Please enter server IP", true);
                } else if (network_join(g_state.network, ip, port, name, g_state.world,
                                        g_state.player, g_state.entity_manager, &g_state.time_of_day)) {
                    pause_menu_set_state(g_state.pause_menu, MENU_STATE_CONNECTING);
                    printf("[NETWORK] Connecting to %s:%d\n", ip, port);
                } else {
                    pause_menu_set_status(g_state.pause_menu, "Failed to connect", true);
                }
                break;
            }

            case 8:  // Disconnect / Stop Server
                network_disconnect(g_state.network);
                pause_menu_set_state(g_state.pause_menu, MENU_STATE_MAIN);
                pause_menu_set_status(g_state.pause_menu, "Disconnected", false);
                break;

            case 9:  // Cancel connecting
                network_disconnect(g_state.network);
                pause_menu_set_state(g_state.pause_menu, MENU_STATE_JOIN_SETUP);
                break;
        }
    }

    // ESC key hierarchy: inventory > pause menu > open pause menu
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (g_state.player->inventory->is_open) {
            // Priority 1: Close inventory
            g_state.player->inventory->is_open = false;
            DisableCursor();
        } else if (pause_menu_is_open(g_state.pause_menu)) {
            // Priority 2: Resume game (close pause menu)
            pause_menu_close(g_state.pause_menu);
            DisableCursor();
        } else {
            // Priority 3: Open pause menu
            pause_menu_open(g_state.pause_menu);
            EnableCursor();
        }
    }
}

/**
 * Render game - renderer-agnostic (works on Raylib + SDL3)
 */
static void game_draw(void) {
    // Clear background
    ClearBackground(BLACK);  // Clear to black first

    // 3D rendering with player camera
    Camera3D camera = player_get_camera(g_state.player);

    // Render sky BEFORE 3D scene
    sky_render(camera, g_state.time_of_day);

    BeginMode3D(camera);

    // Disable backface culling so all block faces render
    rlDisableBackfaceCulling();

    // Draw all chunks in the world with time-based lighting
    world_render_with_time(g_state.world, g_state.time_of_day);

    // Draw all entities
    entity_manager_render(g_state.entity_manager);

    // Draw player model (only visible in third-person view)
    player_render_model(g_state.player);

    // Draw wireframe around targeted block
    if (g_state.has_target_block) {
        Vector3 block_pos = g_state.target_block_pos;
        Vector3 cube_center = {block_pos.x + 0.5f, block_pos.y + 0.5f, block_pos.z + 0.5f};
        Vector3 cube_size = {1.01f, 1.01f, 1.01f};  // Slightly larger than block

        DrawCubeWires(cube_center, cube_size.x, cube_size.y, cube_size.z, BLACK);
        DrawCubeWires(cube_center, cube_size.x * 0.99f, cube_size.y * 0.99f, cube_size.z * 0.99f, WHITE);
    }

    EndMode3D();

    // Draw nametags above remote players (2D overlay)
    network_draw_nametags(g_state.network, camera);

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

    // Draw flying mode notification
    if (g_state.flying_message_timer > 0.0f) {
        const char* message = g_state.player->is_flying ? "FLYING MODE ENABLED" : "FLYING MODE DISABLED";
        int font_size = 24;
        int text_width = MeasureText(message, font_size);
        int text_x = (screen_width - text_width) / 2;
        int text_y = 100;  // Near top of screen

        // Draw background
        DrawRectangle(text_x - 10, text_y - 5, text_width + 20, font_size + 10, (Color){0, 0, 0, 180});

        // Draw text
        Color text_color = g_state.player->is_flying ? GREEN : ORANGE;
        DrawText(message, text_x, text_y, font_size, text_color);
    }

    // Draw view mode notification
    if (g_state.view_mode_message_timer > 0.0f) {
        const char* message = (g_state.player->view_mode == VIEW_MODE_THIRD_PERSON)
            ? "THIRD PERSON VIEW" : "FIRST PERSON VIEW";
        int font_size = 24;
        int text_width = MeasureText(message, font_size);
        int text_x = (screen_width - text_width) / 2;
        int text_y = 130;  // Below flying mode message

        // Draw background
        DrawRectangle(text_x - 10, text_y - 5, text_width + 20, font_size + 10, (Color){0, 0, 0, 180});

        // Draw text
        Color text_color = (g_state.player->view_mode == VIEW_MODE_THIRD_PERSON) ? SKYBLUE : WHITE;
        DrawText(message, text_x, text_y, font_size, text_color);
    }

    // Draw hotbar (always visible)
    Texture2D atlas = texture_atlas_get_texture();
    inventory_ui_draw_hotbar(g_state.player->inventory, atlas);

    // Debug: Show time when H is held
    if (IsKeyDown(KEY_H)) {
        int hours = (int)g_state.time_of_day;
        int minutes = (int)((g_state.time_of_day - hours) * 60.0f);
        char time_str[64];
        sprintf(time_str, "Time: %02d:%02d (%.1fx speed)", hours, minutes, g_state.day_speed);
        DrawText(time_str, 10, 10, 20, WHITE);

        // Show if paused
        if (g_state.time_paused) {
            DrawText("[PAUSED]", 10, 35, 20, YELLOW);
        }
    }

    // Draw network status indicator (top-right corner)
    NetworkMode net_mode = network_get_mode(g_state.network);
    if (net_mode != NET_MODE_NONE) {
        int status_x = screen_width - 160;
        int status_y = 10;

        // Background
        DrawRectangle(status_x - 5, status_y - 3, 155, 26, (Color){0, 0, 0, 150});

        // Status dot
        Color dot_color = GREEN;
        const char* status_text = "";

        if (net_mode == NET_MODE_HOST) {
            int player_count = network_get_player_count(g_state.network);
            char host_text[64];
            sprintf(host_text, "Hosting (%d players)", player_count);
            status_text = host_text;
            DrawCircle(status_x + 8, status_y + 10, 6, dot_color);
            DrawText(status_text, status_x + 20, status_y, 16, WHITE);
        } else if (net_mode == NET_MODE_CLIENT) {
            if (g_state.network->client) {
                NetClientState client_state = net_client_get_state(g_state.network->client);
                if (client_state == NET_STATE_CONNECTED) {
                    status_text = "Connected";
                    dot_color = GREEN;
                } else if (client_state == NET_STATE_CONNECTING) {
                    status_text = "Connecting...";
                    dot_color = YELLOW;
                } else {
                    status_text = "Disconnected";
                    dot_color = RED;
                }
            }
            DrawCircle(status_x + 8, status_y + 10, 6, dot_color);
            DrawText(status_text, status_x + 20, status_y, 16, WHITE);
        }
    }

    // Draw full inventory if open
    if (g_state.player->inventory->is_open) {
        inventory_ui_draw_full_screen(g_state.player->inventory, atlas);

        // Draw tooltip for hovered item (only when not holding item)
        if (!g_state.player->inventory->is_holding_item) {
            Vector2 mouse_pos = GetMousePosition();
            inventory_ui_draw_tooltip(g_state.player->inventory, (int)mouse_pos.x, (int)mouse_pos.y);
        }

        // Draw held item following cursor
        if (g_state.player->inventory->is_holding_item) {
            Vector2 mouse_pos = GetMousePosition();
            inventory_ui_draw_item_icon(
                g_state.player->inventory->held_item.type,
                (int)mouse_pos.x - 16,
                (int)mouse_pos.y - 16,
                32,
                atlas
            );
        }
    }

    // Draw pause menu if open (rendered on top of everything)
    if (pause_menu_is_open(g_state.pause_menu)) {
        pause_menu_draw(g_state.pause_menu);
    }
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
