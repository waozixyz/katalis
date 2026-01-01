/**
 * Sheep Entity Implementation
 *
 * Passive mob with wandering and flee AI behavior
 */

#include "voxel/entity/sheep.h"
#include "voxel/entity/collision.h"
#include "voxel/entity/entity_utils.h"
#include "voxel/world/world.h"
#include "voxel/player/player.h"
#include "voxel/core/block.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

/**
 * Create sheep data with given colors
 */
static SheepData* sheep_create_data(Color wool_color, Color skin_color) {
    SheepData* data = (SheepData*)malloc(sizeof(SheepData));
    if (!data) {
        printf("[SHEEP] Failed to allocate data\n");
        return NULL;
    }

    data->wool_color = wool_color;
    data->skin_color = skin_color;

    // Animation state
    data->walk_animation_time = 0.0f;
    data->leg_swing_angle = 0.0f;
    data->idle_time = 0.0f;
    data->head_yaw_target = 0.0f;
    data->head_yaw_current = 0.0f;
    data->head_look_timer = entity_random_range(2.0f, 4.0f);
    data->blink_timer = entity_random_range(3.0f, 6.0f);
    data->blink_progress = 0.0f;

    // AI state - start wandering
    data->wander_timer = entity_random_range(SHEEP_WANDER_TIME_MIN, SHEEP_WANDER_TIME_MAX);
    data->graze_timer = 0.0f;
    data->is_grazing = false;
    data->is_fleeing = false;
    data->wander_direction = entity_random_direction();

    // Default lighting (full brightness)
    data->ambient_light = (Vector3){1.0f, 1.0f, 1.0f};

    // Health & damage
    data->hp = 4;                   // 4 hits to kill
    data->damage_flash_timer = 0.0f;

    // Jump
    data->jump_cooldown = 0.0f;

    return data;
}

// ============================================================================
// ENTITY CALLBACKS
// ============================================================================

/**
 * Update function for sheep
 * Handles AI behavior (wandering, grazing, fleeing) and animation
 */
