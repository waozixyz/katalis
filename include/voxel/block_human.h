/**
 * Block Human Entity - Minecraft-style humanoid character
 *
 * A simple block-based humanoid with:
 * - Sphere head
 * - Block torso
 * - Block arms (2)
 * - Block legs (2)
 */

#ifndef BLOCK_HUMAN_H
#define BLOCK_HUMAN_H

#include "voxel/entity.h"
#include <raylib.h>

// ============================================================================
// BODY PROPORTIONS (Minecraft-style)
// ============================================================================

// Head (sphere)
#define BLOCK_HUMAN_HEAD_SIZE 0.5f              // Sphere radius

// Torso (tall rectangular block)
#define BLOCK_HUMAN_TORSO_WIDTH 0.5f
#define BLOCK_HUMAN_TORSO_HEIGHT 0.75f
#define BLOCK_HUMAN_TORSO_DEPTH 0.25f

// Arms (thin rectangular blocks)
#define BLOCK_HUMAN_ARM_WIDTH 0.15f
#define BLOCK_HUMAN_ARM_LENGTH 0.6f
#define BLOCK_HUMAN_ARM_DEPTH 0.15f

// Legs (thin rectangular blocks)
#define BLOCK_HUMAN_LEG_WIDTH 0.15f
#define BLOCK_HUMAN_LEG_LENGTH 0.6f
#define BLOCK_HUMAN_LEG_DEPTH 0.15f

// Total height calculation
#define BLOCK_HUMAN_TOTAL_HEIGHT \
    (BLOCK_HUMAN_LEG_LENGTH + BLOCK_HUMAN_TORSO_HEIGHT + BLOCK_HUMAN_HEAD_SIZE * 2.0f)

// ============================================================================
// BLOCK HUMAN DATA
// ============================================================================

/**
 * Block human appearance colors
 */
typedef struct {
    Color head_color;       // Skin tone for face
    Color torso_color;      // Shirt/body color
    Color arm_color;        // Arm color (usually matches torso)
    Color leg_color;        // Pants/leg color
} BlockHumanAppearance;

/**
 * Block human entity data
 */
typedef struct {
    BlockHumanAppearance appearance;

    // Future animation/state data:
    // float arm_swing_angle;
    // float leg_swing_angle;
    // AnimationState state;
} BlockHumanData;

// ============================================================================
// BLOCK HUMAN API
// ============================================================================

/**
 * Spawn a new block human entity
 * @param manager Entity manager to add to
 * @param position World position (at feet)
 * @return Pointer to spawned entity
 */
Entity* block_human_spawn(EntityManager* manager, Vector3 position);

/**
 * Spawn a block human with custom colors
 * @param manager Entity manager to add to
 * @param position World position (at feet)
 * @param head_color Color for head
 * @param torso_color Color for torso and arms
 * @param leg_color Color for legs
 * @return Pointer to spawned entity
 */
Entity* block_human_spawn_colored(EntityManager* manager, Vector3 position,
                                   Color head_color, Color torso_color, Color leg_color);

// ============================================================================
// INTERNAL CALLBACKS (called by entity system)
// ============================================================================

/**
 * Update callback for block human entities
 */
void block_human_update(Entity* entity, struct World* world, float dt);

/**
 * Render callback for block human entities
 */
void block_human_render(Entity* entity);

/**
 * Destroy callback for block human entities
 */
void block_human_destroy(Entity* entity);

#endif // BLOCK_HUMAN_H
