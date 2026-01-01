/**
 * Raycast Module
 *
 * Block and entity raycasting for player interaction
 */

#ifndef RAYCAST_H
#define RAYCAST_H

#include <raylib.h>
#include <stdbool.h>
#include "voxel/core/texture_atlas.h"  // For BlockFace enum

// Forward declarations
typedef struct World World;
typedef struct Entity Entity;
typedef struct EntityManager EntityManager;

/**
 * Raycast to find the block the player is looking at
 *
 * Uses DDA algorithm for voxel traversal
 *
 * @param world World to raycast in
 * @param origin Ray starting position
 * @param direction Ray direction (will be normalized)
 * @param max_distance Maximum ray distance
 * @param hit_block Output: position of hit block
 * @param hit_face Output: which face was hit
 * @return true if a solid block was hit
 */
bool raycast_block(World* world, Vector3 origin, Vector3 direction,
                   float max_distance, Vector3* hit_block, BlockFace* hit_face);

/**
 * Raycast against all entities to find the closest hit
 *
 * Uses ray-AABB intersection testing
 *
 * @param manager Entity manager containing entities
 * @param origin Ray starting position
 * @param direction Ray direction
 * @param max_distance Maximum ray distance
 * @return Closest entity hit, or NULL if none
 */
Entity* raycast_entity(EntityManager* manager, Vector3 origin, Vector3 direction,
                       float max_distance);

#endif // RAYCAST_H
