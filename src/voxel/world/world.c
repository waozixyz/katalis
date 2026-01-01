/**
 * World System Implementation
 */

#include "voxel/world/world.h"
#include "voxel/world/chunk_worker.h"
#include "voxel/world/spawn.h"
#include "voxel/world/water.h"
#include "voxel/world/chest.h"
#include "voxel/core/texture_atlas.h"
#include "voxel/world/terrain.h"
#include "voxel/render/light.h"
#include "voxel/render/chunk_batcher.h"
#include "voxel/entity/entity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

// ============================================================================
// CACHED SHADER UNIFORM LOCATIONS
// ============================================================================

static struct {
    int ambient;
    int camera_pos;
    int fog_start;
    int fog_end;
    int fog_color;
    int underwater;
    int time;
    bool initialized;
} g_shader_locs = {0};

/**
 * Initialize shader uniform location cache (call once after shader is loaded)
 */
static void init_shader_loc_cache(Shader shader) {
    if (g_shader_locs.initialized) return;

    g_shader_locs.ambient = GetShaderLocation(shader, "u_ambient_light");
    g_shader_locs.camera_pos = GetShaderLocation(shader, "u_camera_pos");
    g_shader_locs.fog_start = GetShaderLocation(shader, "u_fog_start");
    g_shader_locs.fog_end = GetShaderLocation(shader, "u_fog_end");
    g_shader_locs.fog_color = GetShaderLocation(shader, "u_fog_color");
    g_shader_locs.underwater = GetShaderLocation(shader, "u_underwater");
    g_shader_locs.time = GetShaderLocation(shader, "u_time");
    g_shader_locs.initialized = true;
}

// Note: Frustum culling functions were moved to chunk_batcher.c
// The batcher now handles batch-level culling

// ============================================================================
// DIRTY CHUNK LIST HELPERS
// ============================================================================

/**
 * Add a chunk to the dirty list if not already in it
 */
static void world_add_to_dirty_list(World* world, Chunk* chunk) {
    if (!world || !chunk || chunk->in_dirty_list) return;

    chunk->dirty_next = world->dirty_head;
    world->dirty_head = chunk;
    chunk->in_dirty_list = true;
    world->dirty_count++;
}

/**
 * Remove a chunk from the dirty list (called after remesh)
 */
static void world_remove_from_dirty_list(World* world, Chunk* chunk) {
    if (!world || !chunk || !chunk->in_dirty_list) return;

    // Find and remove from list
    Chunk** pp = &world->dirty_head;
    while (*pp) {
        if (*pp == chunk) {
            *pp = chunk->dirty_next;
            chunk->dirty_next = NULL;
            chunk->in_dirty_list = false;
            world->dirty_count--;
            return;
        }
        pp = (Chunk**)&((*pp)->dirty_next);
    }
}

// ============================================================================
// HASH MAP HELPERS
// ============================================================================

/**
 * Hash function for chunk coordinates
 */
static uint32_t hash_chunk_coords(int x, int z) {
    // Simple hash combining x and z
    uint32_t h = 2166136261u;
    h = (h ^ (uint32_t)x) * 16777619u;
    h = (h ^ (uint32_t)z) * 16777619u;
    return h % WORLD_MAX_CHUNKS;
}

/**
 * Create chunk hash map
 */
static ChunkHashMap* chunk_hashmap_create(void) {
    ChunkHashMap* map = (ChunkHashMap*)malloc(sizeof(ChunkHashMap));
    memset(map->buckets, 0, sizeof(map->buckets));
    map->chunk_count = 0;
    return map;
}

/**
 * Destroy chunk hash map
 */
static void chunk_hashmap_destroy(ChunkHashMap* map) {
    if (!map) return;

    // Free all chunks and nodes
    for (int i = 0; i < WORLD_MAX_CHUNKS; i++) {
        ChunkNode* node = map->buckets[i];
        while (node) {
            ChunkNode* next = node->next;
            chunk_destroy(node->chunk);
            free(node);
            node = next;
        }
    }

    free(map);
}

/**
 * Insert chunk into hash map
 */
