/**
 * Particle System Implementation
 */

#include "voxel/render/particle.h"
#include "voxel/core/texture_atlas.h"
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global particle system
static ParticleSystem g_particles;

// Gravity constant
#define PARTICLE_GRAVITY 15.0f

/**
 * Find an inactive particle slot
 */
static Particle* find_inactive_particle(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!g_particles.particles[i].active) {
            return &g_particles.particles[i];
        }
    }
    return NULL;  // All particles in use
}

void particle_system_init(void) {
    memset(&g_particles, 0, sizeof(ParticleSystem));

    // Load particle shader
    g_particles.shader = LoadShader("shaders/particle.vs", "shaders/particle.fs");

    if (g_particles.shader.id == 0) {
        printf("[PARTICLE] Warning: Failed to load particle shaders, using default\n");
        g_particles.shader = LoadShaderFromMemory(NULL, NULL);
    }

    g_particles.initialized = true;
    printf("[PARTICLE] System initialized (max %d particles)\n", MAX_PARTICLES);
}

void particle_system_destroy(void) {
    if (!g_particles.initialized) return;

    UnloadShader(g_particles.shader);
    memset(&g_particles, 0, sizeof(ParticleSystem));

    printf("[PARTICLE] System destroyed\n");
}

void particle_system_update(float dt) {
    if (!g_particles.initialized) return;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle* p = &g_particles.particles[i];
        if (!p->active) continue;

        // Apply gravity
        p->velocity.y -= PARTICLE_GRAVITY * dt;

        // Apply velocity damping (air resistance)
        float damping = 0.98f;
        p->velocity.x *= damping;
        p->velocity.z *= damping;

        // Update position
        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->position.z += p->velocity.z * dt;

        // Update lifetime
        p->life -= dt;

        // Deactivate expired or underground particles
        if (p->life <= 0.0f || p->position.y < 0.0f) {
            p->active = false;
            g_particles.active_count--;
        }
    }
}

void particle_system_render(Camera3D camera) {
    if (!g_particles.initialized || g_particles.active_count == 0) return;

    // Get camera vectors for billboarding
    Vector3 camera_forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 camera_right = Vector3Normalize(Vector3CrossProduct(camera_forward, camera.up));
    Vector3 camera_up = Vector3CrossProduct(camera_right, camera_forward);

    // Get texture atlas
    Texture2D atlas = texture_atlas_get_texture();

    // Enable blending for transparent particles
    rlSetBlendMode(RL_BLEND_ALPHA);
    rlSetTexture(atlas.id);

    rlBegin(RL_QUADS);

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle* p = &g_particles.particles[i];
        if (!p->active) continue;

        // Calculate alpha based on remaining life
        float alpha = p->life / p->max_life;
        alpha = alpha * alpha;  // Ease out

        // Scale size slightly as particle ages
        float size = p->size * (0.5f + 0.5f * alpha);
        float half_size = size * 0.5f;

        // Calculate billboard corners
        Vector3 right = Vector3Scale(camera_right, half_size);
        Vector3 up = Vector3Scale(camera_up, half_size);

        Vector3 bl = Vector3Subtract(Vector3Subtract(p->position, right), up);
        Vector3 br = Vector3Subtract(Vector3Add(p->position, right), up);
        Vector3 tr = Vector3Add(Vector3Add(p->position, right), up);
        Vector3 tl = Vector3Add(Vector3Subtract(p->position, right), up);

        // Set color with alpha fade
        unsigned char a = (unsigned char)(alpha * 255.0f);
        rlColor4ub(p->color.r, p->color.g, p->color.b, a);

        // Draw quad (counter-clockwise)
        rlTexCoord2f(p->u_min, p->v_max); rlVertex3f(bl.x, bl.y, bl.z);
        rlTexCoord2f(p->u_max, p->v_max); rlVertex3f(br.x, br.y, br.z);
        rlTexCoord2f(p->u_max, p->v_min); rlVertex3f(tr.x, tr.y, tr.z);
        rlTexCoord2f(p->u_min, p->v_min); rlVertex3f(tl.x, tl.y, tl.z);
    }

    rlEnd();
    rlSetTexture(0);
}

