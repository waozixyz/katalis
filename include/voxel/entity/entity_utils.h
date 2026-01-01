/**
 * Entity Utilities
 *
 * Shared helper functions for entity implementations
 */

#ifndef ENTITY_UTILS_H
#define ENTITY_UTILS_H

#include <raylib.h>
#include <raymath.h>

// ============================================================================
// ANIMATION CONSTANTS
// ============================================================================

#define ENTITY_ANIMATION_DAMPING 0.85f   // Limb swing damping factor per frame
#define ENTITY_WALK_ANIM_SPEED 0.8f      // Normal walk animation speed
#define ENTITY_FLEE_ANIM_SPEED 1.5f      // Fast animation when fleeing

// ============================================================================
// FUNCTIONS
// ============================================================================

/**
 * Get a random float between min and max
 */
float entity_random_range(float min, float max);

/**
 * Pick a random horizontal direction (unit vector on XZ plane)
 */
Vector3 entity_random_direction(void);

/**
 * Apply ambient lighting to a color
 */
Color entity_apply_ambient(Color base, Vector3 ambient);

#endif // ENTITY_UTILS_H