static void chunk_hashmap_insert(ChunkHashMap* map, int chunk_x, int chunk_z, Chunk* chunk) {
    uint32_t hash = hash_chunk_coords(chunk_x, chunk_z);

    ChunkNode* node = (ChunkNode*)malloc(sizeof(ChunkNode));
    node->chunk_x = chunk_x;
    node->chunk_z = chunk_z;
    node->chunk = chunk;
    node->next = map->buckets[hash];
    map->buckets[hash] = node;
    map->chunk_count++;
}

/**
 * Get chunk from hash map
 */
static Chunk* chunk_hashmap_get(ChunkHashMap* map, int chunk_x, int chunk_z) {
    uint32_t hash = hash_chunk_coords(chunk_x, chunk_z);
    ChunkNode* node = map->buckets[hash];

    while (node) {
        if (node->chunk_x == chunk_x && node->chunk_z == chunk_z) {
            return node->chunk;
        }
        node = node->next;
    }

    return NULL;
}

// ============================================================================
// COORDINATE CONVERSION
// ============================================================================

void world_to_chunk_coords(int world_x, int world_z, int* chunk_x, int* chunk_z) {
    *chunk_x = world_x >= 0 ? world_x / CHUNK_SIZE : (world_x - CHUNK_SIZE + 1) / CHUNK_SIZE;
    *chunk_z = world_z >= 0 ? world_z / CHUNK_SIZE : (world_z - CHUNK_SIZE + 1) / CHUNK_SIZE;
}

void world_to_local_coords(int world_x, int world_y, int world_z,
                           int* chunk_x, int* chunk_z,
                           int* local_x, int* local_y, int* local_z) {
    world_to_chunk_coords(world_x, world_z, chunk_x, chunk_z);

    *local_x = world_x - (*chunk_x * CHUNK_SIZE);
    *local_y = world_y;
    *local_z = world_z - (*chunk_z * CHUNK_SIZE);

    // Handle negative coordinates
    if (*local_x < 0) *local_x += CHUNK_SIZE;
    if (*local_z < 0) *local_z += CHUNK_SIZE;
}

// ============================================================================
// WORLD MANAGEMENT
// ============================================================================

World* world_create(TerrainParams terrain_params) {
    World* world = (World*)malloc(sizeof(World));
    world->chunks = chunk_hashmap_create();
    world->worker = chunk_worker_create();
    world->batcher = chunk_batcher_create();
    world->center_chunk_x = 0;
    world->center_chunk_z = 0;
    world->view_distance = WORLD_VIEW_DISTANCE;
    world->terrain_params = terrain_params;
    world->player = NULL;  // Set by game after player creation
    world->entity_manager = NULL;  // Set by game after entity manager creation
    world->time_of_day = 12.0f;  // Default to noon
    world->water_queue = water_queue_create();
    world->game_tick = 0;
    world->chest_registry = chest_registry_create();
    world->dirty_head = NULL;
    world->dirty_count = 0;
    world->batch_rebuilds_per_frame = 16;  // Default from BATCH_REBUILDS_PER_FRAME
    world->max_uploads_per_frame = MAX_UPLOADS_PER_FRAME;

    // Initialize spawn system
    spawn_system_init();

    printf("[WORLD] Created world with view distance %d\n", world->view_distance);
    return world;
}

void world_destroy(World* world) {
    if (!world) return;

    // Stop worker threads first
    if (world->worker) {
        chunk_worker_destroy(world->worker);
    }

    // Destroy batcher before chunks (has references to chunks)
    if (world->batcher) {
        chunk_batcher_destroy(world->batcher);
    }

    // Destroy water system
    if (world->water_queue) {
        water_queue_destroy(world->water_queue);
    }

    // Destroy chest registry
    if (world->chest_registry) {
        chest_registry_destroy(world->chest_registry);
    }

    chunk_hashmap_destroy(world->chunks);
    free(world);

    printf("[WORLD] Destroyed world\n");
}

Chunk* world_get_chunk(World* world, int chunk_x, int chunk_z) {
    if (!world) return NULL;
    return chunk_hashmap_get(world->chunks, chunk_x, chunk_z);
}

Chunk* world_get_or_create_chunk(World* world, int chunk_x, int chunk_z) {
    if (!world) return NULL;

    Chunk* chunk = world_get_chunk(world, chunk_x, chunk_z);
    if (chunk) return chunk;

    // Create new chunk
    chunk = chunk_create(chunk_x, chunk_z);
    chunk_hashmap_insert(world->chunks, chunk_x, chunk_z, chunk);

    return chunk;
}

