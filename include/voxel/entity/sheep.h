/**
 * Sheep Entity - Passive mob that wanders and flees from players
 *
 * A blocky Minecraft-style sheep with:
 * - Woolly body (cube)
 * - Head with eyes (cube)
 * - 4 animated legs
 * - Wandering + flee AI behavior
 */

#ifndef SHEEP_H
#define SHEEP_H

#include "voxel/entity/entity.h"
#include <raylib.h>

// ============================================================================
// BODY PROPORTIONS (Minecraft-style blocky sheep)
// ============================================================================

// Body (wide, short, long - horizontal orientation)
#define SHEEP_BODY_WIDTH 0.6f
#define SHEEP_BODY_HEIGHT 0.5f
#define SHEEP_BODY_LENGTH 0.9f

// Head (cube)
#define SHEEP_HEAD_WIDTH 0.35f
#define SHEEP_HEAD_HEIGHT 0.3f
#define SHEEP_HEAD_LENGTH 0.25f

// Legs
#define SHEEP_LEG_WIDTH 0.12f
#define SHEEP_LEG_LENGTH 0.4f

// Total height calculation
#define SHEEP_TOTAL_HEIGHT (SHEEP_LEG_LENGTH + SHEEP_BODY_HEIGHT)

// ============================================================================
// AI CONSTANTS
// ============================================================================

#define SHEEP_WANDER_SPEED 1.5f
#define SHEEP_FLEE_SPEED 4.0f
#define SHEEP_FLEE_DISTANCE 5.0f    // Start fleeing when player within this range
#define SHEEP_SAFE_DISTANCE 10.0f   // Stop fleeing when player beyond this

#define SHEEP_WANDER_TIME_MIN 2.0f  // Minimum time before direction change
#define SHEEP_WANDER_TIME_MAX 5.0f  // Maximum time before direction change
#define SHEEP_GRAZE_CHANCE 0.3f     // 30% chance to graze when picking new direction
#define SHEEP_GRAZE_TIME 2.0f       // Time spent grazing

// ============================================================================
// SHEEP DATA
// ============================================================================

typedef struct {
    // Appearance
    Color wool_color;               // Body/wool color (white, light gray, etc.)
    Color skin_color;               // Head/legs color (pinkish)

    // Animation
    float walk_animation_time;      // Accumulated time for walk cycle
    float leg_swing_angle;          // Current leg rotation angle

    // AI state
    float wander_timer;             // Time until next direction change
    float graze_timer;              // Time remaining in graze state
    bool is_grazing;                // Currently grazing (head down, not moving)
    bool is_fleeing;                // Currently fleeing from player
    Vector3 wander_direction;       // Current movement direction (normalized)

    // Lighting (updated each frame)
    Vector3 ambient_light;          // Current ambient light color (0-1)

    // Health & damage
    int hp;                         // Current health (default: 4, dies at 0)
    float damage_flash_timer;       // Red flash duration (0 = no flash)
} SheepData;

// ============================================================================
// SHEEP API
// ============================================================================

/**
 * Spawn a new sheep entity with default white wool
 * @param manager Entity manager to add to
 * @param position World position (at feet)
 * @return Pointer to spawned entity
 */
Entity* sheep_spawn(EntityManager* manager, Vector3 position);

/**
 * Spawn a sheep with custom wool color
 * @param manager Entity manager to add to
 * @param position World position (at feet)
 * @param wool_color Color for the wool/body
 * @return Pointer to spawned entity
 */
Entity* sheep_spawn_colored(EntityManager* manager, Vector3 position, Color wool_color);

/**
 * Damage a sheep
 * @param entity The sheep entity to damage
 * @param damage Amount of damage to deal
 * @return true if the sheep died, false otherwise
 */
bool sheep_damage(Entity* entity, int damage);

// ============================================================================
// INTERNAL CALLBACKS (called by entity system)
// ============================================================================

/**
 * Update callback for sheep entities
 * Handles AI behavior and animation
 */
void sheep_update(Entity* entity, struct World* world, float dt);

/**
 * Render callback for sheep entities
 * Draws the blocky sheep model
 */
void sheep_render(Entity* entity);

/**
 * Destroy callback for sheep entities
 * Frees SheepData
 */
void sheep_destroy(Entity* entity);

#endif // SHEEP_H
