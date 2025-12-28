/**
 * Player Controller Implementation
 */

#include "voxel/player.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define GRAVITY 32.0f
#define JUMP_VELOCITY 10.0f
#define MAX_PITCH 89.0f  // Prevent camera flip

// ============================================================================
// PLAYER LIFECYCLE
// ============================================================================

/**
 * Create and initialize player at position
 */
Player* player_create(Vector3 start_position) {
    Player* player = (Player*)malloc(sizeof(Player));
    if (!player) return NULL;

    // Position
    player->position = start_position;

    // Camera
    player->camera.position = start_position;
    player->camera.target = Vector3Add(start_position, (Vector3){0.0f, 0.0f, -1.0f});
    player->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    player->camera.fovy = 60.0f;
    player->camera.projection = CAMERA_PERSPECTIVE;

    // Rotation
    player->yaw = 0.0f;
    player->pitch = 0.0f;

    // Movement
    player->velocity = (Vector3){0.0f, 0.0f, 0.0f};
    player->is_flying = true;  // Start in flying mode
    player->is_grounded = false;

    // Settings
    player->mouse_sensitivity = 0.1f;
    player->move_speed = 10.0f;
    player->fly_speed = 20.0f;
    player->sprint_multiplier = 2.0f;

    printf("[PLAYER] Created at (%.1f, %.1f, %.1f)\n",
           start_position.x, start_position.y, start_position.z);

    return player;
}

/**
 * Destroy player
 */
void player_destroy(Player* player) {
    if (player) {
        free(player);
    }
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

/**
 * Update camera rotation from mouse input
 */
static void update_camera_rotation(Player* player, float dt) {
    (void)dt; // Unused

    // Get mouse delta
    Vector2 mouse_delta = GetMouseDelta();

    // Update yaw and pitch
    player->yaw += mouse_delta.x * player->mouse_sensitivity;
    player->pitch -= mouse_delta.y * player->mouse_sensitivity;

    // Clamp pitch to prevent camera flip
    if (player->pitch > MAX_PITCH) player->pitch = MAX_PITCH;
    if (player->pitch < -MAX_PITCH) player->pitch = -MAX_PITCH;

    // Wrap yaw
    if (player->yaw > 360.0f) player->yaw -= 360.0f;
    if (player->yaw < 0.0f) player->yaw += 360.0f;
}

/**
 * Get movement direction vectors from camera rotation
 */
static void get_movement_vectors(Player* player, Vector3* forward, Vector3* right) {
    // Forward vector (ignore pitch for horizontal movement)
    float yaw_rad = player->yaw * DEG2RAD;
    forward->x = sinf(yaw_rad);
    forward->y = 0.0f;
    forward->z = -cosf(yaw_rad);
    *forward = Vector3Normalize(*forward);

    // Right vector (perpendicular to forward)
    *right = Vector3CrossProduct((Vector3){0.0f, 1.0f, 0.0f}, *forward);
    *right = Vector3Normalize(*right);
}

/**
 * Handle movement input
 */
static void update_movement(Player* player, float dt) {
    Vector3 forward, right;
    get_movement_vectors(player, &forward, &right);

    Vector3 move_direction = {0.0f, 0.0f, 0.0f};

    // WASD movement
    if (IsKeyDown(KEY_W)) move_direction = Vector3Add(move_direction, forward);
    if (IsKeyDown(KEY_S)) move_direction = Vector3Subtract(move_direction, forward);
    if (IsKeyDown(KEY_A)) move_direction = Vector3Add(move_direction, right);
    if (IsKeyDown(KEY_D)) move_direction = Vector3Subtract(move_direction, right);

    // Normalize diagonal movement
    if (Vector3Length(move_direction) > 0.0f) {
        move_direction = Vector3Normalize(move_direction);
    }

    // Calculate speed
    float speed = player->is_flying ? player->fly_speed : player->move_speed;

    // Sprint modifier
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        speed *= player->sprint_multiplier;
    }

    // Apply movement
    if (player->is_flying) {
        // Flying mode: direct movement in all directions
        Vector3 velocity = Vector3Scale(move_direction, speed);

        // Vertical movement
        if (IsKeyDown(KEY_SPACE)) velocity.y = speed;
        if (IsKeyDown(KEY_LEFT_CONTROL)) velocity.y = -speed;

        player->velocity = velocity;
    } else {
        // Walking mode: apply gravity
        player->velocity.x = move_direction.x * speed;
        player->velocity.z = move_direction.z * speed;

        // Jump
        if (IsKeyDown(KEY_SPACE) && player->is_grounded) {
            player->velocity.y = JUMP_VELOCITY;
            player->is_grounded = false;
        }

        // Apply gravity
        player->velocity.y -= GRAVITY * dt;

        // Simple ground check (TODO: replace with proper collision)
        if (player->position.y <= 40.0f) {  // Assume ground at y=40 for now
            player->position.y = 40.0f;
            player->velocity.y = 0.0f;
            player->is_grounded = true;
        }
    }

    // Update position
    player->position = Vector3Add(player->position, Vector3Scale(player->velocity, dt));
}

/**
 * Update camera position and target
 */
static void update_camera(Player* player) {
    // Camera position is player position (first-person)
    player->camera.position = player->position;

    // Calculate camera target from yaw and pitch
    float yaw_rad = player->yaw * DEG2RAD;
    float pitch_rad = player->pitch * DEG2RAD;

    Vector3 direction;
    direction.x = cosf(pitch_rad) * sinf(yaw_rad);
    direction.y = sinf(pitch_rad);
    direction.z = -cosf(pitch_rad) * cosf(yaw_rad);

    player->camera.target = Vector3Add(player->camera.position, direction);
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Update player - call every frame with delta time
 */
void player_update(Player* player, float dt) {
    if (!player) return;

    // Handle flying mode toggle
    if (IsKeyPressed(KEY_F)) {
        player_toggle_flying(player);
    }

    // Update camera rotation from mouse
    update_camera_rotation(player, dt);

    // Update movement from keyboard
    update_movement(player, dt);

    // Update camera
    update_camera(player);
}

/**
 * Set player position
 */
void player_set_position(Player* player, Vector3 position) {
    if (!player) return;
    player->position = position;
}

/**
 * Toggle flying mode
 */
void player_toggle_flying(Player* player) {
    if (!player) return;
    player->is_flying = !player->is_flying;

    if (player->is_flying) {
        printf("[PLAYER] Flying mode enabled\n");
        player->velocity.y = 0.0f;  // Cancel gravity
    } else {
        printf("[PLAYER] Walking mode enabled\n");
    }
}

/**
 * Get player's camera for rendering
 */
Camera3D player_get_camera(Player* player) {
    return player ? player->camera : (Camera3D){0};
}
