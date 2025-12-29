/**
 * Player Controller Implementation
 */

#include "voxel/player.h"
#include "voxel/world.h"
#include "voxel/block.h"
#include "voxel/inventory.h"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define GRAVITY 32.0f
#define JUMP_VELOCITY 10.0f
#define MAX_PITCH 89.0f  // Prevent camera flip

// Player collision box (AABB)
#define PLAYER_WIDTH 0.8f    // Player width (X and Z)
#define PLAYER_HEIGHT 1.8f   // Player height (Y)
#define PLAYER_EYE_HEIGHT 1.6f  // Camera offset from feet

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

    // View mode
    player->view_mode = VIEW_MODE_FIRST_PERSON;  // Start in first-person
    player->third_person_distance = 5.0f;  // Default distance for third-person view

    // Movement
    player->velocity = (Vector3){0.0f, 0.0f, 0.0f};
    player->is_flying = false;  // Start in walking mode with gravity
    player->is_grounded = false;

    // Settings
    player->mouse_sensitivity = 0.1f;
    player->move_speed = 10.0f;
    player->fly_speed = 20.0f;
    player->sprint_multiplier = 2.0f;

    // Create inventory
    player->inventory = inventory_create();

    // Animation state (start at rest)
    player->walk_animation_time = 0.0f;
    player->arm_swing_angle = 0.0f;
    player->leg_swing_angle = 0.0f;

    // Lighting (default to full brightness)
    player->ambient_light = (Vector3){1.0f, 1.0f, 1.0f};

    printf("[PLAYER] Created at (%.1f, %.1f, %.1f)\n",
           start_position.x, start_position.y, start_position.z);

    return player;
}

/**
 * Destroy player
 */
void player_destroy(Player* player) {
    if (player) {
        inventory_destroy(player->inventory);
        free(player);
    }
}

// ============================================================================
// COLLISION DETECTION
// ============================================================================

/**
 * Check if a point is inside a solid block
 */
static bool is_solid_at(World * world, float x, float y, float z) {
    if (!world) return false;

    int block_x = (int)floorf(x);
    int block_y = (int)floorf(y);
    int block_z = (int)floorf(z);

    Block block = world_get_block(world, block_x, block_y, block_z);
    return block_is_solid(block);
}

/**
 * Check if the chunk containing a position is loaded
 */
static bool is_chunk_loaded_at(World* world, float x, float z) {
    if (!world) return false;

    int chunk_x = (int)floorf(x / CHUNK_SIZE);
    int chunk_z = (int)floorf(z / CHUNK_SIZE);

    Chunk* chunk = world_get_chunk(world, chunk_x, chunk_z);
    return chunk != NULL;
}

/**
 * Check if player's bounding box collides with world at given position
 */