Block world_get_block(World* world, int x, int y, int z) {
    int chunk_x, chunk_z;
    int local_x, local_y, local_z;
    world_to_local_coords(x, y, z, &chunk_x, &chunk_z, &local_x, &local_y, &local_z);

    Chunk* chunk = world_get_chunk(world, chunk_x, chunk_z);
    if (!chunk) {
        return (Block){BLOCK_AIR, 0, 0};
    }

    return chunk_get_block(chunk, local_x, local_y, local_z);
}

void world_set_block(World* world, int x, int y, int z, Block block) {
    int chunk_x, chunk_z;
    int local_x, local_y, local_z;
    world_to_local_coords(x, y, z, &chunk_x, &chunk_z, &local_x, &local_y, &local_z);

    Chunk* chunk = world_get_or_create_chunk(world, chunk_x, chunk_z);
    chunk_set_block(chunk, local_x, local_y, local_z, block);

    // Add chunk to dirty list for remeshing
    world_add_to_dirty_list(world, chunk);

    // Recalculate lighting for this chunk when a block changes
    light_calculate_chunk(chunk);

    // Notify water system of block change
    if (world->water_queue) {
        water_on_block_change(world->water_queue, world, x, y, z);
    }

    // Invalidate batch containing this chunk
    if (world->batcher) {
        chunk_batcher_invalidate(world->batcher, chunk_x, chunk_z);
    }
}

void world_update(World* world, int center_chunk_x, int center_chunk_z) {
    if (!world) return;

    // Process water flow updates (every 2 frames for performance)
    world->game_tick++;
    if (world->water_queue && (world->game_tick % 2 == 0)) {
        water_process_tick(world->water_queue, world);
    }

    static bool first_update = true;
    static int last_center_x = 0;
    static int last_center_z = 0;

    world->center_chunk_x = center_chunk_x;
    world->center_chunk_z = center_chunk_z;

    if (first_update || last_center_x != center_chunk_x || last_center_z != center_chunk_z) {
        // Center chunk changed
        last_center_x = center_chunk_x;
        last_center_z = center_chunk_z;
    }

    // Poll for completed chunks from worker threads (configurable via settings)
    int max_uploads = world->max_uploads_per_frame > 0 ? world->max_uploads_per_frame : MAX_UPLOADS_PER_FRAME;
    int uploaded = 0;
    for (int i = 0; i < max_uploads; i++) {
        CompletedChunk* completed = chunk_worker_poll_completed(world->worker);
        if (!completed) break;

        // Upload mesh to GPU (must be on main thread)
        chunk_worker_upload_mesh(completed->chunk, &completed->mesh);

        // Register chunk with batcher for batched rendering
        if (world->batcher) {
            chunk_batcher_register_chunk(world->batcher, completed->chunk);
        }

        // Spawn animals for newly completed chunks (biome-aware herds)
        if (world->entity_manager && !completed->chunk->has_spawned) {
            spawn_animals_for_chunk(world, completed->chunk->x, completed->chunk->z, world->terrain_params);
            completed->chunk->has_spawned = true;
        }

        free(completed);
        uploaded++;
    }


    // Load chunks in view distance if not already loaded
    for (int x = -world->view_distance; x <= world->view_distance; x++) {
        for (int z = -world->view_distance; z <= world->view_distance; z++) {
            int cx = center_chunk_x + x;
            int cz = center_chunk_z + z;

            Chunk* chunk = world_get_chunk(world, cx, cz);
            if (!chunk) {
                // Chunk not loaded, create it
                chunk = world_get_or_create_chunk(world, cx, cz);
            }

            // Enqueue for async generation if still empty (handles queue-full retry)
            // Priority is based on distance from center_chunk position
            if (chunk->state == CHUNK_STATE_EMPTY) {
                chunk_worker_enqueue(world->worker, chunk, world->terrain_params,
                                     center_chunk_x, center_chunk_z);
            }
        }
    }

    if (first_update) {
        first_update = false;
    }

    // Process pending mesh regenerations using dirty list (O(dirty) instead of O(all_chunks))
    // This eliminates the lag spike when placing/breaking blocks
    Chunk* chunk = world->dirty_head;
    while (chunk) {
        Chunk* next = (Chunk*)chunk->dirty_next;  // Save next before removing from list
        // Only remesh if chunk is complete and needs it
        if (chunk->state == CHUNK_STATE_COMPLETE && chunk->needs_remesh) {
            chunk_generate_mesh(chunk);
            // Invalidate batch when chunk mesh changes
            if (world->batcher) {
                chunk_batcher_invalidate(world->batcher, chunk->x, chunk->z);
            }
            // Remove from dirty list after successful remesh
            world_remove_from_dirty_list(world, chunk);
        }
        chunk = next;
    }

    // Update batched meshes (rebuild dirty batches)
    if (world->batcher) {
        chunk_batcher_update(world->batcher, world->batch_rebuilds_per_frame);
    }
}

