/**
 * World System Implementation
 */

#include "voxel/world.h"
#include "voxel/texture_atlas.h"
#include "voxel/terrain.h"
#include "voxel/light.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

// ============================================================================
// FRUSTUM CULLING
// ============================================================================

typedef struct {
    Vector4 planes[6];  // left, right, bottom, top, near, far
} Frustum;

/**
 * Extract frustum planes from view-projection matrix
 */
static Frustum frustum_extract(Matrix vp) {
    Frustum f;

    // Left plane
    f.planes[0] = (Vector4){
        vp.m3 + vp.m0, vp.m7 + vp.m4, vp.m11 + vp.m8, vp.m15 + vp.m12
    };

    // Right plane
    f.planes[1] = (Vector4){
        vp.m3 - vp.m0, vp.m7 - vp.m4, vp.m11 - vp.m8, vp.m15 - vp.m12
    };

    // Bottom plane
    f.planes[2] = (Vector4){
        vp.m3 + vp.m1, vp.m7 + vp.m5, vp.m11 + vp.m9, vp.m15 + vp.m13
    };

    // Top plane
    f.planes[3] = (Vector4){
        vp.m3 - vp.m1, vp.m7 - vp.m5, vp.m11 - vp.m9, vp.m15 - vp.m13
    };

    // Near plane
    f.planes[4] = (Vector4){
        vp.m3 + vp.m2, vp.m7 + vp.m6, vp.m11 + vp.m10, vp.m15 + vp.m14
    };

    // Far plane
    f.planes[5] = (Vector4){
        vp.m3 - vp.m2, vp.m7 - vp.m6, vp.m11 - vp.m10, vp.m15 - vp.m14
    };

    // Normalize all planes
    for (int i = 0; i < 6; i++) {
        float len = sqrtf(f.planes[i].x * f.planes[i].x +
                         f.planes[i].y * f.planes[i].y +
                         f.planes[i].z * f.planes[i].z);
        if (len > 0.0001f) {
            f.planes[i].x /= len;
            f.planes[i].y /= len;
            f.planes[i].z /= len;
            f.planes[i].w /= len;
        }
    }

    return f;
}

/**
 * Check if AABB is inside or intersects frustum
 */
