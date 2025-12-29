/**
 * Player Controller - First-Person Movement
 *
 * Handles camera, movement, and physics
 */

#ifndef VOXEL_PLAYER_H
#define VOXEL_PLAYER_H

#include <raylib.h>
#include <stdbool.h>
#include "world.h"  // Need World type

// ============================================================================
// PLAYER STATE
// ============================================================================

typedef struct {
    // Position and camera
    Vector3 position;           // Player position in world
    Camera3D camera;            // First-person camera

    // Rotation (in radians)
    float yaw;                  // Horizontal rotation
    float pitch;                // Vertical rotation (up/down look)

    // Movement
    Vector3 velocity;           // Current velocity
    bool is_flying;             // Flying mode toggle
    bool is_grounded;           // Standing on ground

    // Control sensitivity
    float mouse_sensitivity;    // Mouse look sensitivity
    float move_speed;           // Movement speed
    float fly_speed;            // Flying speed
    float sprint_multiplier;    // Sprint speed multiplier

} Player;

// ============================================================================
// API
// ============================================================================

/**
 * Create and initialize player at position
 */
Player* player_create(Vector3 start_position);

/**
 * Destroy player
 */
void player_destroy(Player* player);

/**
 * Update player - call every frame with delta time
 * Handles input, movement, collision, and camera updates
 */
void player_update(Player* player, World* world, float dt);

/**
 * Set player position
 */
void player_set_position(Player* player, Vector3 position);

/**
 * Toggle flying mode
 */
void player_toggle_flying(Player* player);

/**
 * Get player's camera for rendering
 */
Camera3D player_get_camera(Player* player);

#endif // VOXEL_PLAYER_H
