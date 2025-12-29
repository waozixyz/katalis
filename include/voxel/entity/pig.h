/**
 * Pig Entity - Passive mob that wanders and flees from players
 *
 * A blocky Minecraft-style pig with:
 * - Pink body (cube)
 * - Head with snout and eyes
 * - Small ears
 * - 4 animated legs
 * - Curly tail
 * - Wandering + flee AI behavior
 */

#ifndef PIG_H
#define PIG_H

#include "voxel/entity/entity.h"
#include <raylib.h>

// ============================================================================
// BODY PROPORTIONS (Minecraft-style blocky pig)
// ============================================================================

// Body (wide, short - stocky build)
#define PIG_BODY_WIDTH 0.5f
#define PIG_BODY_HEIGHT 0.4f
#define PIG_BODY_LENGTH 0.7f

// Head (boxier than sheep)
#define PIG_HEAD_WIDTH 0.35f
#define PIG_HEAD_HEIGHT 0.3f
#define PIG_HEAD_LENGTH 0.3f

// Snout
#define PIG_SNOUT_WIDTH 0.15f
#define PIG_SNOUT_HEIGHT 0.1f
#define PIG_SNOUT_LENGTH 0.1f

// Ears
#define PIG_EAR_SIZE 0.08f

// Legs (shorter than sheep)
#define PIG_LEG_WIDTH 0.1f
#define PIG_LEG_LENGTH 0.25f

// Total height calculation
#define PIG_TOTAL_HEIGHT (PIG_LEG_LENGTH + PIG_BODY_HEIGHT)

// ============================================================================
// AI CONSTANTS
// ============================================================================

#define PIG_WANDER_SPEED 1.2f       // Slower than sheep
#define PIG_FLEE_SPEED 3.5f         // Slower flee
#define PIG_FLEE_DISTANCE 4.0f      // Start fleeing when player within this range
#define PIG_SAFE_DISTANCE 8.0f      // Stop fleeing when player beyond this

#define PIG_WANDER_TIME_MIN 2.0f    // Minimum time before direction change
#define PIG_WANDER_TIME_MAX 6.0f    // Maximum time before direction change
#define PIG_IDLE_CHANCE 0.4f        // 40% chance to stand idle
#define PIG_IDLE_TIME 2.5f          // Time spent idle

// ============================================================================
// PIG DATA
// ============================================================================

typedef struct {
    // Appearance
    Color body_color;               // Pink body color
    Color snout_color;              // Lighter pink snout

    // Animation
    float walk_animation_time;      // Accumulated time for walk cycle
    float leg_swing_angle;          // Current leg rotation angle
    float tail_wiggle_angle;        // Tail wiggle animation

    // AI state
    float wander_timer;             // Time until next direction change
    float idle_timer;               // Time remaining in idle state
    bool is_idle;                   // Currently standing still
    bool is_fleeing;                // Currently fleeing from player
    Vector3 wander_direction;       // Current movement direction (normalized)

    // Lighting (updated each frame)
    Vector3 ambient_light;          // Current ambient light color (0-1)

    // Health & damage
    int hp;                         // Current health (default: 5, dies at 0)
    float damage_flash_timer;       // Red flash duration (0 = no flash)
} PigData;

// ============================================================================
// PIG API
// ============================================================================

/**
 * Spawn a new pig entity
 * @param manager Entity manager to add to
 * @param position World position (at feet)
 * @return Pointer to spawned entity
 */
Entity* pig_spawn(EntityManager* manager, Vector3 position);

/**
 * Damage a pig
 * @param entity The pig entity to damage
 * @param damage Amount of damage to deal
 * @return true if the pig died, false otherwise
 */
bool pig_damage(Entity* entity, int damage);

// ============================================================================
// INTERNAL CALLBACKS (called by entity system)
// ============================================================================

/**
 * Update callback for pig entities
 * Handles AI behavior and animation
 */
void pig_update(Entity* entity, struct World* world, float dt);

/**
 * Render callback for pig entities
 * Draws the blocky pig model
 */
void pig_render(Entity* entity);

/**
 * Destroy callback for pig entities
 * Frees PigData
 */
void pig_destroy(Entity* entity);

#endif // PIG_H
