/**
 * World System Implementation
 */

#include "voxel/world.h"
#include "voxel/texture_atlas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>

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

World* world_create(void) {
    World* world = (World*)malloc(sizeof(World));
    world->chunks = chunk_hashmap_create();
    world->center_chunk_x = 0;
    world->center_chunk_z = 0;
    world->view_distance = WORLD_VIEW_DISTANCE;

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
}

void world_update(World* world, int center_chunk_x, int center_chunk_z) {
    if (!world) return;

    world->center_chunk_x = center_chunk_x;
    world->center_chunk_z = center_chunk_z;

    // TODO: Implement chunk loading/unloading based on view distance
    // For now, we'll keep all chunks loaded

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
        }
    }
}

void world_render(World* world) {
    if (!world) return;

    Matrix identity = MatrixIdentity();
    Material material = texture_atlas_get_material();

    // Render all loaded chunks
    for (int i = 0; i < WORLD_MAX_CHUNKS; i++) {
        ChunkNode* node = world->chunks->buckets[i];
        while (node) {
            Chunk* chunk = node->chunk;

            // Generate mesh if needed
            if (chunk->needs_remesh) {
                chunk_generate_mesh(chunk);
            }

            // Render chunk if it has a mesh
            if (chunk->mesh_generated && chunk->mesh.vboId != NULL) {
                // Calculate world position offset for this chunk
                Matrix transform = MatrixTranslate(
                    (float)(chunk->x * CHUNK_SIZE),
                    0.0f,
                    (float)(chunk->z * CHUNK_SIZE)
                );

                DrawMesh(chunk->mesh, material, transform);
            }

            node = node->next;
        }
    }
}
