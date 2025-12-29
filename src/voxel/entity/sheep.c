/**
 * Sheep Entity Implementation
 *
 * Passive mob with wandering and flee AI behavior
 */

#include "voxel/entity/sheep.h"
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

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

/**
 * Get a random float between min and max
 */
static float random_range(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

/**
 * Pick a random horizontal direction
 */
static Vector3 random_direction(void) {
    float angle = random_range(0, 2.0f * PI);
    return (Vector3){cosf(angle), 0, sinf(angle)};
}

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

    // AI state - start wandering
    data->wander_timer = random_range(SHEEP_WANDER_TIME_MIN, SHEEP_WANDER_TIME_MAX);
    data->graze_timer = 0.0f;
    data->is_grazing = false;
    data->is_fleeing = false;
    data->wander_direction = random_direction();

    // Default lighting (full brightness)
    data->ambient_light = (Vector3){1.0f, 1.0f, 1.0f};

    // Health & damage
    data->hp = 4;                   // 4 hits to kill
    data->damage_flash_timer = 0.0f;

    return data;
}

/**
 * Check if position is safe (has solid ground below and air at feet/head)
 */
static bool is_position_safe(World* world, Vector3 pos) {
    if (!world) return true;  // No world, assume safe

    int x = (int)floorf(pos.x);
    int y = (int)floorf(pos.y);
    int z = (int)floorf(pos.z);

    // Check ground below
    Block ground = world_get_block(world, x, y - 1, z);
    if (!block_is_solid(ground)) return false;

    // Check air at feet and body level
    Block feet = world_get_block(world, x, y, z);
    Block body = world_get_block(world, x, y + 1, z);
    if (block_is_solid(feet) || block_is_solid(body)) return false;

    return true;
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
            data->wander_timer = random_range(SHEEP_WANDER_TIME_MIN, SHEEP_WANDER_TIME_MAX);
            data->wander_direction = random_direction();
        }
    }
    else {
        // Wandering
        data->wander_timer -= dt;

        if (data->wander_timer <= 0) {
            // Time to change behavior
            if (random_range(0, 1) < SHEEP_GRAZE_CHANCE) {
                // Start grazing
                data->is_grazing = true;
                data->graze_timer = SHEEP_GRAZE_TIME;
            } else {
                // Pick new wander direction
                data->wander_direction = random_direction();
                data->wander_timer = random_range(SHEEP_WANDER_TIME_MIN, SHEEP_WANDER_TIME_MAX);
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
    // PHYSICS
    // ========================================================================

    // Apply gravity
    entity->velocity.y -= 20.0f * dt;

    // Calculate new position
    Vector3 new_pos = entity->position;
    new_pos.x += entity->velocity.x * dt;
    new_pos.z += entity->velocity.z * dt;
    new_pos.y += entity->velocity.y * dt;

    // Simple ground collision
    if (world) {
        int ground_y = (int)floorf(new_pos.y);
        Block below = world_get_block(world, (int)floorf(new_pos.x), ground_y - 1, (int)floorf(new_pos.z));
        Block at_feet = world_get_block(world, (int)floorf(new_pos.x), ground_y, (int)floorf(new_pos.z));

        // If falling into solid, stop
        if (block_is_solid(at_feet)) {
            new_pos.y = entity->position.y;
            entity->velocity.y = 0;
        }
        // If on ground, stay on ground
        else if (block_is_solid(below) && entity->velocity.y < 0) {
            new_pos.y = ground_y;
            entity->velocity.y = 0;
        }

        // Horizontal collision - if hitting wall, pick new direction
        Block ahead = world_get_block(world, (int)floorf(new_pos.x), (int)floorf(new_pos.y), (int)floorf(new_pos.z));
        if (block_is_solid(ahead)) {
            new_pos.x = entity->position.x;
            new_pos.z = entity->position.z;
            entity->velocity.x = 0;
            entity->velocity.z = 0;
            // Pick new direction when hitting wall
            if (!data->is_fleeing) {
                data->wander_direction = random_direction();
            }
        }
    }

    entity->position = new_pos;

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
    } else {
        // Idle: smoothly return to rest pose
        data->leg_swing_angle *= 0.85f;
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
 * Apply ambient lighting to a color
 */
static Color apply_ambient(Color c, Vector3 ambient) {
    return (Color){
        (unsigned char)(c.r * ambient.x),
        (unsigned char)(c.g * ambient.y),
        (unsigned char)(c.b * ambient.z),
        c.a
    };
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
    Color wool_lit = apply_ambient(data->wool_color, data->ambient_light);
    Color skin_lit = apply_ambient(data->skin_color, data->ambient_light);

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
    // BODY (woolly cube)
    // ========================================================================
    Vector3 body_center = {pos.x, y + SHEEP_BODY_HEIGHT / 2.0f, pos.z};

    rlPushMatrix();
    rlTranslatef(body_center.x, body_center.y, body_center.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0},
             SHEEP_BODY_WIDTH, SHEEP_BODY_HEIGHT, SHEEP_BODY_LENGTH,
             wool_lit);
    rlPopMatrix();

    // ========================================================================
    // HEAD (cube at front with eyes)
    // ========================================================================
    // Head position: in front of body, slightly lower
    float head_forward_offset = SHEEP_BODY_LENGTH / 2.0f + SHEEP_HEAD_LENGTH / 2.0f;
    float head_y_offset = data->is_grazing ? -0.15f : 0.0f;  // Lower when grazing

    Vector3 head_center = body_center;
    head_center.x += forward.x * head_forward_offset;
    head_center.z += forward.z * head_forward_offset;
    head_center.y += head_y_offset;

    rlPushMatrix();
    rlTranslatef(head_center.x, head_center.y, head_center.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0},
             SHEEP_HEAD_WIDTH, SHEEP_HEAD_HEIGHT, SHEEP_HEAD_LENGTH,
             skin_lit);
    rlPopMatrix();

    // Eyes - apply ambient lighting
    Color eye_white = apply_ambient(WHITE, data->ambient_light);
    Color eye_black = apply_ambient((Color){30, 30, 30, 255}, data->ambient_light);  // Slight gray so lighting shows

    float eye_offset_y = 0.03f;
    float eye_offset_side = 0.08f;
    float eye_offset_forward = SHEEP_HEAD_LENGTH / 2.0f * 0.9f;

    // Left eye
    Vector3 left_eye = head_center;
    left_eye.x += forward.x * eye_offset_forward - right.x * eye_offset_side;
    left_eye.z += forward.z * eye_offset_forward - right.z * eye_offset_side;
    left_eye.y += eye_offset_y;
    DrawSphere(left_eye, 0.04f, eye_white);

    // Left pupil
    Vector3 left_pupil = left_eye;
    left_pupil.x += forward.x * 0.025f;
    left_pupil.z += forward.z * 0.025f;
    DrawSphere(left_pupil, 0.02f, eye_black);

    // Right eye
    Vector3 right_eye = head_center;
    right_eye.x += forward.x * eye_offset_forward + right.x * eye_offset_side;
    right_eye.z += forward.z * eye_offset_forward + right.z * eye_offset_side;
    right_eye.y += eye_offset_y;
    DrawSphere(right_eye, 0.04f, eye_white);

    // Right pupil
    Vector3 right_pupil = right_eye;
    right_pupil.x += forward.x * 0.025f;
    right_pupil.z += forward.z * 0.025f;
    DrawSphere(right_pupil, 0.02f, eye_black);
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
    entity->rotation.y = random_range(0, 360);

    // Add to entity manager
    entity_manager_add(manager, entity);

    printf("[SHEEP] Spawned #%d at (%.1f, %.1f, %.1f) - wool: RGB(%d,%d,%d)\n",
           entity->id, position.x, position.y, position.z,
           wool_color.r, wool_color.g, wool_color.b);

    return entity;
}
