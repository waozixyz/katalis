/**
 * Raycast Module Implementation
 *
 * Block and entity raycasting for player interaction
 */

#include "voxel/world/raycast.h"
#include "voxel/world/world.h"
#include "voxel/core/block.h"
#include "voxel/entity/entity.h"
#include <raymath.h>
#include <math.h>
#include <float.h>
#include <stddef.h>  // For NULL

/**
 * Raycast to find the block the player is looking at
 * Uses DDA (Digital Differential Analyzer) algorithm for voxel traversal
 */
bool raycast_block(World* world, Vector3 origin, Vector3 direction,
                   float max_distance, Vector3* hit_block, BlockFace* hit_face) {
    // Normalize direction
    direction = Vector3Normalize(direction);

    // Current voxel position
    int x = (int)floorf(origin.x);
    int y = (int)floorf(origin.y);
    int z = (int)floorf(origin.z);

    // Direction to step in each axis (-1, 0, or 1)
    int step_x = (direction.x > 0) ? 1 : -1;
    int step_y = (direction.y > 0) ? 1 : -1;
    int step_z = (direction.z > 0) ? 1 : -1;

    // Distance to next voxel boundary in each axis
    float t_delta_x = (direction.x != 0) ? fabsf(1.0f / direction.x) : FLT_MAX;
    float t_delta_y = (direction.y != 0) ? fabsf(1.0f / direction.y) : FLT_MAX;
    float t_delta_z = (direction.z != 0) ? fabsf(1.0f / direction.z) : FLT_MAX;

    // Calculate initial t_max values
    float t_max_x, t_max_y, t_max_z;

    if (direction.x > 0) {
        t_max_x = ((float)(x + 1) - origin.x) / direction.x;
    } else if (direction.x < 0) {
        t_max_x = (origin.x - (float)x) / -direction.x;
    } else {
        t_max_x = FLT_MAX;
    }

    if (direction.y > 0) {
        t_max_y = ((float)(y + 1) - origin.y) / direction.y;
    } else if (direction.y < 0) {
        t_max_y = (origin.y - (float)y) / -direction.y;
    } else {
        t_max_y = FLT_MAX;
    }

    if (direction.z > 0) {
        t_max_z = ((float)(z + 1) - origin.z) / direction.z;
    } else if (direction.z < 0) {
        t_max_z = (origin.z - (float)z) / -direction.z;
    } else {
        t_max_z = FLT_MAX;
    }

    // DDA traversal
    float t = 0.0f;
    BlockFace face = FACE_TOP;  // Default

    while (t < max_distance) {
        // Check current block
        Block block = world_get_block(world, x, y, z);
        if (block_is_solid(block)) {
            hit_block->x = (float)x;
            hit_block->y = (float)y;
            hit_block->z = (float)z;
            if (hit_face) *hit_face = face;
            return true;
        }

        // Step to next voxel and track which face we crossed
        if (t_max_x < t_max_y && t_max_x < t_max_z) {
            x += step_x;
            t = t_max_x;
            t_max_x += t_delta_x;
            face = (step_x > 0) ? FACE_LEFT : FACE_RIGHT;  // LEFT=-X, RIGHT=+X
        } else if (t_max_y < t_max_z) {
            y += step_y;
            t = t_max_y;
            t_max_y += t_delta_y;
            face = (step_y > 0) ? FACE_BOTTOM : FACE_TOP;
        } else {
            z += step_z;
            t = t_max_z;
            t_max_z += t_delta_z;
            face = (step_z > 0) ? FACE_BACK : FACE_FRONT;  // BACK=-Z, FRONT=+Z
        }
    }

    return false;
}

/**
 * Ray-AABB intersection test
 * Returns true if ray hits box, outputs intersection distance to t_out
 */
static bool ray_intersects_aabb(Vector3 origin, Vector3 dir, Vector3 box_min,
                                Vector3 box_max, float* t_out) {
    float tmin = 0.0f, tmax = FLT_MAX;

    // For each axis
    float* o = (float*)&origin;
    float* d = (float*)&dir;
    float* bmin = (float*)&box_min;
    float* bmax = (float*)&box_max;

    for (int i = 0; i < 3; i++) {
        if (fabsf(d[i]) < 0.0001f) {
            // Ray parallel to this axis
            if (o[i] < bmin[i] || o[i] > bmax[i]) return false;
        } else {
            float t1 = (bmin[i] - o[i]) / d[i];
            float t2 = (bmax[i] - o[i]) / d[i];

            if (t1 > t2) {
                float tmp = t1;
                t1 = t2;
                t2 = tmp;
            }

            tmin = fmaxf(tmin, t1);
            tmax = fminf(tmax, t2);

            if (tmin > tmax) return false;
        }
    }

    *t_out = tmin;
    return true;
}

/**
 * Raycast against all entities to find the closest hit
 */
Entity* raycast_entity(EntityManager* manager, Vector3 origin, Vector3 dir,
                       float max_dist) {
    if (!manager) return NULL;

    Entity* closest = NULL;
    float closest_t = max_dist;

    Entity* e = manager->entities;
    while (e) {
        if (e->active) {
            // Calculate world-space bounding box
            Vector3 box_min = Vector3Add(e->position, e->bbox_min);
            Vector3 box_max = Vector3Add(e->position, e->bbox_max);

            float t;
            if (ray_intersects_aabb(origin, dir, box_min, box_max, &t) &&
                t < closest_t && t >= 0) {
                closest_t = t;
                closest = e;
            }
        }
        e = e->next;
    }

    return closest;
}
