/**
 * Pig Entity Implementation
 *
 * Passive mob with wandering and flee AI behavior
 */

#include "voxel/entity/pig.h"
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
 * Create pig data
 */
static PigData* pig_create_data(void) {
    PigData* data = (PigData*)malloc(sizeof(PigData));
    if (!data) {
        printf("[PIG] Failed to allocate data\n");
        return NULL;
    }

    // Pink colors
    data->body_color = (Color){255, 180, 180, 255};   // Pink body
    data->snout_color = (Color){255, 150, 150, 255};  // Darker pink snout

    // Animation state
    data->walk_animation_time = 0.0f;
    data->leg_swing_angle = 0.0f;
    data->tail_wiggle_angle = 0.0f;
    data->idle_time = 0.0f;
    data->head_yaw_target = 0.0f;
    data->head_yaw_current = 0.0f;
    data->head_look_timer = entity_random_range(2.0f, 4.0f);
    data->blink_timer = entity_random_range(3.0f, 6.0f);
    data->blink_progress = 0.0f;
    data->ear_twitch_timer = entity_random_range(2.0f, 5.0f);
    data->ear_twitch_angle = 0.0f;

    // AI state - start wandering
    data->wander_timer = entity_random_range(PIG_WANDER_TIME_MIN, PIG_WANDER_TIME_MAX);
    data->idle_timer = 0.0f;
    data->is_idle = false;
    data->is_fleeing = false;
    data->wander_direction = entity_random_direction();

    // Default lighting (full brightness)
    data->ambient_light = (Vector3){1.0f, 1.0f, 1.0f};

    // Health & damage
    data->hp = 5;                   // 5 hits to kill (more than sheep)
    data->damage_flash_timer = 0.0f;

    // Jump
    data->jump_cooldown = 0.0f;

    return data;
}

// ============================================================================
// ENTITY CALLBACKS
// ============================================================================

/**
 * Update function for pig
 * Handles AI behavior (wandering, idling, fleeing) and animation
 */