// ============================================================================
// LIGHTING HELPERS
// ============================================================================

/**
 * Smooth ease-in-out function (S-curve) for gradual transitions
 */
static float smooth_step(float t) {
    return t * t * (3.0f - 2.0f * t);
}

/**
 * Calculate ambient brightness from time of day
 * Uses smooth S-curve transitions for gradual lighting changes
 */
static float calculate_ambient_brightness(float time) {
    if (time < 5.0f) return 0.15f;          // Night (0-5am): Darker
    if (time < 8.0f) {                       // Dawn (5-8am): 3 hour smooth transition
        float t = (time - 5.0f) / 3.0f;      // 0.0 to 1.0
        return 0.15f + smooth_step(t) * 0.85f;
    }
    if (time < 17.0f) return 1.0f;           // Day (8am-5pm): Full bright
    if (time < 20.0f) {                      // Dusk (5-8pm): 3 hour smooth transition
        float t = (time - 17.0f) / 3.0f;     // 0.0 to 1.0
        return 1.0f - smooth_step(t) * 0.85f;
    }
    return 0.15f;                            // Night (8pm-12am): Darker
}

/**
 * Get ambient light color with temperature shift
 */
static Vector3 get_ambient_color(float time) {
    float brightness = calculate_ambient_brightness(time);

    // Color temperature shifts (extended ranges for smoother transitions)
    if (time >= 5.0f && time < 9.0f) {
        // Dawn: Warm orange tint
        return (Vector3){brightness * 1.0f, brightness * 0.8f, brightness * 0.6f};
    } else if (time >= 16.0f && time < 20.0f) {
        // Dusk: Warm red/orange tint
        return (Vector3){brightness * 1.0f, brightness * 0.7f, brightness * 0.5f};
    } else if (time < 5.0f || time >= 20.0f) {
        // Night: Cool blue tint
        return (Vector3){brightness * 0.5f, brightness * 0.6f, brightness * 0.8f};
    } else {
        // Day: Neutral white
        return (Vector3){brightness, brightness, brightness};
    }
}

/**
 * Public wrapper for ambient color (for entity rendering)
 */
Vector3 world_get_ambient_color(float time_of_day) {
    return get_ambient_color(time_of_day);
}

// ============================================================================
// ENTITY MANAGER ACCESS
// ============================================================================

EntityManager* world_get_entity_manager(World* world) {
    if (!world) return NULL;
    return world->entity_manager;
}

void world_set_entity_manager(World* world, EntityManager* manager) {
    if (!world) return;
    world->entity_manager = manager;
}

int world_get_view_distance(World* world) {
    if (!world) return WORLD_VIEW_DISTANCE;
    return world->view_distance;
}

void world_set_view_distance(World* world, int distance) {
    if (!world) return;

    // Clamp to reasonable range (2-32 chunks)
    if (distance < 2) distance = 2;
    if (distance > 32) distance = 32;

    if (distance != world->view_distance) {
        world->view_distance = distance;
        printf("[WORLD] View distance set to %d chunks\n", distance);
    }
}

void world_set_batch_rebuilds(World* world, int max_rebuilds) {
    if (!world) return;
    // Clamp to reasonable range (4-64)
    if (max_rebuilds < 4) max_rebuilds = 4;
    if (max_rebuilds > 64) max_rebuilds = 64;
    world->batch_rebuilds_per_frame = max_rebuilds;
}

void world_set_max_uploads(World* world, int max_uploads) {
    if (!world) return;
    // Clamp to reasonable range (8-128)
    if (max_uploads < 8) max_uploads = 8;
    if (max_uploads > 128) max_uploads = 128;
    world->max_uploads_per_frame = max_uploads;
}

// ============================================================================
// RENDERING
// ============================================================================

