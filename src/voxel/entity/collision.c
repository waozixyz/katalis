/**
 * Entity Collision System Implementation
 *
 * Provides AABB-based collision detection for entities against the voxel world.
 */

#include "voxel/entity/collision.h"
#include "voxel/world/world.h"
#include "voxel/core/block.h"
#include <math.h>

// Shrink bounding box slightly to prevent edge sticking
#define COLLISION_EPSILON 0.01f

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

/**
 * Check if a point is inside a solid block
 */
bool entity_is_solid_at(struct World* world, float x, float y, float z) {
    if (!world) return false;

    int bx = (int)floorf(x);
    int by = (int)floorf(y);
    int bz = (int)floorf(z);

    Block block = world_get_block(world, bx, by, bz);
    return block_is_solid(block);
}

// ============================================================================
// COLLISION DETECTION
// ============================================================================

/**
 * Check if an AABB at the given position collides with any solid blocks
 *
 * Uses 12-point sampling: 4 corners at 3 height levels
 * Shrinks bounding box slightly to prevent edge sticking
 */
bool entity_check_collision(struct World* world, Vector3 pos, Vector3 bbox_min, Vector3 bbox_max) {
    if (!world) return false;

    // Calculate world-space bounding box corners (shrunk slightly)
    float x_min = pos.x + bbox_min.x + COLLISION_EPSILON;
    float x_max = pos.x + bbox_max.x - COLLISION_EPSILON;
    float z_min = pos.z + bbox_min.z + COLLISION_EPSILON;
    float z_max = pos.z + bbox_max.z - COLLISION_EPSILON;

    // Calculate 3 height levels for checking
    float height = bbox_max.y - bbox_min.y;
    float heights[3] = {
        pos.y + bbox_min.y + 0.1f,           // Bottom (slightly above feet)
        pos.y + bbox_min.y + height * 0.5f,  // Middle
        pos.y + bbox_max.y - 0.1f            // Top (slightly below head)
    };

    // Check all 12 points (4 corners x 3 heights)
    for (int h = 0; h < 3; h++) {
        float y = heights[h];

        // Skip if height is invalid
        if (y < 0) continue;

        // Check 4 corners at this height
        if (entity_is_solid_at(world, x_min, y, z_min)) return true;
        if (entity_is_solid_at(world, x_max, y, z_min)) return true;
        if (entity_is_solid_at(world, x_min, y, z_max)) return true;
        if (entity_is_solid_at(world, x_max, y, z_max)) return true;
    }

    return false;
}

/**
 * Push an entity out of any blocks it's stuck in
 * Returns true if entity was stuck and pushed out
 */
static bool entity_push_out_of_blocks(Entity* entity, struct World* world) {
    if (!entity || !world) return false;

    // Check if currently colliding
    if (!entity_check_collision(world, entity->position, entity->bbox_min, entity->bbox_max)) {
        return false;  // Not stuck
    }

    // Try to push out in each direction, find the smallest push
    float push_amount = 0.05f;
    int max_attempts = 20;  // Max 1 block push

    // Try pushing up first (most common case - stuck in ground)
    for (int i = 0; i < max_attempts; i++) {
        Vector3 test_pos = entity->position;
        test_pos.y += push_amount * (i + 1);

        if (!entity_check_collision(world, test_pos, entity->bbox_min, entity->bbox_max)) {
            entity->position = test_pos;
            entity->velocity.y = 0;
            return true;
        }
    }

    // Try pushing in X direction
    for (int dir = -1; dir <= 1; dir += 2) {
        for (int i = 0; i < max_attempts; i++) {
            Vector3 test_pos = entity->position;
            test_pos.x += push_amount * (i + 1) * dir;

            if (!entity_check_collision(world, test_pos, entity->bbox_min, entity->bbox_max)) {
                entity->position = test_pos;
                entity->velocity.x = 0;
                return true;
            }
        }
    }

    // Try pushing in Z direction
    for (int dir = -1; dir <= 1; dir += 2) {
        for (int i = 0; i < max_attempts; i++) {
            Vector3 test_pos = entity->position;
            test_pos.z += push_amount * (i + 1) * dir;

            if (!entity_check_collision(world, test_pos, entity->bbox_min, entity->bbox_max)) {
                entity->position = test_pos;
                entity->velocity.z = 0;
                return true;
            }
        }
    }

    return false;  // Couldn't push out
}

// ============================================================================
// MOVEMENT WITH COLLISION
// ============================================================================

/**
 * Move an entity with per-axis collision detection
 *
 * Returns collision flags: bit 0 = X, bit 1 = Z, bit 2 = Y
 */