void pig_update(Entity* entity, struct World* world, float dt) {
    if (!entity || !entity->data) return;

    PigData* data = (PigData*)entity->data;

    // Get player position for flee behavior
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
            flee_direction = Vector3Scale(to_player, -1.0f / player_dist);
        }
    }

    // ========================================================================
    // AI STATE MACHINE
    // ========================================================================

    // Check if should start fleeing
    if (has_player && player_dist < PIG_FLEE_DISTANCE) {
        data->is_fleeing = true;
        data->is_idle = false;
    }
    // Check if can stop fleeing
    else if (data->is_fleeing && player_dist > PIG_SAFE_DISTANCE) {
        data->is_fleeing = false;
    }

    // Update based on state
    if (data->is_fleeing) {
        // Fleeing: run away from player
        entity->velocity.x = flee_direction.x * PIG_FLEE_SPEED;
        entity->velocity.z = flee_direction.z * PIG_FLEE_SPEED;

        // Face away from player
        entity->rotation.y = atan2f(flee_direction.x, flee_direction.z) * RAD2DEG;
    }
    else if (data->is_idle) {
        // Idle: stand still, wiggle tail
        entity->velocity.x = 0;
        entity->velocity.z = 0;

        data->idle_timer -= dt;
        if (data->idle_timer <= 0) {
            data->is_idle = false;
            data->wander_timer = entity_random_range(PIG_WANDER_TIME_MIN, PIG_WANDER_TIME_MAX);
            data->wander_direction = entity_random_direction();
        }
    }
    else {
        // Wandering
        data->wander_timer -= dt;

        if (data->wander_timer <= 0) {
            // Time to change behavior
            if (entity_random_range(0, 1) < PIG_IDLE_CHANCE) {
                // Start idling
                data->is_idle = true;
                data->idle_timer = PIG_IDLE_TIME;
            } else {
                // Pick new wander direction
                data->wander_direction = entity_random_direction();
                data->wander_timer = entity_random_range(PIG_WANDER_TIME_MIN, PIG_WANDER_TIME_MAX);
            }
        }

        // Move in wander direction
        entity->velocity.x = data->wander_direction.x * PIG_WANDER_SPEED;
        entity->velocity.z = data->wander_direction.z * PIG_WANDER_SPEED;

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
        entity->velocity.y = PIG_JUMP_VELOCITY;
        data->jump_cooldown = PIG_JUMP_COOLDOWN;
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
        float anim_speed = data->is_fleeing ? 1.5f : 0.8f;
        data->walk_animation_time += dt * horiz_speed * anim_speed;
        data->leg_swing_angle = sinf(data->walk_animation_time) * 20.0f;
        data->idle_time = 0.0f;  // Reset idle time when moving
    } else {
        // Idle: smoothly return to rest pose
        data->leg_swing_angle *= ENTITY_ANIMATION_DAMPING;
        data->idle_time += dt;  // Accumulate idle time for breathing
    }

    // Tail wiggle animation (always wiggles slightly, more when idle)
    float wiggle_speed = data->is_idle ? 8.0f : 3.0f;
    data->tail_wiggle_angle = sinf(data->walk_animation_time * wiggle_speed) * 15.0f;

    // Head look-around animation (when idle and not fleeing)
    if (!data->is_fleeing && horiz_speed < 0.1f) {
        data->head_look_timer -= dt;
        if (data->head_look_timer <= 0) {
            data->head_yaw_target = entity_random_range(-30.0f, 30.0f);
            data->head_look_timer = entity_random_range(2.0f, 4.0f);
        }
    } else {
        // Reset head to forward when moving or fleeing
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

    // Ear twitch animation
    data->ear_twitch_timer -= dt;
    if (data->ear_twitch_timer <= 0) {
        data->ear_twitch_angle = 10.0f;  // Start twitch
        data->ear_twitch_timer = entity_random_range(2.0f, 5.0f);
    }
    data->ear_twitch_angle *= 0.9f;  // Decay back to 0

    // ========================================================================
    // LIGHTING
    // ========================================================================

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
 * Render function for pig
 * Draws a blocky Minecraft-style pig
 */
void pig_render(Entity* entity) {
    if (!entity || !entity->data) return;

    PigData* data = (PigData*)entity->data;
    Vector3 pos = entity->position;

    // Apply ambient lighting to colors
    Color body_lit = entity_apply_ambient(data->body_color, data->ambient_light);
    Color snout_lit = entity_apply_ambient(data->snout_color, data->ambient_light);

    // Apply red flash when damaged
    if (data->damage_flash_timer > 0) {
        body_lit = (Color){255, 100, 100, 255};
        snout_lit = (Color){255, 100, 100, 255};
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
    // LEGS (4 short legs with walk animation)
    // ========================================================================
    float leg_offset_side = PIG_BODY_WIDTH / 2.0f - PIG_LEG_WIDTH / 2.0f;
    float leg_offset_front = PIG_BODY_LENGTH / 2.0f - PIG_LEG_WIDTH;
    float hip_y = y + PIG_LEG_LENGTH;

    // Front-left leg
    Vector3 fl_hip = pos;
    fl_hip.x += forward.x * leg_offset_front - right.x * leg_offset_side;
    fl_hip.z += forward.z * leg_offset_front - right.z * leg_offset_side;
    fl_hip.y = hip_y;

    rlPushMatrix();
    rlTranslatef(fl_hip.x, fl_hip.y, fl_hip.z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(data->leg_swing_angle, 1, 0, 0);
    DrawCube((Vector3){0, -PIG_LEG_LENGTH / 2.0f, 0},
             PIG_LEG_WIDTH, PIG_LEG_LENGTH, PIG_LEG_WIDTH,
             body_lit);
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
    DrawCube((Vector3){0, -PIG_LEG_LENGTH / 2.0f, 0},
             PIG_LEG_WIDTH, PIG_LEG_LENGTH, PIG_LEG_WIDTH,
             body_lit);
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
    DrawCube((Vector3){0, -PIG_LEG_LENGTH / 2.0f, 0},
             PIG_LEG_WIDTH, PIG_LEG_LENGTH, PIG_LEG_WIDTH,
             body_lit);
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
    DrawCube((Vector3){0, -PIG_LEG_LENGTH / 2.0f, 0},
             PIG_LEG_WIDTH, PIG_LEG_LENGTH, PIG_LEG_WIDTH,
             body_lit);
    rlPopMatrix();

    y += PIG_LEG_LENGTH;

    // ========================================================================
    // BODY (stocky cube) with breathing and bounce animations
    // ========================================================================

    // Calculate breathing offset (subtle up/down when idle)
    float breath_offset = sinf(data->idle_time * 1.5f * 2.0f * PI) * 0.02f;

    // Calculate body bounce (when walking)
    float horiz_speed = sqrtf(entity->velocity.x * entity->velocity.x +
                              entity->velocity.z * entity->velocity.z);
    float body_bounce = 0.0f;
    if (horiz_speed > 0.1f) {
        body_bounce = sinf(data->walk_animation_time * 2.0f) * 0.025f;
    }

    float body_y_offset = breath_offset + body_bounce;
    Vector3 body_center = {pos.x, y + PIG_BODY_HEIGHT / 2.0f + body_y_offset, pos.z};

    rlPushMatrix();
    rlTranslatef(body_center.x, body_center.y, body_center.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0},
             PIG_BODY_WIDTH, PIG_BODY_HEIGHT, PIG_BODY_LENGTH,
             body_lit);
    rlPopMatrix();

    // ========================================================================
    // TAIL (small wiggling cube at back)
    // ========================================================================
    float tail_offset = PIG_BODY_LENGTH / 2.0f + 0.05f;
    Vector3 tail_pos = body_center;
    tail_pos.x -= forward.x * tail_offset;
    tail_pos.z -= forward.z * tail_offset;
    tail_pos.y += 0.05f;

    rlPushMatrix();
    rlTranslatef(tail_pos.x, tail_pos.y, tail_pos.z);
    rlRotatef(yaw + data->tail_wiggle_angle, 0, 1, 0);
    DrawCube((Vector3){0, 0, -0.03f}, 0.06f, 0.06f, 0.08f, snout_lit);
    rlPopMatrix();

    // ========================================================================
    // HEAD (cube at front with snout, ears, and eyes) with look-around animation
    // ========================================================================
    float head_forward_offset = PIG_BODY_LENGTH / 2.0f + PIG_HEAD_LENGTH / 2.0f;

    Vector3 head_center = body_center;
    head_center.x += forward.x * head_forward_offset;
    head_center.z += forward.z * head_forward_offset;

    // Calculate head rotation with look-around offset
    float head_yaw = yaw + data->head_yaw_current;
    float head_yaw_rad = head_yaw * DEG2RAD;
    Vector3 head_forward = {sinf(head_yaw_rad), 0, cosf(head_yaw_rad)};
    Vector3 head_right = {cosf(head_yaw_rad), 0, -sinf(head_yaw_rad)};

    rlPushMatrix();
    rlTranslatef(head_center.x, head_center.y, head_center.z);
    rlRotatef(head_yaw, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0},
             PIG_HEAD_WIDTH, PIG_HEAD_HEIGHT, PIG_HEAD_LENGTH,
             body_lit);
    rlPopMatrix();

    // ========================================================================
    // SNOUT (protruding from face) - follows head rotation
    // ========================================================================
    float snout_offset = PIG_HEAD_LENGTH / 2.0f + PIG_SNOUT_LENGTH / 2.0f;
    Vector3 snout_pos = head_center;
    snout_pos.x += head_forward.x * snout_offset;
    snout_pos.z += head_forward.z * snout_offset;
    snout_pos.y -= 0.05f;  // Slightly lower

    rlPushMatrix();
    rlTranslatef(snout_pos.x, snout_pos.y, snout_pos.z);
    rlRotatef(head_yaw, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0},
             PIG_SNOUT_WIDTH, PIG_SNOUT_HEIGHT, PIG_SNOUT_LENGTH,
             snout_lit);
    rlPopMatrix();

    // Nostrils (two small dark spots on snout)
    Color nostril_color = entity_apply_ambient((Color){100, 60, 60, 255}, data->ambient_light);
    float nostril_forward = snout_offset + PIG_SNOUT_LENGTH / 2.0f * 0.9f;
    float nostril_side = 0.03f;

    Vector3 left_nostril = head_center;
    left_nostril.x += head_forward.x * nostril_forward - head_right.x * nostril_side;
    left_nostril.z += head_forward.z * nostril_forward - head_right.z * nostril_side;
    left_nostril.y -= 0.05f;
    DrawSphere(left_nostril, 0.015f, nostril_color);

    Vector3 right_nostril = head_center;
    right_nostril.x += head_forward.x * nostril_forward + head_right.x * nostril_side;
    right_nostril.z += head_forward.z * nostril_forward + head_right.z * nostril_side;
    right_nostril.y -= 0.05f;
    DrawSphere(right_nostril, 0.015f, nostril_color);

    // ========================================================================
    // EARS (two small cubes on top of head) with twitch animation
    // ========================================================================
    float ear_y_offset = PIG_HEAD_HEIGHT / 2.0f + PIG_EAR_SIZE / 2.0f;
    float ear_side_offset = PIG_HEAD_WIDTH / 2.0f - PIG_EAR_SIZE / 2.0f;

    // Left ear
    Vector3 left_ear = head_center;
    left_ear.x -= head_right.x * ear_side_offset;
    left_ear.z -= head_right.z * ear_side_offset;
    left_ear.y += ear_y_offset;

    rlPushMatrix();
    rlTranslatef(left_ear.x, left_ear.y, left_ear.z);
    rlRotatef(head_yaw, 0, 1, 0);
    rlRotatef(-15 - data->ear_twitch_angle, 0, 0, 1);  // Tilt outward + twitch
    DrawCube((Vector3){0, 0, 0}, PIG_EAR_SIZE, PIG_EAR_SIZE * 0.6f, PIG_EAR_SIZE, body_lit);
    rlPopMatrix();

    // Right ear
    Vector3 right_ear = head_center;
    right_ear.x += head_right.x * ear_side_offset;
    right_ear.z += head_right.z * ear_side_offset;
    right_ear.y += ear_y_offset;

    rlPushMatrix();
    rlTranslatef(right_ear.x, right_ear.y, right_ear.z);
    rlRotatef(head_yaw, 0, 1, 0);
    rlRotatef(15 + data->ear_twitch_angle, 0, 0, 1);  // Tilt outward + twitch
    DrawCube((Vector3){0, 0, 0}, PIG_EAR_SIZE, PIG_EAR_SIZE * 0.6f, PIG_EAR_SIZE, body_lit);
    rlPopMatrix();

    // ========================================================================
    // EYES with blink animation - follows head rotation
    // ========================================================================
    Color eye_white = entity_apply_ambient(WHITE, data->ambient_light);
    Color eye_black = entity_apply_ambient((Color){30, 30, 30, 255}, data->ambient_light);

    float eye_offset_y = 0.03f;
    float eye_offset_side = 0.1f;
    float eye_offset_forward = PIG_HEAD_LENGTH / 2.0f * 0.9f;

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
        DrawCube((Vector3){0, 0, 0}, 0.06f, 0.01f, 0.01f, eye_black);
        rlPopMatrix();

        rlPushMatrix();
        rlTranslatef(right_eye.x, right_eye.y, right_eye.z);
        rlRotatef(head_yaw, 0, 1, 0);
        DrawCube((Vector3){0, 0, 0}, 0.06f, 0.01f, 0.01f, eye_black);
        rlPopMatrix();
    } else {
        // Eyes open - draw normal spheres
        DrawSphere(left_eye, 0.035f, eye_white);
        DrawSphere(right_eye, 0.035f, eye_white);

        // Pupils
        Vector3 left_pupil = left_eye;
        left_pupil.x += head_forward.x * 0.02f;
        left_pupil.z += head_forward.z * 0.02f;
        DrawSphere(left_pupil, 0.018f, eye_black);

        Vector3 right_pupil = right_eye;
        right_pupil.x += head_forward.x * 0.02f;
        right_pupil.z += head_forward.z * 0.02f;
        DrawSphere(right_pupil, 0.018f, eye_black);
    }
}

