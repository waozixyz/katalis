/**
 * Block Human Entity Implementation
 */

#include "voxel/block_human.h"
#include "voxel/world.h"
#include <stdlib.h>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

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

    return data;
}

// ============================================================================
// ENTITY CALLBACKS
// ============================================================================

/**
 * Update function for block human
 * Currently just a placeholder - could add physics, AI, animation here
 */
void block_human_update(Entity* entity, struct World* world, float dt) {
    (void)world;  // Unused for now
    (void)dt;     // Unused for now

    if (!entity || !entity->data) return;

    // Future: Add gravity, collision, AI, animation, etc.
    // For now, block humans are static
}

/**
 * Render function for block human
 * Draws the character using Raylib primitives (sphere + cubes)
 */
void block_human_render(Entity* entity) {
    if (!entity || !entity->data) return;

    BlockHumanData* data = (BlockHumanData*)entity->data;
    Vector3 pos = entity->position;

    // Build the character from bottom to top (feet to head)
    float y = pos.y;

    // ========================================================================
    // LEGS (2 thin blocks, spaced apart)
    // ========================================================================
    float leg_spacing = 0.125f;  // Half the spacing between legs
    Vector3 left_leg_center = {
        pos.x - leg_spacing,
        y + BLOCK_HUMAN_LEG_LENGTH / 2.0f,
        pos.z
    };
    Vector3 right_leg_center = {
        pos.x + leg_spacing,
        y + BLOCK_HUMAN_LEG_LENGTH / 2.0f,
        pos.z
    };
    Vector3 leg_size = {BLOCK_HUMAN_LEG_WIDTH, BLOCK_HUMAN_LEG_LENGTH, BLOCK_HUMAN_LEG_DEPTH};

    DrawCubeV(left_leg_center, leg_size, data->appearance.leg_color);
    DrawCubeV(right_leg_center, leg_size, data->appearance.leg_color);

    // Move up past legs
    y += BLOCK_HUMAN_LEG_LENGTH;

    // ========================================================================
    // TORSO (tall rectangular block)
    // ========================================================================
    Vector3 torso_center = {
        pos.x,
        y + BLOCK_HUMAN_TORSO_HEIGHT / 2.0f,
        pos.z
    };
    Vector3 torso_size = {BLOCK_HUMAN_TORSO_WIDTH, BLOCK_HUMAN_TORSO_HEIGHT, BLOCK_HUMAN_TORSO_DEPTH};

    DrawCubeV(torso_center, torso_size, data->appearance.torso_color);

    // ========================================================================
    // ARMS (2 thin blocks, attached to upper torso sides)
    // ========================================================================
    float arm_attach_height = y + BLOCK_HUMAN_TORSO_HEIGHT * 0.75f;  // Near shoulder level
    float arm_offset_x = (BLOCK_HUMAN_TORSO_WIDTH / 2.0f) + (BLOCK_HUMAN_ARM_WIDTH / 2.0f);

    Vector3 left_arm_center = {
        pos.x - arm_offset_x,
        arm_attach_height - BLOCK_HUMAN_ARM_LENGTH / 2.0f,
        pos.z
    };
    Vector3 right_arm_center = {
        pos.x + arm_offset_x,
        arm_attach_height - BLOCK_HUMAN_ARM_LENGTH / 2.0f,
        pos.z
    };
    Vector3 arm_size = {BLOCK_HUMAN_ARM_WIDTH, BLOCK_HUMAN_ARM_LENGTH, BLOCK_HUMAN_ARM_DEPTH};

    DrawCubeV(left_arm_center, arm_size, data->appearance.arm_color);
    DrawCubeV(right_arm_center, arm_size, data->appearance.arm_color);

    // Move up past torso
    y += BLOCK_HUMAN_TORSO_HEIGHT;

    // ========================================================================
    // HEAD (sphere)
    // ========================================================================
    Vector3 head_center = {
        pos.x,
        y + BLOCK_HUMAN_HEAD_SIZE,
        pos.z
    };

    DrawSphere(head_center, BLOCK_HUMAN_HEAD_SIZE, data->appearance.head_color);

    // Optional: Draw wireframe for debugging
    #ifdef BLOCK_HUMAN_DEBUG
    DrawSphereWires(head_center, BLOCK_HUMAN_HEAD_SIZE, 8, 8, BLACK);
    DrawCubeWiresV(torso_center, torso_size, BLACK);
    DrawCubeWiresV(left_arm_center, arm_size, BLACK);
    DrawCubeWiresV(right_arm_center, arm_size, BLACK);
    DrawCubeWiresV(left_leg_center, leg_size, BLACK);
    DrawCubeWiresV(right_leg_center, leg_size, BLACK);
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