/**
 * Get fog color that blends with sky at horizon
 */
static Vector3 get_fog_color(float time_of_day) {
    // Fog color is affected by ambient brightness to prevent distant blocks looking brighter
    float brightness = calculate_ambient_brightness(time_of_day);

    // Base fog colors for different times
    Vector3 base_fog;
    if (time_of_day >= 5.0f && time_of_day < 9.0f) {
        // Dawn: warm orange horizon
        base_fog = (Vector3){0.9f, 0.7f, 0.5f};
    } else if (time_of_day >= 16.0f && time_of_day < 20.0f) {
        // Dusk: warm red/orange horizon
        base_fog = (Vector3){0.8f, 0.5f, 0.4f};
    } else if (time_of_day < 5.0f || time_of_day >= 20.0f) {
        // Night: dark blue
        base_fog = (Vector3){0.05f, 0.08f, 0.15f};
    } else {
        // Day: light blue sky
        base_fog = (Vector3){0.6f, 0.75f, 0.95f};
    }

    // Scale fog color by ambient brightness so distant blocks don't appear brighter
    return (Vector3){
        base_fog.x * brightness,
        base_fog.y * brightness,
        base_fog.z * brightness
    };
}

/**
 * Apply common shader uniforms for world rendering
 * Used by both opaque and transparent passes
 */
static void apply_world_shader_uniforms(Material material, World* world,
                                         float time_of_day, Vector3 camera_pos, bool underwater) {
    // Ambient light based on time of day
    Vector3 ambient_light = get_ambient_color(time_of_day);
    SetShaderValue(material.shader, g_shader_locs.ambient, &ambient_light, SHADER_UNIFORM_VEC3);

    // Fog settings - start further out for better visibility
    float fog_start = world->view_distance * CHUNK_SIZE * 0.8f;
    float fog_end = world->view_distance * CHUNK_SIZE * 1.2f;
    Vector3 fog_color = get_fog_color(time_of_day);
    int underwater_flag = underwater ? 1 : 0;

    // Water animation time
    float shader_time = (float)GetTime();
    SetShaderValue(material.shader, g_shader_locs.time, &shader_time, SHADER_UNIFORM_FLOAT);

    SetShaderValue(material.shader, g_shader_locs.camera_pos, &camera_pos, SHADER_UNIFORM_VEC3);
    SetShaderValue(material.shader, g_shader_locs.fog_start, &fog_start, SHADER_UNIFORM_FLOAT);
    SetShaderValue(material.shader, g_shader_locs.fog_end, &fog_end, SHADER_UNIFORM_FLOAT);
    SetShaderValue(material.shader, g_shader_locs.fog_color, &fog_color, SHADER_UNIFORM_VEC3);
    SetShaderValue(material.shader, g_shader_locs.underwater, &underwater_flag, SHADER_UNIFORM_INT);
}

void world_render_with_time(World* world, float time_of_day, Vector3 camera_pos, bool underwater) {
    if (!world) return;

    // Enable alpha blending for transparent blocks (leaves, water, etc.)
    rlSetBlendMode(RL_BLEND_ALPHA);

    Material material = texture_atlas_get_material();

    // Initialize shader location cache on first call
    init_shader_loc_cache(material.shader);

    // Apply common shader uniforms
    apply_world_shader_uniforms(material, world, time_of_day, camera_pos, underwater);

    // Use batched rendering for reduced draw calls
    if (world->batcher) {
        chunk_batcher_render_opaque(world->batcher, world, material, camera_pos);
    }
}

void world_render(World* world) {
    // Fallback to noon lighting if time not specified
    Vector3 default_camera = {0, 64, 0};
    world_render_with_time(world, 12.0f, default_camera, false);
}

void world_render_transparent_with_time(World* world, float time_of_day, Vector3 camera_pos, bool underwater) {
    if (!world) return;

    Material material = texture_atlas_get_material();

    // Initialize shader location cache on first call (may already be initialized by opaque pass)
    init_shader_loc_cache(material.shader);

    // Apply common shader uniforms
    apply_world_shader_uniforms(material, world, time_of_day, camera_pos, underwater);

    // Use batched rendering for reduced draw calls (handles back-to-front sorting internally)
    if (world->batcher) {
        chunk_batcher_render_transparent(world->batcher, world, material, camera_pos);
    }
}