int entity_move_with_collision(Entity* entity, struct World* world, float dt) {
    if (!entity || !world) return 0;

    // First, push entity out if stuck in a block
    entity_push_out_of_blocks(entity, world);

    int collision_flags = 0;
    Vector3 new_pos = entity->position;

    // ========================================================================
    // X AXIS
    // ========================================================================
    if (entity->velocity.x != 0) {
        new_pos.x += entity->velocity.x * dt;

        if (entity_check_collision(world, new_pos, entity->bbox_min, entity->bbox_max)) {
            new_pos.x = entity->position.x;
            entity->velocity.x = 0;
            collision_flags |= 0x01;  // X collision
        }
    }

    // ========================================================================
    // Z AXIS
    // ========================================================================
    if (entity->velocity.z != 0) {
        new_pos.z += entity->velocity.z * dt;

        if (entity_check_collision(world, new_pos, entity->bbox_min, entity->bbox_max)) {
            new_pos.z = entity->position.z;
            entity->velocity.z = 0;
            collision_flags |= 0x02;  // Z collision
        }
    }

    // ========================================================================
    // Y AXIS
    // ========================================================================
    if (entity->velocity.y != 0) {
        new_pos.y += entity->velocity.y * dt;

        if (entity_check_collision(world, new_pos, entity->bbox_min, entity->bbox_max)) {
            new_pos.y = entity->position.y;
            entity->velocity.y = 0;
            collision_flags |= 0x04;  // Y collision
        }
    }

    // Update final position
    entity->position = new_pos;

    return collision_flags;
}

// ============================================================================
// GROUND DETECTION
// ============================================================================

/**
 * Check if an entity is standing on solid ground
 *
 * Checks all 4 bottom corners slightly below the entity
 */
bool entity_is_on_ground(Entity* entity, struct World* world) {
    if (!entity || !world) return false;

    // Check slightly below the entity's bottom
    float check_y = entity->position.y + entity->bbox_min.y - 0.1f;

    // Calculate corner positions
    float x_min = entity->position.x + entity->bbox_min.x;
    float x_max = entity->position.x + entity->bbox_max.x;
    float z_min = entity->position.z + entity->bbox_min.z;
    float z_max = entity->position.z + entity->bbox_max.z;

    // Check all 4 corners
    return entity_is_solid_at(world, x_min, check_y, z_min) ||
           entity_is_solid_at(world, x_max, check_y, z_min) ||
           entity_is_solid_at(world, x_min, check_y, z_max) ||
           entity_is_solid_at(world, x_max, check_y, z_max);
}

// ============================================================================
// GRAVITY
// ============================================================================

/**
 * Apply gravity to an entity with ground clamping
 */
void entity_apply_gravity(Entity* entity, struct World* world, float dt, float gravity) {
    if (!entity || !world) return;

    // Apply gravity acceleration
    entity->velocity.y -= gravity * dt;

    // Clamp terminal velocity (prevent falling too fast)
    if (entity->velocity.y < -50.0f) {
        entity->velocity.y = -50.0f;
    }
}

// ============================================================================
// JUMP OBSTACLE DETECTION
// ============================================================================

/**
 * Check if an entity can jump over an obstacle ahead
 *
 * Checks for a jumpable 1-block obstacle:
 * - Must be on ground
 * - Blocked at feet level ahead
 * - Clear at jump height (1.3 blocks up)
 * - Has a landing surface
 */
bool entity_can_jump_obstacle(Entity* entity, struct World* world, Vector3 direction) {
    if (!entity || !world) return false;

    // Must be on ground to jump
    if (!entity_is_on_ground(entity, world)) return false;

    // Check position ahead (0.6 blocks in movement direction)
    float look_ahead = 0.6f;
    float ahead_x = entity->position.x + direction.x * look_ahead;
    float ahead_z = entity->position.z + direction.z * look_ahead;
    float base_y = entity->position.y;

    // Check if blocked at feet level (0.3 blocks up)
    bool blocked_low = entity_is_solid_at(world, ahead_x, base_y + 0.3f, ahead_z);

    // Check if clear at jump height (1.3 blocks up - enough clearance for jump arc)
    bool clear_high = !entity_is_solid_at(world, ahead_x, base_y + 1.3f, ahead_z);

    // Check if there's a landing surface on top of the obstacle
    // Look for solid block at obstacle top level
    bool has_landing = entity_is_solid_at(world, ahead_x, base_y + 0.5f, ahead_z);

    // Also check there's headroom at the landing position
    bool landing_clear = !entity_is_solid_at(world, ahead_x, base_y + 2.0f, ahead_z);

    return blocked_low && clear_high && has_landing && landing_clear;
}
