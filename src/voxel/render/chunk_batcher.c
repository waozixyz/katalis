/**
 * Chunk Batcher Implementation
 *
 * Merges 2x2 chunks into single meshes to reduce draw calls by 75%
 */

#include "voxel/render/chunk_batcher.h"
#include "voxel/world/world.h"
#include "voxel/world/chunk.h"
#include "voxel/world/chunk_worker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <raymath.h>

// ============================================================================
// HASH MAP HELPERS
// ============================================================================

static unsigned int batch_hash(int batch_x, int batch_z) {
    // Simple hash combining both coordinates
    unsigned int h = (unsigned int)(batch_x * 73856093) ^ (unsigned int)(batch_z * 19349663);
    return h % BATCH_MAX_COUNT;
}

// ============================================================================
// TRANSPARENT SORTING
// ============================================================================

/**
 * Comparison function for qsort - sorts back-to-front (farthest first)
 */
static int compare_sort_entries(const void* a, const void* b) {
    const SortEntry* ea = (const SortEntry*)a;
    const SortEntry* eb = (const SortEntry*)b;
    // Sort descending (farthest first for back-to-front rendering)
    if (eb->dist_sq > ea->dist_sq) return 1;
    if (eb->dist_sq < ea->dist_sq) return -1;
    return 0;
}

// ============================================================================
// BATCH MESH BUILDING
// ============================================================================

/**
 * Count total vertices needed for a batch's combined mesh
 */
static int count_batch_vertices(ChunkBatch* batch, bool transparent) {
    int total = 0;
    for (int bz = 0; bz < BATCH_SIZE; bz++) {
        for (int bx = 0; bx < BATCH_SIZE; bx++) {
            Chunk* chunk = batch->chunks[bx][bz];
            if (!chunk) continue;

            // Select mesh based on LOD distance (simplified: always use full detail for batches)
            if (transparent) {
                if (chunk->transparent_mesh_generated && chunk->transparent_mesh.vertexCount > 0) {
                    total += chunk->transparent_mesh.vertexCount;
                }
            } else {
                if (chunk->mesh_generated && chunk->mesh.vertexCount > 0) {
                    total += chunk->mesh.vertexCount;
                }
            }
        }
    }
    return total;
}

/**
 * Build combined mesh from all chunks in batch
 */
