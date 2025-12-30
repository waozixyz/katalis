/**
 * Particle System
 *
 * Pool-based particle system for visual effects like block breaking,
 * water splashes, and other effects.
 */

#ifndef VOXEL_RENDER_PARTICLE_H
#define VOXEL_RENDER_PARTICLE_H

#include <raylib.h>
#include <stdbool.h>
#include "voxel/core/block.h"

// Maximum number of particles in the system
#define MAX_PARTICLES 2048

// Particle types
typedef enum {
    PARTICLE_TYPE_BLOCK_BREAK,   // Debris when mining blocks
    PARTICLE_TYPE_WATER_SPLASH,  // Water entry/exit splash
} ParticleType;

// Single particle data
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float size;
    float life;           // Remaining lifetime (seconds)
    float max_life;       // Initial lifetime for fade calculation
    ParticleType type;
    bool active;
    // Texture coordinates for atlas-based particles
    float u_min, v_min, u_max, v_max;
} Particle;

// Particle system state
typedef struct {
    Particle particles[MAX_PARTICLES];
    int active_count;
    Shader shader;
    bool initialized;
} ParticleSystem;

/**
 * Initialize the particle system
 * Must be called after texture atlas is initialized
 */
void particle_system_init(void);

/**
 * Destroy the particle system and free resources
 */
void particle_system_destroy(void);

/**
 * Update all active particles
 * @param dt Delta time in seconds
 */
void particle_system_update(float dt);

/**
 * Render all active particles
 * @param camera Current camera for billboard rendering
 */
void particle_system_render(Camera3D camera);

/**
 * Spawn block break particles at a position
 * @param position World position of broken block
 * @param block_type Type of block that was broken
 * @param count Number of particles to spawn (typically 8-16)
 */
void particle_spawn_block_break(Vector3 position, BlockType block_type, int count);

/**
 * Spawn water splash particles
 * @param position World position of splash
 * @param count Number of particles to spawn
 * @param upward If true, particles go up (exiting water), otherwise down (entering)
 */
void particle_spawn_water_splash(Vector3 position, int count, bool upward);

/**
 * Get the number of currently active particles
 */
int particle_get_active_count(void);

#endif // VOXEL_RENDER_PARTICLE_H