void sheep_update(Entity* entity, struct World* world, float dt) {
    if (!entity || !entity->data) return;

    SheepData* data = (SheepData*)entity->data;

    // Get player position for flee behavior (if world has player reference)
    Vector3 player_pos = {0, 0, 0};
    bool has_player = false;
    if (world && world->player) {
        player_pos = world->player->position;
        has_player = true;
    }

    // Calculate distance to player
    float player_dist = FLT_MAX;
    Vector3 flee_direction = {0, 0, 0};
    if (has_player) {
        Vector3 to_player = Vector3Subtract(player_pos, entity->position);
        to_player.y = 0;  // Only horizontal distance
        player_dist = Vector3Length(to_player);
        if (player_dist > 0.01f) {
            flee_direction = Vector3Scale(to_player, -1.0f / player_dist);  // Away from player
        }
    }

    // ========================================================================
    // AI STATE MACHINE
    // ========================================================================

    // Check if should start fleeing
    if (has_player && player_dist < SHEEP_FLEE_DISTANCE) {
        data->is_fleeing = true;
        data->is_grazing = false;
    }
    // Check if can stop fleeing
    else if (data->is_fleeing && player_dist > SHEEP_SAFE_DISTANCE) {
        data->is_fleeing = false;
    }

    // Update based on state
    if (data->is_fleeing) {
        // Fleeing: run away from player
        entity->velocity.x = flee_direction.x * SHEEP_FLEE_SPEED;
        entity->velocity.z = flee_direction.z * SHEEP_FLEE_SPEED;

        // Face away from player
        entity->rotation.y = atan2f(flee_direction.x, flee_direction.z) * RAD2DEG;
    }
    else if (data->is_grazing) {
        // Grazing: stand still
        entity->velocity.x = 0;
        entity->velocity.z = 0;

        data->graze_timer -= dt;
        if (data->graze_timer <= 0) {
            data->is_grazing = false;
            data->wander_timer = entity_random_range(SHEEP_WANDER_TIME_MIN, SHEEP_WANDER_TIME_MAX);
            data->wander_direction = entity_random_direction();
        }
    }
    else {
        // Wandering
        data->wander_timer -= dt;

        if (data->wander_timer <= 0) {
            // Time to change behavior
            if (entity_random_range(0, 1) < SHEEP_GRAZE_CHANCE) {
                // Start grazing
                data->is_grazing = true;
                data->graze_timer = SHEEP_GRAZE_TIME;
            } else {
                // Pick new wander direction
                data->wander_direction = entity_random_direction();
                data->wander_timer = entity_random_range(SHEEP_WANDER_TIME_MIN, SHEEP_WANDER_TIME_MAX);
            }
        }

        // Move in wander direction
        entity->velocity.x = data->wander_direction.x * SHEEP_WANDER_SPEED;
        entity->velocity.z = data->wander_direction.z * SHEEP_WANDER_SPEED;

        // Face movement direction
        if (Vector3Length(data->wander_direction) > 0.01f) {
            entity->rotation.y = atan2f(data->wander_direction.x, data->wander_direction.z) * RAD2DEG;
        }
    }

    // ========================================================================
    // PHYSICS (using AABB collision system)
    // ========================================================================

    // Update jump cooldown
    if (data->jump_cooldown > 0) {
        data->jump_cooldown -= dt;
    }

    // Check for jumpable obstacles ahead (using current movement direction)
    Vector3 move_dir = data->is_fleeing ? flee_direction : data->wander_direction;
    bool should_jump = false;
    if (data->jump_cooldown <= 0 && (move_dir.x != 0 || move_dir.z != 0)) {
        should_jump = entity_can_jump_obstacle(entity, world, move_dir);
    }

    // Apply jump if obstacle is jumpable
    if (should_jump) {
        entity->velocity.y = SHEEP_JUMP_VELOCITY;
        data->jump_cooldown = SHEEP_JUMP_COOLDOWN;
    }

    // Apply gravity
    entity_apply_gravity(entity, world, dt, 20.0f);

    // Move with per-axis collision detection
    int collision_flags = entity_move_with_collision(entity, world, dt);

    // Pick new direction when hitting a wall (only if can't jump and not fleeing)
    if (COLLISION_HIT_WALL(collision_flags) && !data->is_fleeing && !should_jump) {
        data->wander_direction = entity_random_direction();
    }

    // ========================================================================
    // ANIMATION
    // ========================================================================

    float horiz_speed = sqrtf(entity->velocity.x * entity->velocity.x +
                              entity->velocity.z * entity->velocity.z);
    if (horiz_speed > 0.1f) {
        // Moving: animate walk cycle
        float anim_speed = data->is_fleeing ? 1.5f : 0.8f;  // Faster animation when fleeing
        data->walk_animation_time += dt * horiz_speed * anim_speed;
        data->leg_swing_angle = sinf(data->walk_animation_time) * 25.0f;
        data->idle_time = 0.0f;  // Reset idle time when moving
    } else {
        // Idle: smoothly return to rest pose
        data->leg_swing_angle *= ENTITY_ANIMATION_DAMPING;
        data->idle_time += dt;  // Accumulate idle time for breathing
    }

    // Head look-around animation (when idle and not fleeing or grazing)
    if (!data->is_fleeing && !data->is_grazing && horiz_speed < 0.1f) {
        data->head_look_timer -= dt;
        if (data->head_look_timer <= 0) {
            data->head_yaw_target = entity_random_range(-30.0f, 30.0f);
            data->head_look_timer = entity_random_range(2.0f, 4.0f);
        }
    } else {
        // Reset head to forward when moving, fleeing, or grazing
        data->head_yaw_target = 0.0f;
    }
    // Smooth interpolation to target
    data->head_yaw_current += (data->head_yaw_target - data->head_yaw_current) * dt * 3.0f;

    // Blink animation
    data->blink_timer -= dt;
    if (data->blink_timer <= 0) {
        data->blink_progress = 1.0f;  // Start blink
        data->blink_timer = entity_random_range(3.0f, 6.0f);
    }
    if (data->blink_progress > 0) {
        data->blink_progress -= dt * 8.0f;  // Fast blink (~0.125s)
        if (data->blink_progress < 0) data->blink_progress = 0;
    }

    // ========================================================================
    // LIGHTING
    // ========================================================================

    // Get ambient light from world's time of day
    if (world) {
        data->ambient_light = world_get_ambient_color(world->time_of_day);
    }

    // ========================================================================
    // DAMAGE FLASH
    // ========================================================================

    if (data->damage_flash_timer > 0) {
        data->damage_flash_timer -= dt;
    }
}

