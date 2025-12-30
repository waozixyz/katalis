/**
 * Player Controller - First-Person Movement
 *
 * Handles camera, movement, and physics
 */

#ifndef VOXEL_PLAYER_H
#define VOXEL_PLAYER_H

#include <raylib.h>
#include <stdbool.h>
#include "voxel/world/world.h"  // Need World type

// Forward declarations
typedef struct Inventory Inventory;

// ============================================================================
// VIEW MODES
// ============================================================================

typedef enum {
    VIEW_MODE_FIRST_PERSON,
    VIEW_MODE_THIRD_PERSON,
    VIEW_MODE_THIRD_PERSON_FRONT  // Camera in front, facing player
} ViewMode;

// ============================================================================
// PLAYER STATE
// ============================================================================

typedef struct Player {
    // Position and camera
    Vector3 position;           // Player position in world
    Camera3D camera;            // First-person camera

    // Rotation (in radians)
    float yaw;                  // Horizontal rotation
    float pitch;                // Vertical rotation (up/down look)

    // View mode
    ViewMode view_mode;         // Current camera view mode
    float third_person_distance; // Distance of camera in third-person mode

    // Movement
    Vector3 velocity;           // Current velocity
    bool is_flying;             // Flying mode toggle
    bool is_grounded;           // Standing on ground

    // Control sensitivity
    float mouse_sensitivity;    // Mouse look sensitivity
    float move_speed;           // Movement speed
    float fly_speed;            // Flying speed
    float sprint_multiplier;    // Sprint speed multiplier

    // Inventory
    Inventory* inventory;       // Player inventory (hotbar + main + crafting)

    // Animation state
    float walk_animation_time;  // Accumulated time for walk cycle
    float arm_swing_angle;      // Current arm rotation angle (degrees)
    float leg_swing_angle;      // Current leg rotation angle (degrees)

    // First-person swing animation (for hitting/mining)
    float swing_time;           // Current swing progress (0 = idle, >0 = swinging)
    float swing_duration;       // Total swing time (default 0.25 seconds)
    bool is_swinging;           // Currently in swing animation

    // Lighting
    Vector3 ambient_light;      // Current ambient light color (for model rendering)

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
 * Update player physics only (gravity, collisions) - no input handling
 * Use when game is paused but world should continue (like Minecraft menu)
 */
void player_update_physics(Player* player, World* world, float dt);

/**
 * Set player position
 */
void player_set_position(Player* player, Vector3 position);

/**
 * Toggle flying mode
 */
void player_toggle_flying(Player* player);

/**
 * Toggle view mode between first-person and third-person
 */
void player_toggle_view_mode(Player* player);

/**
 * Get player's camera for rendering
 */
Camera3D player_get_camera(Player* player);

/**
 * Check if a block position would collide with the player
 * Used for preventing block placement inside player
 */
bool player_collides_with_position(Player* player, Vector3 block_pos);

/**
 * Render the player model (for third-person view)
 */
void player_render_model(Player* player);

/**
 * Start a swing animation (for hitting/mining)
 */
void player_start_swing(Player* player);

/**
 * Update swing animation - call every frame
 * Returns current swing angle in degrees
 */
float player_update_swing(Player* player, float dt);

/**
 * Get current swing angle for rendering
 */
float player_get_swing_angle(Player* player);

#endif // VOXEL_PLAYER_H