/**
 * Destroy function for pig
 * Frees the PigData
 */
void pig_destroy(Entity* entity) {
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
 * Damage a pig
 * Returns true if the pig died
 */
bool pig_damage(Entity* entity, int damage) {
    if (!entity || !entity->data) return false;

    PigData* data = (PigData*)entity->data;
    data->hp -= damage;
    data->damage_flash_timer = 0.2f;  // Flash red for 0.2s

    // Start fleeing after being hit
    data->is_fleeing = true;
    data->is_idle = false;

    printf("[PIG] #%d took %d damage, HP: %d\n", entity->id, damage, data->hp);

    return data->hp <= 0;  // Returns true if dead
}

// ============================================================================
// PUBLIC SPAWN API
// ============================================================================

/**
 * Spawn a pig
 */
Entity* pig_spawn(EntityManager* manager, Vector3 position) {
    if (!manager) return NULL;

    // Create entity
    Entity* entity = entity_create(ENTITY_TYPE_PIG);
    if (!entity) return NULL;

    // Set position
    entity->position = position;

    // Set bounding box
    entity->bbox_min = (Vector3){-PIG_BODY_WIDTH / 2.0f, 0.0f, -PIG_BODY_LENGTH / 2.0f};
    entity->bbox_max = (Vector3){PIG_BODY_WIDTH / 2.0f, PIG_TOTAL_HEIGHT, PIG_BODY_LENGTH / 2.0f};

    // Set callbacks
    entity->update = pig_update;
    entity->render = pig_render;
    entity->destroy_data = pig_destroy;

    // Create type-specific data
    entity->data = pig_create_data();
    if (!entity->data) {
        entity_destroy(entity);
        return NULL;
    }

    // Random initial rotation
    entity->rotation.y = entity_random_range(0, 360);

    // Add to entity manager
    entity_manager_add(manager, entity);

    return entity;
}