/**
 * Render function for sheep
 * Draws a blocky Minecraft-style sheep
 */
void sheep_render(Entity* entity) {
    if (!entity || !entity->data) return;

    SheepData* data = (SheepData*)entity->data;
    Vector3 pos = entity->position;

    // Apply ambient lighting to colors
    Color wool_lit = entity_apply_ambient(data->wool_color, data->ambient_light);
    Color skin_lit = entity_apply_ambient(data->skin_color, data->ambient_light);

    // Apply red flash when damaged
    if (data->damage_flash_timer > 0) {
        wool_lit = (Color){255, 100, 100, 255};
        skin_lit = (Color){255, 100, 100, 255};
    }

    float yaw = entity->rotation.y;
    float yaw_rad = yaw * DEG2RAD;
    float cos_yaw = cosf(yaw_rad);
    float sin_yaw = sinf(yaw_rad);

    // Forward and right vectors
    Vector3 forward = {sin_yaw, 0, cos_yaw};
    Vector3 right = {cos_yaw, 0, -sin_yaw};

    float y = pos.y;

    // ========================================================================
    // LEGS (4 legs with walk animation)
    // ========================================================================
    float leg_offset_side = SHEEP_BODY_WIDTH / 2.0f - SHEEP_LEG_WIDTH / 2.0f;
    float leg_offset_front = SHEEP_BODY_LENGTH / 2.0f - SHEEP_LEG_WIDTH;
    float hip_y = y + SHEEP_LEG_LENGTH;

    // Front-left leg
    Vector3 fl_hip = pos;
    fl_hip.x += forward.x * leg_offset_front - right.x * leg_offset_side;
    fl_hip.z += forward.z * leg_offset_front - right.z * leg_offset_side;
    fl_hip.y = hip_y;

    rlPushMatrix();
    rlTranslatef(fl_hip.x, fl_hip.y, fl_hip.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(data->leg_swing_angle, 1, 0, 0);
    DrawCube((Vector3){0, -SHEEP_LEG_LENGTH / 2.0f, 0},
             SHEEP_LEG_WIDTH, SHEEP_LEG_LENGTH, SHEEP_LEG_WIDTH,
             skin_lit);
    rlPopMatrix();

    // Front-right leg (opposite swing)
    Vector3 fr_hip = pos;
    fr_hip.x += forward.x * leg_offset_front + right.x * leg_offset_side;
    fr_hip.z += forward.z * leg_offset_front + right.z * leg_offset_side;
    fr_hip.y = hip_y;

    rlPushMatrix();
    rlTranslatef(fr_hip.x, fr_hip.y, fr_hip.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(-data->leg_swing_angle, 1, 0, 0);
    DrawCube((Vector3){0, -SHEEP_LEG_LENGTH / 2.0f, 0},
             SHEEP_LEG_WIDTH, SHEEP_LEG_LENGTH, SHEEP_LEG_WIDTH,
             skin_lit);
    rlPopMatrix();

    // Back-left leg (opposite to front-left)
    Vector3 bl_hip = pos;
    bl_hip.x -= forward.x * leg_offset_front - right.x * leg_offset_side;
    bl_hip.z -= forward.z * leg_offset_front - right.z * leg_offset_side;
    bl_hip.y = hip_y;

    rlPushMatrix();
    rlTranslatef(bl_hip.x, bl_hip.y, bl_hip.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(-data->leg_swing_angle, 1, 0, 0);
    DrawCube((Vector3){0, -SHEEP_LEG_LENGTH / 2.0f, 0},
             SHEEP_LEG_WIDTH, SHEEP_LEG_LENGTH, SHEEP_LEG_WIDTH,
             skin_lit);
    rlPopMatrix();

    // Back-right leg (same as front-left)
    Vector3 br_hip = pos;
    br_hip.x -= forward.x * leg_offset_front + right.x * leg_offset_side;
    br_hip.z -= forward.z * leg_offset_front + right.z * leg_offset_side;
    br_hip.y = hip_y;

    rlPushMatrix();
    rlTranslatef(br_hip.x, br_hip.y, br_hip.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(data->leg_swing_angle, 1, 0, 0);
    DrawCube((Vector3){0, -SHEEP_LEG_LENGTH / 2.0f, 0},
             SHEEP_LEG_WIDTH, SHEEP_LEG_LENGTH, SHEEP_LEG_WIDTH,
             skin_lit);
    rlPopMatrix();

    y += SHEEP_LEG_LENGTH;

    // ========================================================================
    // BODY (woolly cube) with breathing and bounce animations
    // ========================================================================

    // Calculate breathing offset (subtle up/down when idle)
    float breath_offset = sinf(data->idle_time * 1.5f * 2.0f * PI) * 0.02f;

    // Calculate body bounce (when walking)
    float horiz_speed = sqrtf(entity->velocity.x * entity->velocity.x +
                              entity->velocity.z * entity->velocity.z);
    float body_bounce = 0.0f;
    if (horiz_speed > 0.1f) {
        body_bounce = sinf(data->walk_animation_time * 2.0f) * 0.03f;
    }

    float body_y_offset = breath_offset + body_bounce;
    Vector3 body_center = {pos.x, y + SHEEP_BODY_HEIGHT / 2.0f + body_y_offset, pos.z};

    rlPushMatrix();
    rlTranslatef(body_center.x, body_center.y, body_center.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0},
             SHEEP_BODY_WIDTH, SHEEP_BODY_HEIGHT, SHEEP_BODY_LENGTH,
             wool_lit);
    rlPopMatrix();

    // ========================================================================
    // HEAD (cube at front with eyes) with look-around animation
    // ========================================================================
    // Head position: in front of body, slightly lower
    float head_forward_offset = SHEEP_BODY_LENGTH / 2.0f + SHEEP_HEAD_LENGTH / 2.0f;
    float head_y_offset = data->is_grazing ? -0.15f : 0.0f;  // Lower when grazing

    Vector3 head_center = body_center;
    head_center.x += forward.x * head_forward_offset;
    head_center.z += forward.z * head_forward_offset;
    head_center.y += head_y_offset;

    // Calculate head rotation with look-around offset
    float head_yaw = yaw + data->head_yaw_current;
    float head_yaw_rad = head_yaw * DEG2RAD;
    Vector3 head_forward = {sinf(head_yaw_rad), 0, cosf(head_yaw_rad)};
    Vector3 head_right = {cosf(head_yaw_rad), 0, -sinf(head_yaw_rad)};

    rlPushMatrix();
    rlTranslatef(head_center.x, head_center.y, head_center.z);
    rlRotatef(head_yaw, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0},
             SHEEP_HEAD_WIDTH, SHEEP_HEAD_HEIGHT, SHEEP_HEAD_LENGTH,
             skin_lit);
    rlPopMatrix();

    // ========================================================================
    // EYES with blink animation - follows head rotation
    // ========================================================================
    Color eye_white = entity_apply_ambient(WHITE, data->ambient_light);
    Color eye_black = entity_apply_ambient((Color){30, 30, 30, 255}, data->ambient_light);

    float eye_offset_y = 0.03f;
    float eye_offset_side = 0.08f;
    float eye_offset_forward = SHEEP_HEAD_LENGTH / 2.0f * 0.9f;

    // Left eye position (follows head rotation)
    Vector3 left_eye = head_center;
    left_eye.x += head_forward.x * eye_offset_forward - head_right.x * eye_offset_side;
    left_eye.z += head_forward.z * eye_offset_forward - head_right.z * eye_offset_side;
    left_eye.y += eye_offset_y;

    // Right eye position (follows head rotation)
    Vector3 right_eye = head_center;
    right_eye.x += head_forward.x * eye_offset_forward + head_right.x * eye_offset_side;
    right_eye.z += head_forward.z * eye_offset_forward + head_right.z * eye_offset_side;
    right_eye.y += eye_offset_y;

    // Blink animation - draw thin lines when blinking, spheres when open
    if (data->blink_progress > 0.5f) {
        // Eyes closed - draw thin horizontal lines
        rlPushMatrix();
        rlTranslatef(left_eye.x, left_eye.y, left_eye.z);
        rlRotatef(head_yaw, 0, 1, 0);
        DrawCube((Vector3){0, 0, 0}, 0.07f, 0.01f, 0.01f, eye_black);
        rlPopMatrix();

        rlPushMatrix();
        rlTranslatef(right_eye.x, right_eye.y, right_eye.z);
        rlRotatef(head_yaw, 0, 1, 0);
        DrawCube((Vector3){0, 0, 0}, 0.07f, 0.01f, 0.01f, eye_black);
        rlPopMatrix();
    } else {
        // Eyes open - draw normal spheres
        DrawSphere(left_eye, 0.04f, eye_white);
        DrawSphere(right_eye, 0.04f, eye_white);

        // Pupils
        Vector3 left_pupil = left_eye;
        left_pupil.x += head_forward.x * 0.025f;
        left_pupil.z += head_forward.z * 0.025f;
        DrawSphere(left_pupil, 0.02f, eye_black);

        Vector3 right_pupil = right_eye;
        right_pupil.x += head_forward.x * 0.025f;
        right_pupil.z += head_forward.z * 0.025f;
        DrawSphere(right_pupil, 0.02f, eye_black);
    }
}

/**
 * Destroy function for sheep
 * Frees the SheepData
 */
void sheep_destroy(Entity* entity) {
    if (!entity) return;

    if (entity->data) {
        free(entity->data);
        entity->data = NULL;
    }
}

// ============================================================================
// DAMAGE API
// ============================================================================

/**
 * Damage a sheep
 * Returns true if the sheep died
 */
bool sheep_damage(Entity* entity, int damage) {
    if (!entity || !entity->data) return false;

    SheepData* data = (SheepData*)entity->data;
    data->hp -= damage;
    data->damage_flash_timer = 0.2f;  // Flash red for 0.2s

    // Start fleeing after being hit
    data->is_fleeing = true;
    data->is_grazing = false;

    printf("[SHEEP] #%d took %d damage, HP: %d\n", entity->id, damage, data->hp);

    return data->hp <= 0;  // Returns true if dead
}

// ============================================================================
// PUBLIC SPAWN API
// ============================================================================

/**
 * Spawn a sheep with default white wool
 */
Entity* sheep_spawn(EntityManager* manager, Vector3 position) {
    // Default: white wool, pinkish skin
    Color white_wool = (Color){245, 245, 245, 255};
    return sheep_spawn_colored(manager, position, white_wool);
}

/**
 * Spawn a sheep with custom wool color
 */
Entity* sheep_spawn_colored(EntityManager* manager, Vector3 position, Color wool_color) {
    if (!manager) return NULL;

    // Create entity
    Entity* entity = entity_create(ENTITY_TYPE_SHEEP);
    if (!entity) return NULL;

    // Set position
    entity->position = position;

    // Set bounding box
    entity->bbox_min = (Vector3){-SHEEP_BODY_WIDTH / 2.0f, 0.0f, -SHEEP_BODY_LENGTH / 2.0f};
    entity->bbox_max = (Vector3){SHEEP_BODY_WIDTH / 2.0f, SHEEP_TOTAL_HEIGHT, SHEEP_BODY_LENGTH / 2.0f};

    // Set callbacks
    entity->update = sheep_update;
    entity->render = sheep_render;
    entity->destroy_data = sheep_destroy;

    // Create type-specific data
    Color skin_color = (Color){255, 200, 180, 255};  // Pinkish skin
    entity->data = sheep_create_data(wool_color, skin_color);
    if (!entity->data) {
        entity_destroy(entity);
        return NULL;
    }

    // Random initial rotation
    entity->rotation.y = entity_random_range(0, 360);

    // Add to entity manager
    entity_manager_add(manager, entity);

    printf("[SHEEP] Spawned #%d at (%.1f, %.1f, %.1f) - wool: RGB(%d,%d,%d)\n",
           entity->id, position.x, position.y, position.z,
           wool_color.r, wool_color.g, wool_color.b);

    return entity;
}
