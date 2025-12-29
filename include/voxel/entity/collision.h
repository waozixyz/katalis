/**
 * Entity Collision System
 *
 * Provides AABB-based collision detection for entities against the voxel world.
 * Uses multi-point sampling (12 points) for accurate collision detection.
 */

#ifndef ENTITY_COLLISION_H
#define ENTITY_COLLISION_H

#include "voxel/entity/entity.h"
#include <stdbool.h>
#include <raylib.h>

// Forward declaration
struct World;

// ============================================================================
// COLLISION DETECTION
// ============================================================================

/**
 * Check if an AABB at the given position collides with any solid blocks
 *
 * Uses 12-point sampling: 4 corners at 3 height levels (bottom, middle, top)
 *
 * @param world The world to check collision against
 * @param pos Position of the entity (at feet)
 * @param bbox_min Bounding box minimum (relative to pos)
 * @param bbox_max Bounding box maximum (relative to pos)
 * @return true if collision detected, false otherwise
 */
bool entity_check_collision(struct World* world, Vector3 pos, Vector3 bbox_min, Vector3 bbox_max);

/**
 * Check if a point is inside a solid block
 *
 * @param world The world to check
 * @param x World X coordinate
 * @param y World Y coordinate
 * @param z World Z coordinate
 * @return true if the point is inside a solid block
 */
bool entity_is_solid_at(struct World* world, float x, float y, float z);

// ============================================================================
// MOVEMENT WITH COLLISION
// ============================================================================

/**
 * Move an entity with per-axis collision detection
 *
 * Tests X, Z, then Y axes independently to allow wall sliding.
 * Updates entity->position and zeroes velocity on collision axes.
 *
 * @param entity The entity to move
 * @param world The world for collision detection
 * @param dt Delta time
 * @return Collision flags: bit 0 = X collision, bit 1 = Z collision, bit 2 = Y collision
 */
int entity_move_with_collision(Entity* entity, struct World* world, float dt);

/**
 * Check if any horizontal (X or Z) collision occurred
 * @param collision_flags Return value from entity_move_with_collision
 */
#define COLLISION_HIT_WALL(flags) (((flags) & 0x03) != 0)

/**
 * Check if vertical (Y) collision occurred
 * @param collision_flags Return value from entity_move_with_collision
 */
#define COLLISION_HIT_VERTICAL(flags) (((flags) & 0x04) != 0)

// ============================================================================
// GROUND DETECTION
// ============================================================================

/**
 * Check if an entity is standing on solid ground
 *
 * Checks all 4 bottom corners slightly below the entity
 *
 * @param entity The entity to check
 * @param world The world to check against
 * @return true if entity is on solid ground
 */
bool entity_is_on_ground(Entity* entity, struct World* world);

// ============================================================================
// GRAVITY
// ============================================================================

/**
 * Apply gravity to an entity with ground clamping
 *
 * @param entity The entity to apply gravity to
 * @param world The world for ground detection
 * @param dt Delta time
 * @param gravity Gravity acceleration (positive = downward, typically 20.0)
 */
void entity_apply_gravity(Entity* entity, struct World* world, float dt, float gravity);

// ============================================================================
// JUMP OBSTACLE DETECTION
// ============================================================================

/**
 * Check if an entity can jump over an obstacle ahead
 *
 * Checks if:
 * - Entity is on ground
 * - There's a 1-block obstacle ahead (blocked at feet, clear at head height)
 * - There's a landing surface on top of the obstacle
 *
 * @param entity The entity to check
 * @param world The world to check against
 * @param direction Movement direction (should be normalized XZ)
 * @return true if entity can jump the obstacle
 */
bool entity_can_jump_obstacle(Entity* entity, struct World* world, Vector3 direction);

#endif // ENTITY_COLLISION_H