static void build_batch_mesh(ChunkBatch* batch, bool transparent) {
    int total_vertices = count_batch_vertices(batch, transparent);

    if (total_vertices == 0) {
        if (transparent) {
            batch->transparent_valid = false;
        } else {
            batch->opaque_valid = false;
        }
        return;
    }

    // Allocate combined buffers
    float* vertices = (float*)malloc(total_vertices * 3 * sizeof(float));
    float* texcoords = (float*)malloc(total_vertices * 2 * sizeof(float));
    float* normals = (float*)malloc(total_vertices * 3 * sizeof(float));
    unsigned char* colors = (unsigned char*)malloc(total_vertices * 4);

    if (!vertices || !texcoords || !normals || !colors) {
        if (vertices) free(vertices);
        if (texcoords) free(texcoords);
        if (normals) free(normals);
        if (colors) free(colors);
        return;
    }

    int offset = 0;

    // Copy vertex data from each chunk
    for (int bz = 0; bz < BATCH_SIZE; bz++) {
        for (int bx = 0; bx < BATCH_SIZE; bx++) {
            Chunk* chunk = batch->chunks[bx][bz];
            if (!chunk) continue;

            Mesh* src_mesh;
            bool has_mesh;

            if (transparent) {
                src_mesh = &chunk->transparent_mesh;
                has_mesh = chunk->transparent_mesh_generated && src_mesh->vertexCount > 0;
            } else {
                src_mesh = &chunk->mesh;
                has_mesh = chunk->mesh_generated && src_mesh->vertexCount > 0;
            }

            if (!has_mesh) continue;

            int vc = src_mesh->vertexCount;

            // Calculate chunk offset within batch
            float chunk_offset_x = (float)((chunk->x - batch->batch_x * BATCH_SIZE) * CHUNK_SIZE);
            float chunk_offset_z = (float)((chunk->z - batch->batch_z * BATCH_SIZE) * CHUNK_SIZE);

            // Copy and offset vertices
            for (int i = 0; i < vc; i++) {
                vertices[(offset + i) * 3 + 0] = src_mesh->vertices[i * 3 + 0] + chunk_offset_x;
                vertices[(offset + i) * 3 + 1] = src_mesh->vertices[i * 3 + 1];  // Y unchanged
                vertices[(offset + i) * 3 + 2] = src_mesh->vertices[i * 3 + 2] + chunk_offset_z;
            }

            // Copy texcoords (no offset needed)
            memcpy(texcoords + offset * 2, src_mesh->texcoords, vc * 2 * sizeof(float));

            // Copy normals (no change)
            memcpy(normals + offset * 3, src_mesh->normals, vc * 3 * sizeof(float));

            // Copy colors (no change)
            memcpy(colors + offset * 4, src_mesh->colors, vc * 4);

            offset += vc;
        }
    }

    // Create combined mesh
    Mesh* target_mesh = transparent ? &batch->transparent_mesh : &batch->opaque_mesh;

    // Unload old mesh if exists
    if ((transparent ? batch->transparent_valid : batch->opaque_valid) && target_mesh->vboId != NULL) {
        UnloadMesh(*target_mesh);
    }

    memset(target_mesh, 0, sizeof(Mesh));
    target_mesh->vertexCount = total_vertices;
    target_mesh->triangleCount = total_vertices / 3;
    target_mesh->vertices = vertices;
    target_mesh->texcoords = texcoords;
    target_mesh->normals = normals;
    target_mesh->colors = colors;

    UploadMesh(target_mesh, false);

    if (transparent) {
        batch->transparent_valid = true;
    } else {
        batch->opaque_valid = true;
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

ChunkBatcher* chunk_batcher_create(void) {
    ChunkBatcher* batcher = (ChunkBatcher*)calloc(1, sizeof(ChunkBatcher));
    if (!batcher) {
        printf("[BATCHER] Failed to allocate batcher\n");
        return NULL;
    }

    // Pre-allocate sort buffer for transparent rendering (avoids per-frame malloc)
    batcher->sort_buffer_capacity = 2048;
    batcher->sort_buffer = (SortEntry*)malloc(batcher->sort_buffer_capacity * sizeof(SortEntry));
    if (!batcher->sort_buffer) {
        printf("[BATCHER] Warning: Failed to allocate sort buffer, will use per-frame allocation\n");
        batcher->sort_buffer_capacity = 0;
    }

    printf("[BATCHER] Created chunk batcher (2x2 batching)\n");
    return batcher;
}

void chunk_batcher_destroy(ChunkBatcher* batcher) {
    if (!batcher) return;

    // Free all batches
    for (int i = 0; i < BATCH_MAX_COUNT; i++) {
        BatchNode* node = batcher->buckets[i];
        while (node) {
            BatchNode* next = node->next;

            // Unload meshes
            if (node->batch.opaque_valid && node->batch.opaque_mesh.vboId != NULL) {
                UnloadMesh(node->batch.opaque_mesh);
            }
            if (node->batch.transparent_valid && node->batch.transparent_mesh.vboId != NULL) {
                UnloadMesh(node->batch.transparent_mesh);
            }

            free(node);
            node = next;
        }
    }

    // Free pre-allocated sort buffer
    if (batcher->sort_buffer) {
        free(batcher->sort_buffer);
    }

    free(batcher);
    printf("[BATCHER] Destroyed chunk batcher\n");
}

ChunkBatch* chunk_batcher_get_batch(ChunkBatcher* batcher, int chunk_x, int chunk_z) {
    if (!batcher) return NULL;

    int batch_x, batch_z;
    chunk_to_batch_coords(chunk_x, chunk_z, &batch_x, &batch_z);

    unsigned int hash = batch_hash(batch_x, batch_z);
    BatchNode* node = batcher->buckets[hash];

    // Search for existing batch
    while (node) {
        if (node->batch.batch_x == batch_x && node->batch.batch_z == batch_z) {
            return &node->batch;
        }
        node = node->next;
    }

    // Create new batch
    BatchNode* new_node = (BatchNode*)calloc(1, sizeof(BatchNode));
    if (!new_node) return NULL;

    new_node->batch.batch_x = batch_x;
    new_node->batch.batch_z = batch_z;
    new_node->batch.dirty = true;
    new_node->next = batcher->buckets[hash];
    batcher->buckets[hash] = new_node;
    batcher->batch_count++;
    batcher->dirty_count++;

    return &new_node->batch;
}

void chunk_batcher_register_chunk(ChunkBatcher* batcher, Chunk* chunk) {
    if (!batcher || !chunk) return;

    ChunkBatch* batch = chunk_batcher_get_batch(batcher, chunk->x, chunk->z);
    if (!batch) return;

    // Calculate position within batch
    int bx = chunk->x - batch->batch_x * BATCH_SIZE;
    int bz = chunk->z - batch->batch_z * BATCH_SIZE;

    // Handle negative coordinates
    if (bx < 0) bx += BATCH_SIZE;
    if (bz < 0) bz += BATCH_SIZE;

    if (bx >= 0 && bx < BATCH_SIZE && bz >= 0 && bz < BATCH_SIZE) {
        if (batch->chunks[bx][bz] == NULL) {
            batch->chunk_count++;
        }
        batch->chunks[bx][bz] = chunk;
        if (!batch->dirty) {
            batch->dirty = true;
            batcher->dirty_count++;
        }
    }
}

void chunk_batcher_unregister_chunk(ChunkBatcher* batcher, Chunk* chunk) {
    if (!batcher || !chunk) return;

    int batch_x, batch_z;
    chunk_to_batch_coords(chunk->x, chunk->z, &batch_x, &batch_z);

    unsigned int hash = batch_hash(batch_x, batch_z);
    BatchNode* node = batcher->buckets[hash];

    while (node) {
        if (node->batch.batch_x == batch_x && node->batch.batch_z == batch_z) {
            int bx = chunk->x - batch_x * BATCH_SIZE;
            int bz = chunk->z - batch_z * BATCH_SIZE;
            if (bx < 0) bx += BATCH_SIZE;
            if (bz < 0) bz += BATCH_SIZE;

            if (bx >= 0 && bx < BATCH_SIZE && bz >= 0 && bz < BATCH_SIZE) {
                if (node->batch.chunks[bx][bz] != NULL) {
                    node->batch.chunk_count--;
                }
                node->batch.chunks[bx][bz] = NULL;
                if (!node->batch.dirty) batcher->dirty_count++;
                node->batch.dirty = true;
            }
            return;
        }
        node = node->next;
    }
}

void chunk_batcher_invalidate(ChunkBatcher* batcher, int chunk_x, int chunk_z) {
    if (!batcher) return;

    ChunkBatch* batch = chunk_batcher_get_batch(batcher, chunk_x, chunk_z);
    if (batch && !batch->dirty) {
        batch->dirty = true;
        batcher->dirty_count++;
    }
}

void chunk_batcher_update(ChunkBatcher* batcher, int max_rebuilds) {
    if (!batcher || batcher->dirty_count == 0) return;

    // Use default if not specified
    if (max_rebuilds <= 0) max_rebuilds = BATCH_REBUILDS_PER_FRAME;

    int rebuilt = 0;

    for (int i = 0; i < BATCH_MAX_COUNT && rebuilt < max_rebuilds; i++) {
        BatchNode* node = batcher->buckets[i];
        while (node && rebuilt < max_rebuilds) {
            if (node->batch.dirty) {
                // Rebuild both meshes
                build_batch_mesh(&node->batch, false);  // Opaque
                build_batch_mesh(&node->batch, true);   // Transparent

                node->batch.dirty = false;
                batcher->dirty_count--;
                rebuilt++;
            }
            node = node->next;
        }
    }
}

void chunk_batcher_render_opaque(ChunkBatcher* batcher, World* world,
                                  Material material, Vector3 camera_pos) {
    (void)camera_pos;  // Suppress unused warning
    if (!batcher || !world) return;

    int view_dist = world_get_view_distance(world);
    int center_x = world->center_chunk_x;
    int center_z = world->center_chunk_z;

    // Calculate batch view distance
    int batch_view_dist = (view_dist / BATCH_SIZE) + 1;

    int center_batch_x, center_batch_z;
    chunk_to_batch_coords(center_x, center_z, &center_batch_x, &center_batch_z);

    int rendered_batches = 0;
    int rendered_chunks = 0;
    int missing_batches = 0;

    for (int bz = -batch_view_dist; bz <= batch_view_dist; bz++) {
        for (int bx = -batch_view_dist; bx <= batch_view_dist; bx++) {
            int batch_x = center_batch_x + bx;
            int batch_z = center_batch_z + bz;

            unsigned int hash = batch_hash(batch_x, batch_z);
            BatchNode* node = batcher->buckets[hash];

            bool found = false;
            while (node) {
                if (node->batch.batch_x == batch_x && node->batch.batch_z == batch_z) {
                    found = true;
                    if (node->batch.opaque_valid && node->batch.opaque_mesh.vboId != NULL) {
                        // Batch origin in world coordinates
                        float origin_x = (float)(batch_x * BATCH_SIZE * CHUNK_SIZE);
                        float origin_z = (float)(batch_z * BATCH_SIZE * CHUNK_SIZE);

                        Matrix transform = MatrixTranslate(origin_x, 0.0f, origin_z);
                        DrawMesh(node->batch.opaque_mesh, material, transform);
                        rendered_batches++;
                    } else {
                        // Fallback: render individual chunks when batch not built yet
                        for (int cbz = 0; cbz < BATCH_SIZE; cbz++) {
                            for (int cbx = 0; cbx < BATCH_SIZE; cbx++) {
                                Chunk* chunk = node->batch.chunks[cbx][cbz];
                                if (chunk && chunk->mesh_generated && chunk->mesh.vboId != NULL) {
                                    float origin_x = (float)(chunk->x * CHUNK_SIZE);
                                    float origin_z = (float)(chunk->z * CHUNK_SIZE);
                                    Matrix transform = MatrixTranslate(origin_x, 0.0f, origin_z);
                                    DrawMesh(chunk->mesh, material, transform);
                                    rendered_chunks++;
                                }
                            }
                        }
                    }
                    break;
                }
                node = node->next;
            }
            if (!found) {
                missing_batches++;
                // Fallback: try to render chunks at this batch position directly from world
                for (int cbz = 0; cbz < BATCH_SIZE; cbz++) {
                    for (int cbx = 0; cbx < BATCH_SIZE; cbx++) {
                        int chunk_x = batch_x * BATCH_SIZE + cbx;
                        int chunk_z = batch_z * BATCH_SIZE + cbz;
                        Chunk* chunk = world_get_chunk(world, chunk_x, chunk_z);
                        if (chunk && chunk->mesh_generated && chunk->mesh.vboId != NULL) {
                            float origin_x = (float)(chunk->x * CHUNK_SIZE);
                            float origin_z = (float)(chunk->z * CHUNK_SIZE);
                            Matrix transform = MatrixTranslate(origin_x, 0.0f, origin_z);
                            DrawMesh(chunk->mesh, material, transform);
                            rendered_chunks++;
                        }
                    }
                }
            }
        }
    }

    // Debug output (uncomment to enable):
    // static int frame = 0;
    // if (++frame % 60 == 0) {
    //     printf("[BATCHER] batches=%d chunks=%d missing=%d\n",
    //            rendered_batches, rendered_chunks, missing_batches);
    //     fflush(stdout);
    // }
    (void)rendered_batches; (void)rendered_chunks; (void)missing_batches;  // Suppress warnings
}

void chunk_batcher_render_transparent(ChunkBatcher* batcher, World* world,
                                       Material material, Vector3 camera_pos) {
    if (!batcher || !world) return;

    int view_dist = world_get_view_distance(world);
    int center_x = world->center_chunk_x;
    int center_z = world->center_chunk_z;

    int batch_view_dist = (view_dist / BATCH_SIZE) + 1;

    int center_batch_x, center_batch_z;
    chunk_to_batch_coords(center_x, center_z, &center_batch_x, &center_batch_z);

    // Use pre-allocated buffer if available, fallback to malloc
    SortEntry* entries;
    bool needs_free = false;
    if (batcher->sort_buffer && batcher->sort_buffer_capacity > 0) {
        entries = batcher->sort_buffer;
    } else {
        entries = (SortEntry*)malloc(BATCH_MAX_COUNT * 4 * sizeof(SortEntry));
        if (!entries) return;
        needs_free = true;
    }
    int max_entries = batcher->sort_buffer_capacity > 0 ? batcher->sort_buffer_capacity : (BATCH_MAX_COUNT * 4);

    int count = 0;

    for (int bz = -batch_view_dist; bz <= batch_view_dist; bz++) {
        for (int bx = -batch_view_dist; bx <= batch_view_dist; bx++) {
            int batch_x = center_batch_x + bx;
            int batch_z = center_batch_z + bz;

            unsigned int hash = batch_hash(batch_x, batch_z);
            BatchNode* node = batcher->buckets[hash];

            bool found = false;
            while (node) {
                if (node->batch.batch_x == batch_x && node->batch.batch_z == batch_z) {
                    found = true;
                    if (node->batch.transparent_valid && node->batch.transparent_mesh.vboId != NULL) {
                        if (count < max_entries) {
                            float cx = (batch_x * BATCH_SIZE + BATCH_SIZE / 2.0f) * CHUNK_SIZE;
                            float cz = (batch_z * BATCH_SIZE + BATCH_SIZE / 2.0f) * CHUNK_SIZE;
                            float dx = cx - camera_pos.x;
                            float dz = cz - camera_pos.z;

                            entries[count].batch = &node->batch;
                            entries[count].dist_sq = dx * dx + dz * dz;
                            entries[count].is_batch = true;
                            count++;
                        }
                    } else {
                        // Fallback: add individual chunks from batch
                        for (int cbz = 0; cbz < BATCH_SIZE && count < max_entries; cbz++) {
                            for (int cbx = 0; cbx < BATCH_SIZE && count < max_entries; cbx++) {
                                Chunk* chunk = node->batch.chunks[cbx][cbz];
                                if (chunk && chunk->transparent_mesh_generated &&
                                    chunk->transparent_mesh.vboId != NULL) {
                                    float cx = (chunk->x + 0.5f) * CHUNK_SIZE;
                                    float cz = (chunk->z + 0.5f) * CHUNK_SIZE;
                                    float dx = cx - camera_pos.x;
                                    float dz = cz - camera_pos.z;

                                    entries[count].chunk = chunk;
                                    entries[count].dist_sq = dx * dx + dz * dz;
                                    entries[count].is_batch = false;
                                    count++;
                                }
                            }
                        }
                    }
                    break;
                }
                node = node->next;
            }

            // If no batch found, try to find chunks directly from world
            if (!found) {
                for (int cbz = 0; cbz < BATCH_SIZE && count < max_entries; cbz++) {
                    for (int cbx = 0; cbx < BATCH_SIZE && count < max_entries; cbx++) {
                        int chunk_x = batch_x * BATCH_SIZE + cbx;
                        int chunk_z = batch_z * BATCH_SIZE + cbz;
                        Chunk* chunk = world_get_chunk(world, chunk_x, chunk_z);
                        if (chunk && chunk->transparent_mesh_generated &&
                            chunk->transparent_mesh.vboId != NULL) {
                            float cx = (chunk->x + 0.5f) * CHUNK_SIZE;
                            float cz = (chunk->z + 0.5f) * CHUNK_SIZE;
                            float dx = cx - camera_pos.x;
                            float dz = cz - camera_pos.z;

                            entries[count].chunk = chunk;
                            entries[count].dist_sq = dx * dx + dz * dz;
                            entries[count].is_batch = false;
                            count++;
                        }
                    }
                }
            }
        }
    }

    // Sort back-to-front using qsort (O(n log n) vs O(n²) bubble sort)
    if (count > 1) {
        qsort(entries, count, sizeof(SortEntry), compare_sort_entries);
    }

    // Render sorted
    for (int i = 0; i < count; i++) {
        if (entries[i].is_batch) {
            ChunkBatch* batch = entries[i].batch;
            float origin_x = (float)(batch->batch_x * BATCH_SIZE * CHUNK_SIZE);
            float origin_z = (float)(batch->batch_z * BATCH_SIZE * CHUNK_SIZE);
            Matrix transform = MatrixTranslate(origin_x, 0.0f, origin_z);
            DrawMesh(batch->transparent_mesh, material, transform);
        } else {
            Chunk* chunk = entries[i].chunk;
            float origin_x = (float)(chunk->x * CHUNK_SIZE);
            float origin_z = (float)(chunk->z * CHUNK_SIZE);
            Matrix transform = MatrixTranslate(origin_x, 0.0f, origin_z);
            DrawMesh(chunk->transparent_mesh, material, transform);
        }
    }

    if (needs_free) {
        free(entries);
    }
}
