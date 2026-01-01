/**
 * Block Human Entity Implementation
 */

#include "voxel/entity/block_human.h"
#include "voxel/entity/entity_utils.h"
#include "voxel/world/world.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

// Use entity_apply_ambient from entity_utils.h instead of local duplicate

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

/**
 * Create block human data with default colors
 */
static BlockHumanData* block_human_create_data(Color head, Color torso, Color legs) {
    BlockHumanData* data = (BlockHumanData*)malloc(sizeof(BlockHumanData));
    if (!data) {
        printf("[BLOCK_HUMAN] Failed to allocate data\n");
        return NULL;
    }

    data->appearance.head_color = head;
    data->appearance.torso_color = torso;
    data->appearance.arm_color = torso;  // Arms match torso
    data->appearance.leg_color = legs;

    // Initialize animation state
    data->walk_animation_time = 0.0f;
    data->arm_swing_angle = 0.0f;
    data->leg_swing_angle = 0.0f;

    // Default lighting (full brightness)
    data->ambient_light = (Vector3){1.0f, 1.0f, 1.0f};

    return data;
}

// ============================================================================
// ENTITY CALLBACKS
// ============================================================================

/**
 * Update function for block human
 * Updates walk animation based on velocity
 */
void block_human_update(Entity* entity, struct World* world, float dt) {
    if (!entity || !entity->data) return;

    BlockHumanData* data = (BlockHumanData*)entity->data;

    // Update ambient lighting from world time
    if (world) {
        data->ambient_light = world_get_ambient_color(world->time_of_day);
    }

    // Update walk animation based on horizontal velocity
    float horiz_speed = sqrtf(entity->velocity.x * entity->velocity.x +
                              entity->velocity.z * entity->velocity.z);
    if (horiz_speed > 0.5f) {
        // Moving: animate walk cycle
        data->walk_animation_time += dt * horiz_speed * 0.8f;
        data->arm_swing_angle = sinf(data->walk_animation_time) * 30.0f;
        data->leg_swing_angle = sinf(data->walk_animation_time) * 25.0f;
    } else {
        // Idle: smoothly return to rest pose
        data->arm_swing_angle *= ENTITY_ANIMATION_DAMPING;
        data->leg_swing_angle *= ENTITY_ANIMATION_DAMPING;
    }
}

/**
 * Render function for block human
 * Draws the character using Raylib primitives (sphere + cubes) with walk animation and eyes
 */