void particle_spawn_block_break(Vector3 position, BlockType block_type, int count) {
    if (!g_particles.initialized) return;

    // Get texture coordinates for this block type
    TextureCoords tex = texture_atlas_get_coords(block_type, FACE_TOP);

    // Use a smaller portion of the texture for particles (center region)
    float tex_margin = 0.25f;
    float u_center = (tex.u_min + tex.u_max) * 0.5f;
    float v_center = (tex.v_min + tex.v_max) * 0.5f;
    float u_size = (tex.u_max - tex.u_min) * tex_margin;
    float v_size = (tex.v_max - tex.v_min) * tex_margin;

    for (int i = 0; i < count && g_particles.active_count < MAX_PARTICLES; i++) {
        Particle* p = find_inactive_particle();
        if (!p) break;

        p->active = true;
        p->type = PARTICLE_TYPE_BLOCK_BREAK;

        // Random position within the block
        p->position.x = position.x + 0.5f + ((float)(rand() % 100) / 100.0f - 0.5f) * 0.8f;
        p->position.y = position.y + 0.5f + ((float)(rand() % 100) / 100.0f - 0.5f) * 0.8f;
        p->position.z = position.z + 0.5f + ((float)(rand() % 100) / 100.0f - 0.5f) * 0.8f;

        // Random velocity - mostly upward and outward
        p->velocity.x = ((float)(rand() % 100) / 100.0f - 0.5f) * 4.0f;
        p->velocity.y = ((float)(rand() % 100) / 100.0f) * 5.0f + 2.0f;
        p->velocity.z = ((float)(rand() % 100) / 100.0f - 0.5f) * 4.0f;

        // Particle properties
        p->size = 0.15f + ((float)(rand() % 100) / 1000.0f);
        p->life = 0.5f + ((float)(rand() % 100) / 200.0f);
        p->max_life = p->life;
        p->color = WHITE;

        // Texture coordinates (small piece of block texture)
        p->u_min = u_center - u_size;
        p->u_max = u_center + u_size;
        p->v_min = v_center - v_size;
        p->v_max = v_center + v_size;

        g_particles.active_count++;
    }
}

void particle_spawn_water_splash(Vector3 position, int count, bool upward) {
    if (!g_particles.initialized) return;

    // Get water texture for splash particles
    TextureCoords tex = texture_atlas_get_coords(BLOCK_WATER, FACE_TOP);

    for (int i = 0; i < count && g_particles.active_count < MAX_PARTICLES; i++) {
        Particle* p = find_inactive_particle();
        if (!p) break;

        p->active = true;
        p->type = PARTICLE_TYPE_WATER_SPLASH;

        // Position at water surface
        p->position.x = position.x + ((float)(rand() % 100) / 100.0f - 0.5f) * 0.5f;
        p->position.y = position.y;
        p->position.z = position.z + ((float)(rand() % 100) / 100.0f - 0.5f) * 0.5f;

        // Velocity depends on direction
        float speed = 3.0f + ((float)(rand() % 100) / 100.0f) * 2.0f;
        float angle = ((float)(rand() % 360)) * DEG2RAD;

        p->velocity.x = cosf(angle) * speed * 0.5f;
        p->velocity.z = sinf(angle) * speed * 0.5f;

        if (upward) {
            // Exiting water - spray upward
            p->velocity.y = speed;
        } else {
            // Entering water - smaller splash
            p->velocity.y = speed * 0.3f;
        }

        // Particle properties
        p->size = 0.1f + ((float)(rand() % 50) / 500.0f);
        p->life = 0.3f + ((float)(rand() % 100) / 300.0f);
        p->max_life = p->life;
        p->color = (Color){100, 150, 255, 200};  // Light blue

        // Texture coordinates
        p->u_min = tex.u_min;
        p->u_max = tex.u_max;
        p->v_min = tex.v_min;
        p->v_max = tex.v_max;

        g_particles.active_count++;
    }
}

int particle_get_active_count(void) {
    return g_particles.active_count;
}