static bool frustum_contains_aabb(const Frustum* f, Vector3 min, Vector3 max) {
    for (int i = 0; i < 6; i++) {
        Vector4 p = f->planes[i];

        // Find the positive vertex (furthest along plane normal)
        Vector3 positive = {
            (p.x >= 0) ? max.x : min.x,
            (p.y >= 0) ? max.y : min.y,
            (p.z >= 0) ? max.z : min.z
        };

        // If positive vertex is outside, entire AABB is outside
        if (p.x * positive.x + p.y * positive.y + p.z * positive.z + p.w < 0) {
            return false;
        }
    }

    return true;
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

/**
 * Remove chunk from hash map
 */
static void chunk_hashmap_remove(ChunkHashMap* map, int chunk_x, int chunk_z) {
    uint32_t hash = hash_chunk_coords(chunk_x, chunk_z);
    ChunkNode* node = map->buckets[hash];
    ChunkNode* prev = NULL;

    while (node) {
        if (node->chunk_x == chunk_x && node->chunk_z == chunk_z) {
            if (prev) {
                prev->next = node->next;
            } else {
                map->buckets[hash] = node->next;
            }

            chunk_destroy(node->chunk);
            free(node);
            map->chunk_count--;
            return;
        }
        prev = node;
        node = node->next;
    }
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
    world->center_chunk_x = 0;
    world->center_chunk_z = 0;
    world->view_distance = WORLD_VIEW_DISTANCE;
    world->terrain_params = terrain_params;

    printf("[WORLD] Created world with view distance %d\n", world->view_distance);
    return world;
}

void world_destroy(World* world) {
    if (!world) return;

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

    // Recalculate lighting for this chunk when a block changes
    light_calculate_chunk(chunk);
}

void world_update(World* world, int center_chunk_x, int center_chunk_z) {
    if (!world) return;

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

    // Load chunks in view distance if not already loaded
    int new_chunks = 0;
    for (int x = -world->view_distance; x <= world->view_distance; x++) {
        for (int z = -world->view_distance; z <= world->view_distance; z++) {
            int cx = center_chunk_x + x;
            int cz = center_chunk_z + z;

            Chunk* chunk = world_get_chunk(world, cx, cz);
            if (!chunk) {
                // Chunk not loaded, create it with terrain generation
                chunk = world_get_or_create_chunk(world, cx, cz);

                // Generate terrain for this new chunk
                terrain_generate_chunk(chunk, world->terrain_params);

                // Update empty status after terrain generation
                chunk_update_empty_status(chunk);

                // Generate mesh for rendering
                chunk_generate_mesh(chunk);

                new_chunks++;
            }
        }
    }

    if (first_update) {
        first_update = false;
    }

    // Process pending mesh regenerations (moved from render phase)
    // This prevents GPU stalls during rendering
    for (int i = 0; i < WORLD_MAX_CHUNKS; i++) {
        ChunkNode* node = world->chunks->buckets[i];
        while (node) {
            Chunk* chunk = node->chunk;
            if (chunk->needs_remesh) {
                chunk_generate_mesh(chunk);
            }
            node = node->next;
        }
    }
}

// ============================================================================
// LIGHTING HELPERS
// ============================================================================

/**
 * Calculate ambient brightness from time of day
 */
static float calculate_ambient_brightness(float time) {
    if (time < 6.0f) return 0.25f;     // Night (0-6am): Dark but visible
    if (time < 7.0f) {                 // Dawn (6-7am): Transition
        return 0.25f + (time - 6.0f) * 0.75f;
    }
    if (time < 18.0f) return 1.0f;     // Day (7am-6pm): Full bright
    if (time < 19.0f) {                // Dusk (6-7pm): Transition
        return 1.0f - (time - 18.0f) * 0.75f;
    }
    return 0.25f;                       // Night (7pm-12am): Dark but visible
}

/**
 * Get ambient light color with temperature shift
 */
static Vector3 get_ambient_color(float time) {
    float brightness = calculate_ambient_brightness(time);

    // Color temperature shifts
    if (time >= 6.0f && time < 8.0f) {
        // Dawn: Warm orange tint
        return (Vector3){brightness * 1.0f, brightness * 0.8f, brightness * 0.6f};
    } else if (time >= 17.0f && time < 19.0f) {
        // Dusk: Warm red/orange tint
        return (Vector3){brightness * 1.0f, brightness * 0.7f, brightness * 0.5f};
    } else if (time < 6.0f || time >= 19.0f) {
        // Night: Cool blue tint
        return (Vector3){brightness * 0.5f, brightness * 0.6f, brightness * 0.8f};
    } else {
        // Day: Neutral white
        return (Vector3){brightness, brightness, brightness};
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void world_render_with_time(World* world, float time_of_day) {
    if (!world) return;

    Material material = texture_atlas_get_material();

    // Calculate and set ambient light
    Vector3 ambient_light = get_ambient_color(time_of_day);
    int ambient_loc = GetShaderLocation(material.shader, "u_ambient_light");
    SetShaderValue(material.shader, ambient_loc, &ambient_light, SHADER_UNIFORM_VEC3);

    // Calculate max render distance (in chunks) for culling
    int max_dist = world->view_distance + 1;

    // Extract frustum from current view-projection matrix
    Matrix view = rlGetMatrixModelview();
    Matrix proj = rlGetMatrixProjection();
    Matrix vp = MatrixMultiply(view, proj);
    Frustum frustum = frustum_extract(vp);

    // Render all loaded chunks (mesh generation moved to world_update)
    for (int i = 0; i < WORLD_MAX_CHUNKS; i++) {
        ChunkNode* node = world->chunks->buckets[i];
        while (node) {
            Chunk* chunk = node->chunk;

            // Distance-based culling: skip chunks beyond view distance
            int dx = chunk->x - world->center_chunk_x;
            int dz = chunk->z - world->center_chunk_z;
            if (dx * dx + dz * dz > max_dist * max_dist) {
                node = node->next;
                continue;
            }

            // Frustum culling: skip chunks not visible to camera
            Vector3 chunk_min = {
                (float)(chunk->x * CHUNK_SIZE),
                0.0f,
                (float)(chunk->z * CHUNK_SIZE)
            };
            Vector3 chunk_max = {
                chunk_min.x + CHUNK_SIZE,
                CHUNK_HEIGHT,
                chunk_min.z + CHUNK_SIZE
            };
            if (!frustum_contains_aabb(&frustum, chunk_min, chunk_max)) {
                node = node->next;
                continue;
            }

            // Render chunk if it has a mesh
            if (chunk->mesh_generated && chunk->mesh.vboId != NULL) {
                Matrix transform = MatrixTranslate(
                    chunk_min.x,
                    0.0f,
                    chunk_min.z
                );

                DrawMesh(chunk->mesh, material, transform);
            }

            node = node->next;
        }
    }
}

void world_render(World* world) {
    // Fallback to noon lighting if time not specified
    world_render_with_time(world, 12.0f);
}