static bool check_collision(World * world, Vector3 position) {
    if (!world) return false;

    // Player bounding box corners
    float half_width = PLAYER_WIDTH / 2.0f;

    // Check multiple points on the player's bounding box
    // Bottom corners
    if (is_solid_at(world, position.x - half_width, position.y, position.z - half_width)) return true;
    if (is_solid_at(world, position.x + half_width, position.y, position.z - half_width)) return true;
    if (is_solid_at(world, position.x - half_width, position.y, position.z + half_width)) return true;
    if (is_solid_at(world, position.x + half_width, position.y, position.z + half_width)) return true;

    // Middle corners
    if (is_solid_at(world, position.x - half_width, position.y + PLAYER_HEIGHT / 2.0f, position.z - half_width)) return true;
    if (is_solid_at(world, position.x + half_width, position.y + PLAYER_HEIGHT / 2.0f, position.z - half_width)) return true;
    if (is_solid_at(world, position.x - half_width, position.y + PLAYER_HEIGHT / 2.0f, position.z + half_width)) return true;
    if (is_solid_at(world, position.x + half_width, position.y + PLAYER_HEIGHT / 2.0f, position.z + half_width)) return true;

    // Top corners
    if (is_solid_at(world, position.x - half_width, position.y + PLAYER_HEIGHT, position.z - half_width)) return true;
    if (is_solid_at(world, position.x + half_width, position.y + PLAYER_HEIGHT, position.z - half_width)) return true;
    if (is_solid_at(world, position.x - half_width, position.y + PLAYER_HEIGHT, position.z + half_width)) return true;
    if (is_solid_at(world, position.x + half_width, position.y + PLAYER_HEIGHT, position.z + half_width)) return true;

    return false;
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

/**
 * Update camera rotation from mouse input
 */
static void update_camera_rotation(Player* player, float dt) {
    (void)dt; // Unused

    // Skip mouse input if window is not focused
    if (!IsWindowFocused()) return;

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
 * Handle movement input with collision detection
 */
static void update_movement(Player* player, World * world, float dt) {
    Vector3 forward, right;
    get_movement_vectors(player, &forward, &right);

    // Invert controls in front-facing view (camera is in front, looking at player)
    if (player->view_mode == VIEW_MODE_THIRD_PERSON_FRONT) {
        forward = Vector3Negate(forward);
        right = Vector3Negate(right);
    }

    Vector3 move_direction = {0.0f, 0.0f, 0.0f};

    // Skip keyboard input if window is not focused
    bool window_focused = IsWindowFocused();

    // WASD movement (only when window focused)
    if (window_focused && IsKeyDown(KEY_W)) move_direction = Vector3Add(move_direction, forward);
    if (window_focused && IsKeyDown(KEY_S)) move_direction = Vector3Subtract(move_direction, forward);
    if (window_focused && IsKeyDown(KEY_A)) move_direction = Vector3Add(move_direction, right);
    if (window_focused && IsKeyDown(KEY_D)) move_direction = Vector3Subtract(move_direction, right);

    // Normalize diagonal movement
    if (Vector3Length(move_direction) > 0.0f) {
        move_direction = Vector3Normalize(move_direction);
    }

    // Calculate speed
    float speed = player->is_flying ? player->fly_speed : player->move_speed;

    // Sprint modifier (only when window focused)
    if (window_focused && IsKeyDown(KEY_LEFT_SHIFT)) {
        speed *= player->sprint_multiplier;
    }

    // Apply movement
    if (player->is_flying) {
        // Flying mode: direct movement in all directions
        Vector3 velocity = Vector3Scale(move_direction, speed);

        // Vertical movement (only when window focused)
        if (window_focused && IsKeyDown(KEY_SPACE)) velocity.y = speed;
        if (window_focused && IsKeyDown(KEY_LEFT_CONTROL)) velocity.y = -speed;

        player->velocity = velocity;
    } else {
        // Walking mode: apply gravity
        player->velocity.x = move_direction.x * speed;
        player->velocity.z = move_direction.z * speed;

        // Jump (only when window focused)
        if (window_focused && IsKeyDown(KEY_SPACE) && player->is_grounded) {
            player->velocity.y = JUMP_VELOCITY;
            player->is_grounded = false;
        }

        // Apply gravity
        player->velocity.y -= GRAVITY * dt;
    }

    // Apply movement with collision detection (per-axis)
    Vector3 new_position = player->position;

    // Try to move on X axis
    new_position.x = player->position.x + player->velocity.x * dt;
    if (!is_chunk_loaded_at(world, new_position.x, new_position.z) ||
        check_collision(world, new_position)) {
        new_position.x = player->position.x;  // Cancel X movement
        player->velocity.x = 0.0f;
    }

    // Try to move on Y axis
    new_position.y = player->position.y + player->velocity.y * dt;
    if (check_collision(world, new_position)) {
        // Check if we're hitting ground (moving downward)
        if (player->velocity.y < 0.0f) {
            player->is_grounded = true;
        }

        new_position.y = player->position.y;  // Cancel Y movement
        player->velocity.y = 0.0f;
    } else {
        player->is_grounded = false;
    }

    // Additional ground check - look slightly below player
    Vector3 ground_check = new_position;
    ground_check.y -= 0.1f;  // Check 0.1 blocks below
    if (check_collision(world, ground_check)) {
        player->is_grounded = true;
    }

    // Try to move on Z axis
    new_position.z = player->position.z + player->velocity.z * dt;
    if (!is_chunk_loaded_at(world, new_position.x, new_position.z) ||
        check_collision(world, new_position)) {
        new_position.z = player->position.z;  // Cancel Z movement
        player->velocity.z = 0.0f;
    }

    // Update position
    player->position = new_position;
}

/**
 * Update camera position and target
 */
static void update_camera(Player* player) {
    // Calculate camera direction from yaw and pitch
    float yaw_rad = player->yaw * DEG2RAD;
    float pitch_rad = player->pitch * DEG2RAD;

    Vector3 direction;
    direction.x = cosf(pitch_rad) * sinf(yaw_rad);
    direction.y = sinf(pitch_rad);
    direction.z = -cosf(pitch_rad) * cosf(yaw_rad);

    if (player->view_mode == VIEW_MODE_FIRST_PERSON) {
        // First-person: Camera is at player eye position
        player->camera.position = player->position;
        player->camera.position.y += PLAYER_EYE_HEIGHT;
        player->camera.target = Vector3Add(player->camera.position, direction);
    } else if (player->view_mode == VIEW_MODE_THIRD_PERSON) {
        // Third-person: Camera is behind the player
        Vector3 player_eye_pos = player->position;
        player_eye_pos.y += PLAYER_EYE_HEIGHT;

        // Position camera behind the player
        Vector3 camera_offset = Vector3Scale(direction, -player->third_person_distance);
        player->camera.position = Vector3Add(player_eye_pos, camera_offset);

        // Camera looks at the player
        player->camera.target = player_eye_pos;
    } else {
        // Front-facing: Camera is in front of the player
        Vector3 player_eye_pos = player->position;
        player_eye_pos.y += PLAYER_EYE_HEIGHT;

        // Position camera IN FRONT of the player (positive direction)
        Vector3 camera_offset = Vector3Scale(direction, player->third_person_distance);
        player->camera.position = Vector3Add(player_eye_pos, camera_offset);

        // Camera looks at the player
        player->camera.target = player_eye_pos;
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Update player - call every frame with delta time
 */
void player_update(Player* player, World * world, float dt) {
    if (!player) return;

    // Skip all input if window is not focused
    if (!IsWindowFocused()) {
        // Still update camera position to follow player
        update_camera(player);
        return;
    }

    // Handle flying mode toggle
    if (IsKeyPressed(KEY_F)) {
        player_toggle_flying(player);
    }

    // Handle view mode toggle with V key
    if (IsKeyPressed(KEY_V)) {
        player_toggle_view_mode(player);
    }

    // Update camera rotation from mouse
    update_camera_rotation(player, dt);

    // Update movement from keyboard (with collision detection)
    update_movement(player, world, dt);

    // Update camera
    update_camera(player);

    // Update walk animation based on horizontal velocity
    float horiz_speed = sqrtf(player->velocity.x * player->velocity.x +
                              player->velocity.z * player->velocity.z);
    if (horiz_speed > 0.5f) {
        // Moving: animate walk cycle
        player->walk_animation_time += dt * horiz_speed * 0.8f;
        player->arm_swing_angle = sinf(player->walk_animation_time) * 30.0f;
        player->leg_swing_angle = sinf(player->walk_animation_time) * 25.0f;
    } else {
        // Idle: smoothly return to rest pose
        player->arm_swing_angle *= 0.85f;
        player->leg_swing_angle *= 0.85f;
    }

    // Update ambient lighting from world time
    if (world) {
        player->ambient_light = world_get_ambient_color(world->time_of_day);
    }
}

/**
 * Update player physics only - no input handling
 * Called when game menu is open but world should continue
 */
void player_update_physics(Player* player, World* world, float dt) {
    if (!player) return;

    // Only apply physics if not flying (flying suspends gravity)
    if (!player->is_flying) {
        // Apply gravity
        player->velocity.y -= GRAVITY * dt;

        // Zero out horizontal velocity (no input)
        player->velocity.x = 0.0f;
        player->velocity.z = 0.0f;

        // Apply movement with collision detection
        Vector3 new_position = player->position;

        // Try to move on Y axis (gravity)
        new_position.y = player->position.y + player->velocity.y * dt;
        if (check_collision(world, new_position)) {
            // Hit ground (moving downward)
            if (player->velocity.y < 0.0f) {
                player->is_grounded = true;
            }
            new_position.y = player->position.y;  // Cancel Y movement
            player->velocity.y = 0.0f;
        } else {
            player->is_grounded = false;
        }

        // Additional ground check
        Vector3 ground_check = new_position;
        ground_check.y -= 0.1f;
        if (check_collision(world, ground_check)) {
            player->is_grounded = true;
        }

        // Update position
        player->position = new_position;
    }

    // Update camera to follow player
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
 * Toggle view mode: First-Person -> Third-Person -> Front-Facing -> First-Person
 */
void player_toggle_view_mode(Player* player) {
    if (!player) return;

    switch (player->view_mode) {
        case VIEW_MODE_FIRST_PERSON:
            player->view_mode = VIEW_MODE_THIRD_PERSON;
            printf("[PLAYER] Switched to third-person view\n");
            break;
        case VIEW_MODE_THIRD_PERSON:
            player->view_mode = VIEW_MODE_THIRD_PERSON_FRONT;
            printf("[PLAYER] Switched to front-facing view\n");
            break;
        case VIEW_MODE_THIRD_PERSON_FRONT:
            player->view_mode = VIEW_MODE_FIRST_PERSON;
            printf("[PLAYER] Switched to first-person view\n");
            break;
    }
}

/**
 * Get player's camera for rendering
 */
Camera3D player_get_camera(Player* player) {
    return player ? player->camera : (Camera3D){0};
}

/**
 * Check if a block position would collide with the player
 */
bool player_collides_with_position(Player* player, Vector3 block_pos) {
    if (!player) return false;

    // Block bounding box
    Vector3 block_min = block_pos;
    Vector3 block_max = {block_pos.x + 1.0f, block_pos.y + 1.0f, block_pos.z + 1.0f};

    // Player bounding box
    float half_width = PLAYER_WIDTH / 2.0f;
    Vector3 player_min = {
        player->position.x - half_width,
        player->position.y,
        player->position.z - half_width
    };
    Vector3 player_max = {
        player->position.x + half_width,
        player->position.y + PLAYER_HEIGHT,
        player->position.z + half_width
    };

    // AABB collision check
    return (block_min.x < player_max.x && block_max.x > player_min.x) &&
           (block_min.y < player_max.y && block_max.y > player_min.y) &&
           (block_min.z < player_max.z && block_max.z > player_min.z);
}

/**
 * Apply ambient lighting to a color
 */
static Color apply_player_ambient(Color c, Vector3 ambient) {
    return (Color){
        (unsigned char)(c.r * ambient.x),
        (unsigned char)(c.g * ambient.y),
        (unsigned char)(c.b * ambient.z),
        c.a
    };
}

/**
 * Render the player model (for third-person view)
 * Draws a simple block-based humanoid character with walk animation and eyes
 */
void player_render_model(Player* player) {
    // Render player model in both third-person views (behind and front)
    if (!player || player->view_mode == VIEW_MODE_FIRST_PERSON) return;

    Vector3 pos = player->position;
    float yaw_rad = player->yaw * DEG2RAD;

    // Player colors with ambient lighting applied
    Color head_color = apply_player_ambient((Color){255, 200, 150, 255}, player->ambient_light);   // Skin tone
    Color torso_color = apply_player_ambient((Color){100, 100, 200, 255}, player->ambient_light);  // Blue shirt
    Color leg_color = apply_player_ambient((Color){50, 50, 80, 255}, player->ambient_light);       // Dark pants
    Color arm_color = torso_color;

    // Body proportions (similar to block_human)
    float leg_length = 0.6f;
    float leg_width = 0.15f;
    float torso_height = 0.75f;
    float torso_width = 0.5f;
    float torso_depth = 0.25f;
    float arm_length = 0.6f;
    float arm_width = 0.15f;
    float head_size = 0.4f;

    // Calculate sin/cos for rotation
    float cos_yaw = cosf(yaw_rad);
    float sin_yaw = sinf(yaw_rad);

    // Build character from bottom to top
    float y = pos.y;

    // ========================================================================
    // LEGS (2 blocks, animated with walking)
    // ========================================================================
    float leg_spacing = 0.125f;

    // Left leg pivot point (at hip)
    float left_hip_x = pos.x + leg_spacing * cos_yaw;
    float left_hip_z = pos.z - leg_spacing * sin_yaw;
    float hip_y = y + leg_length;

    // Right leg pivot point
    float right_hip_x = pos.x - leg_spacing * cos_yaw;
    float right_hip_z = pos.z + leg_spacing * sin_yaw;

    // Left leg (swings forward when right arm swings forward)
    rlPushMatrix();
    rlTranslatef(left_hip_x, hip_y, left_hip_z);
    rlRotatef(player->yaw, 0, 1, 0);
    rlRotatef(player->leg_swing_angle, 1, 0, 0);  // Swing around X axis
    DrawCube((Vector3){0, -leg_length / 2.0f, 0}, leg_width, leg_length, leg_width, leg_color);
    rlPopMatrix();

    // Right leg (swings opposite to left)
    rlPushMatrix();
    rlTranslatef(right_hip_x, hip_y, right_hip_z);
    rlRotatef(player->yaw, 0, 1, 0);
    rlRotatef(-player->leg_swing_angle, 1, 0, 0);  // Opposite swing
    DrawCube((Vector3){0, -leg_length / 2.0f, 0}, leg_width, leg_length, leg_width, leg_color);
    rlPopMatrix();

    y += leg_length;

    // ========================================================================
    // TORSO
    // ========================================================================
    Vector3 torso_center = {pos.x, y + torso_height / 2.0f, pos.z};

    // Draw rotated torso using rlgl transforms
    rlPushMatrix();
    rlTranslatef(torso_center.x, torso_center.y, torso_center.z);
    rlRotatef(player->yaw, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0}, torso_width, torso_height, torso_depth, torso_color);
    rlPopMatrix();

    // ========================================================================
    // ARMS (animated with walking)
    // ========================================================================
    float arm_attach_height = y + torso_height * 0.85f;  // Shoulder level
    float arm_offset = (torso_width / 2.0f) + (arm_width / 2.0f);

    // Left shoulder position
    float left_shoulder_x = pos.x + arm_offset * cos_yaw;
    float left_shoulder_z = pos.z - arm_offset * sin_yaw;

    // Right shoulder position
    float right_shoulder_x = pos.x - arm_offset * cos_yaw;
    float right_shoulder_z = pos.z + arm_offset * sin_yaw;

    // Left arm (swings opposite to left leg, same as right leg)
    rlPushMatrix();
    rlTranslatef(left_shoulder_x, arm_attach_height, left_shoulder_z);
    rlRotatef(player->yaw, 0, 1, 0);
    rlRotatef(-player->arm_swing_angle, 1, 0, 0);  // Opposite to left leg
    DrawCube((Vector3){0, -arm_length / 2.0f, 0}, arm_width, arm_length, arm_width, arm_color);
    rlPopMatrix();

    // Right arm (swings same as left leg)
    rlPushMatrix();
    rlTranslatef(right_shoulder_x, arm_attach_height, right_shoulder_z);
    rlRotatef(player->yaw, 0, 1, 0);
    rlRotatef(player->arm_swing_angle, 1, 0, 0);  // Same as left leg
    DrawCube((Vector3){0, -arm_length / 2.0f, 0}, arm_width, arm_length, arm_width, arm_color);
    rlPopMatrix();

    y += torso_height;

    // ========================================================================
    // HEAD (sphere with eyes)
    // ========================================================================
    Vector3 head_center = {pos.x, y + head_size, pos.z};
    DrawSphere(head_center, head_size, head_color);

    // Eye colors with ambient lighting
    Color eye_white = apply_player_ambient(WHITE, player->ambient_light);
    Color eye_black = apply_player_ambient((Color){30, 30, 30, 255}, player->ambient_light);

    // Eye parameters
    float eye_offset_y = 0.05f;       // Slightly above center
    float eye_offset_side = 0.12f;    // Distance apart (half)
    float eye_offset_forward = head_size * 0.9f;  // On head surface

    // Forward direction based on yaw
    Vector3 forward = {sin_yaw, 0, -cos_yaw};
    Vector3 right = {cos_yaw, 0, sin_yaw};

    // Left eye (white part)
    Vector3 left_eye_pos = head_center;
    left_eye_pos.x += forward.x * eye_offset_forward - right.x * eye_offset_side;
    left_eye_pos.y += eye_offset_y;
    left_eye_pos.z += forward.z * eye_offset_forward - right.z * eye_offset_side;
    DrawSphere(left_eye_pos, 0.07f, eye_white);

    // Left pupil (black, slightly forward)
    Vector3 left_pupil_pos = left_eye_pos;
    left_pupil_pos.x += forward.x * 0.04f;
    left_pupil_pos.z += forward.z * 0.04f;
    DrawSphere(left_pupil_pos, 0.035f, eye_black);

    // Right eye (white part)
    Vector3 right_eye_pos = head_center;
    right_eye_pos.x += forward.x * eye_offset_forward + right.x * eye_offset_side;
    right_eye_pos.y += eye_offset_y;
    right_eye_pos.z += forward.z * eye_offset_forward + right.z * eye_offset_side;
    DrawSphere(right_eye_pos, 0.07f, eye_white);

    // Right pupil (black, slightly forward)
    Vector3 right_pupil_pos = right_eye_pos;
    right_pupil_pos.x += forward.x * 0.04f;
    right_pupil_pos.z += forward.z * 0.04f;
    DrawSphere(right_pupil_pos, 0.035f, eye_black);
}