void block_human_render(Entity* entity) {
    if (!entity || !entity->data) return;

    BlockHumanData* data = (BlockHumanData*)entity->data;
    Vector3 pos = entity->position;

    // Apply ambient lighting to colors
    Color head_lit = entity_apply_ambient(data->appearance.head_color, data->ambient_light);
    Color torso_lit = entity_apply_ambient(data->appearance.torso_color, data->ambient_light);
    Color arm_lit = entity_apply_ambient(data->appearance.arm_color, data->ambient_light);
    Color leg_lit = entity_apply_ambient(data->appearance.leg_color, data->ambient_light);

    // Get entity rotation (yaw for facing direction)
    float yaw = entity->rotation.y;  // Y rotation is yaw
    float yaw_rad = yaw * DEG2RAD;
    float cos_yaw = cosf(yaw_rad);
    float sin_yaw = sinf(yaw_rad);

    // Build the character from bottom to top (feet to head)
    float y = pos.y;

    // ========================================================================
    // LEGS (2 thin blocks, animated with walking)
    // ========================================================================
    float leg_spacing = 0.125f;  // Half the spacing between legs

    // Left leg pivot point (at hip)
    float left_hip_x = pos.x - leg_spacing * cos_yaw;
    float left_hip_z = pos.z + leg_spacing * sin_yaw;
    float hip_y = y + BLOCK_HUMAN_LEG_LENGTH;

    // Right leg pivot point
    float right_hip_x = pos.x + leg_spacing * cos_yaw;
    float right_hip_z = pos.z - leg_spacing * sin_yaw;

    // Left leg (animated)
    rlPushMatrix();
    rlTranslatef(left_hip_x, hip_y, left_hip_z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(data->leg_swing_angle, 1, 0, 0);
    DrawCube((Vector3){0, -BLOCK_HUMAN_LEG_LENGTH / 2.0f, 0},
             BLOCK_HUMAN_LEG_WIDTH, BLOCK_HUMAN_LEG_LENGTH, BLOCK_HUMAN_LEG_DEPTH,
             leg_lit);
    rlPopMatrix();

    // Right leg (animated, opposite swing)
    rlPushMatrix();
    rlTranslatef(right_hip_x, hip_y, right_hip_z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(-data->leg_swing_angle, 1, 0, 0);
    DrawCube((Vector3){0, -BLOCK_HUMAN_LEG_LENGTH / 2.0f, 0},
             BLOCK_HUMAN_LEG_WIDTH, BLOCK_HUMAN_LEG_LENGTH, BLOCK_HUMAN_LEG_DEPTH,
             leg_lit);
    rlPopMatrix();

    // Move up past legs
    y += BLOCK_HUMAN_LEG_LENGTH;

    // ========================================================================
    // TORSO (tall rectangular block, rotated with entity)
    // ========================================================================
    Vector3 torso_center = {pos.x, y + BLOCK_HUMAN_TORSO_HEIGHT / 2.0f, pos.z};

    rlPushMatrix();
    rlTranslatef(torso_center.x, torso_center.y, torso_center.z);
    rlRotatef(yaw, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0},
             BLOCK_HUMAN_TORSO_WIDTH, BLOCK_HUMAN_TORSO_HEIGHT, BLOCK_HUMAN_TORSO_DEPTH,
             torso_lit);
    rlPopMatrix();

    // ========================================================================
    // ARMS (2 thin blocks, animated with walking)
    // ========================================================================
    float arm_attach_height = y + BLOCK_HUMAN_TORSO_HEIGHT * 0.85f;  // Shoulder level
    float arm_offset = (BLOCK_HUMAN_TORSO_WIDTH / 2.0f) + (BLOCK_HUMAN_ARM_WIDTH / 2.0f);

    // Left shoulder position
    float left_shoulder_x = pos.x - arm_offset * cos_yaw;
    float left_shoulder_z = pos.z + arm_offset * sin_yaw;

    // Right shoulder position
    float right_shoulder_x = pos.x + arm_offset * cos_yaw;
    float right_shoulder_z = pos.z - arm_offset * sin_yaw;

    // Left arm (swings opposite to left leg)
    rlPushMatrix();
    rlTranslatef(left_shoulder_x, arm_attach_height, left_shoulder_z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(-data->arm_swing_angle, 1, 0, 0);
    DrawCube((Vector3){0, -BLOCK_HUMAN_ARM_LENGTH / 2.0f, 0},
             BLOCK_HUMAN_ARM_WIDTH, BLOCK_HUMAN_ARM_LENGTH, BLOCK_HUMAN_ARM_DEPTH,
             arm_lit);
    rlPopMatrix();

    // Right arm (swings same as left leg)
    rlPushMatrix();
    rlTranslatef(right_shoulder_x, arm_attach_height, right_shoulder_z);
    rlRotatef(yaw, 0, 1, 0);
    rlRotatef(data->arm_swing_angle, 1, 0, 0);
    DrawCube((Vector3){0, -BLOCK_HUMAN_ARM_LENGTH / 2.0f, 0},
             BLOCK_HUMAN_ARM_WIDTH, BLOCK_HUMAN_ARM_LENGTH, BLOCK_HUMAN_ARM_DEPTH,
             arm_lit);
    rlPopMatrix();

    // Move up past torso
    y += BLOCK_HUMAN_TORSO_HEIGHT;

    // ========================================================================
    // HEAD (sphere with eyes)
    // ========================================================================
    Vector3 head_center = {pos.x, y + BLOCK_HUMAN_HEAD_SIZE, pos.z};

    DrawSphere(head_center, BLOCK_HUMAN_HEAD_SIZE, head_lit);

    // Eye colors with ambient lighting
    Color eye_white = entity_apply_ambient(WHITE, data->ambient_light);
    Color eye_black = entity_apply_ambient((Color){30, 30, 30, 255}, data->ambient_light);

    // Eye parameters
    float eye_offset_y = 0.05f;       // Slightly above center
    float eye_offset_side = 0.12f;    // Distance apart (half)
    float eye_offset_forward = BLOCK_HUMAN_HEAD_SIZE * 0.9f;  // On head surface

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

    // Optional: Draw wireframe for debugging
    #ifdef BLOCK_HUMAN_DEBUG
    DrawSphereWires(head_center, BLOCK_HUMAN_HEAD_SIZE, 8, 8, BLACK);
    #endif
}

/**
 * Destroy function for block human
 * Frees the BlockHumanData
 */
void block_human_destroy(Entity* entity) {
    if (!entity) return;

    if (entity->data) {
        free(entity->data);
        entity->data = NULL;
    }
}

// ============================================================================
// PUBLIC SPAWN API
// ============================================================================

/**
 * Spawn a block human with default colors
 */
Entity* block_human_spawn(EntityManager* manager, Vector3 position) {
    // Default colors: skin tone head, blue shirt, brown pants
    Color default_head = (Color){255, 220, 177, 255};   // Skin tone
    Color default_torso = (Color){70, 130, 180, 255};   // Steel blue
    Color default_legs = (Color){101, 67, 33, 255};     // Brown

    return block_human_spawn_colored(manager, position, default_head, default_torso, default_legs);
}

/**
 * Spawn a block human with custom colors
 */
Entity* block_human_spawn_colored(EntityManager* manager, Vector3 position,
                                   Color head_color, Color torso_color, Color leg_color) {
    if (!manager) return NULL;

    // Create entity
    Entity* entity = entity_create(ENTITY_TYPE_BLOCK_HUMAN);
    if (!entity) return NULL;

    // Set position
    entity->position = position;

    // Set bounding box (approximate human size)
    entity->bbox_min = (Vector3){-0.3f, 0.0f, -0.3f};
    entity->bbox_max = (Vector3){0.3f, BLOCK_HUMAN_TOTAL_HEIGHT, 0.3f};

    // Set callbacks
    entity->update = block_human_update;
    entity->render = block_human_render;
    entity->destroy_data = block_human_destroy;

    // Create type-specific data
    entity->data = block_human_create_data(head_color, torso_color, leg_color);
    if (!entity->data) {
        entity_destroy(entity);
        return NULL;
    }

    // Add to entity manager
    entity_manager_add(manager, entity);

    printf("[BLOCK_HUMAN] Spawned at (%.1f, %.1f, %.1f)\n",
           position.x, position.y, position.z);

    return entity;
}
