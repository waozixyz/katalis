/**
 * Chunk System Implementation
 */

#include "voxel/world/chunk.h"
#include "voxel/world/chunk_worker.h"
#include "voxel/core/texture_atlas.h"
#include "voxel/render/light.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <rlgl.h>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Check if coordinates are within chunk bounds
 */
bool chunk_in_bounds(int x, int y, int z) {
    return (x >= 0 && x < CHUNK_SIZE &&
            y >= 0 && y < CHUNK_HEIGHT &&
            z >= 0 && z < CHUNK_SIZE);
}

// ============================================================================
// CHUNK MANAGEMENT
// ============================================================================

/**
 * Create a new chunk
 */
Chunk* chunk_create(int x, int z) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    if (!chunk) {
        printf("[CHUNK] Failed to allocate chunk\n");
        return NULL;
    }

    chunk->x = x;
    chunk->z = z;
    chunk->needs_remesh = true;
    chunk->is_empty = true;
    chunk->mesh_generated = false;
    chunk->transparent_mesh_generated = false;
    chunk->has_spawned = false;
    chunk->solid_block_count = 0;
    chunk->state = CHUNK_STATE_EMPTY;

    // Initialize all blocks to air
    memset(chunk->blocks, 0, sizeof(chunk->blocks));

    // Initialize meshes to zero
    memset(&chunk->mesh, 0, sizeof(Mesh));
    memset(&chunk->transparent_mesh, 0, sizeof(Mesh));

    return chunk;
}

/**
 * Destroy chunk
 */
void chunk_destroy(Chunk* chunk) {
    if (!chunk) return;

    // Unload opaque mesh from GPU if it was generated
    if (chunk->mesh_generated && chunk->mesh.vboId != NULL) {
        UnloadMesh(chunk->mesh);
    }

    // Unload transparent mesh from GPU if it was generated
    if (chunk->transparent_mesh_generated && chunk->transparent_mesh.vboId != NULL) {
        UnloadMesh(chunk->transparent_mesh);
    }

    free(chunk);
}

/**
 * Set block at local coordinates
 */
void chunk_set_block(Chunk* chunk, int x, int y, int z, Block block) {
    if (!chunk || !chunk_in_bounds(x, y, z)) {
        return;
    }

    // Get old block type for counter update
    BlockType old_type = chunk->blocks[x][y][z].type;
    BlockType new_type = block.type;

    chunk->blocks[x][y][z] = block;
    chunk->needs_remesh = true;

    // Update solid block counter (O(1) instead of O(n) scan)
    if (old_type == BLOCK_AIR && new_type != BLOCK_AIR) {
        chunk->solid_block_count++;
    } else if (old_type != BLOCK_AIR && new_type == BLOCK_AIR) {
        chunk->solid_block_count--;
    }

    chunk->is_empty = (chunk->solid_block_count == 0);
}

/**
 * Get block at local coordinates
 */
Block chunk_get_block(Chunk* chunk, int x, int y, int z) {
    if (!chunk || !chunk_in_bounds(x, y, z)) {
        return (Block){BLOCK_AIR, 0, 0};
    }

    return chunk->blocks[x][y][z];
}

/**
 * Fill chunk with specific block type
 */
void chunk_fill(Chunk* chunk, BlockType type) {
    if (!chunk) return;

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                chunk->blocks[x][y][z].type = type;
            }
        }
    }

    // Update counter based on fill type
    chunk->solid_block_count = (type == BLOCK_AIR) ? 0 : (CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE);
    chunk->is_empty = (type == BLOCK_AIR);
    chunk->needs_remesh = true;
}

/**
 * Check if chunk is empty
 */
bool chunk_is_empty(Chunk* chunk) {
    if (!chunk) return true;
    return chunk->is_empty;
}

/**
 * Update the is_empty flag and solid_block_count by scanning the chunk
 * Call this after bulk terrain generation
 */
void chunk_update_empty_status(Chunk* chunk) {
    if (!chunk) return;

    int count = 0;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (chunk->blocks[x][y][z].type != BLOCK_AIR) {
                    count++;
                }
            }
        }
    }

    chunk->solid_block_count = count;
    chunk->is_empty = (count == 0);
}

// ============================================================================
// MESH GENERATION (Basic Face Culling)
// ============================================================================

/**
 * Check if there's a solid block at position (used for face culling)
 */
static bool has_solid_block_at(Chunk* chunk, int x, int y, int z) {
    if (!chunk_in_bounds(x, y, z)) {
        return false;  // Out of bounds = air
    }

    Block block = chunk_get_block(chunk, x, y, z);
    return block_is_solid(block) && !block_is_transparent(block);
}

/**
 * Determine block face from normal vector
 */
static BlockFace get_face_from_normal(Vector3 normal) {
    if (normal.y > 0.5f) return FACE_TOP;
    if (normal.y < -0.5f) return FACE_BOTTOM;
    if (normal.x > 0.5f) return FACE_RIGHT;
    if (normal.x < -0.5f) return FACE_LEFT;
    if (normal.z > 0.5f) return FACE_BACK;
    return FACE_FRONT;
}

/**
 * Calculate lighting brightness based on face normal
 */
static float calculate_face_brightness(Vector3 normal) {
    // Simple directional lighting - brighter values for better visibility
    // All faces should be clearly visible, not black

    if (normal.y > 0.5f) return 1.0f;      // Top - full brightness
    if (normal.y < -0.5f) return 0.8f;     // Bottom - much brighter now

    // Side faces - all very bright
    if (normal.x > 0.5f) return 0.95f;     // Right (+X)
    if (normal.x < -0.5f) return 0.95f;    // Left (-X)
    if (normal.z > 0.5f) return 0.9f;      // Back (+Z)
    if (normal.z < -0.5f) return 0.9f;     // Front (-Z)

    return 0.9f;  // Fallback
}

/**
 * Add a quad face to the mesh buffers
 */
static void add_quad(float* vertices, float* texcoords, float* normals, unsigned char* colors, int* vertex_count,
                     Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Vector3 normal,
                     BlockType block_type, float width, float height, uint8_t block_light_level) {
    int idx = *vertex_count;

    // Get face type from normal
    BlockFace face = get_face_from_normal(normal);

    // Get texture coordinates from atlas
    TextureCoords tex = texture_atlas_get_coords(block_type, face);

    // Don't scale UVs - just stretch the texture across the greedy quad
    // This prevents bleeding into other atlas tiles

    // Calculate face brightness based on normal direction
    float face_brightness = calculate_face_brightness(normal);

    // Apply block's light level (0-15 -> 0.0-1.0)
    float light_factor = (float)block_light_level / 15.0f;

    // Minimum ambient so caves aren't pitch black (15% ambient)
    float min_ambient = 0.15f;
    light_factor = min_ambient + light_factor * (1.0f - min_ambient);

    // Final brightness combines face direction and block light level
    float brightness = face_brightness * light_factor;
    unsigned char light = (unsigned char)(brightness * 255.0f);

    // Triangle 1 (v1, v2, v3)
    vertices[idx * 3 + 0] = v1.x; vertices[idx * 3 + 1] = v1.y; vertices[idx * 3 + 2] = v1.z;
    texcoords[idx * 2 + 0] = tex.u_min; texcoords[idx * 2 + 1] = tex.v_min;
    normals[idx * 3 + 0] = normal.x; normals[idx * 3 + 1] = normal.y; normals[idx * 3 + 2] = normal.z;
    colors[idx * 4 + 0] = light; colors[idx * 4 + 1] = light; colors[idx * 4 + 2] = light; colors[idx * 4 + 3] = 255;
    idx++;

    vertices[idx * 3 + 0] = v2.x; vertices[idx * 3 + 1] = v2.y; vertices[idx * 3 + 2] = v2.z;
    texcoords[idx * 2 + 0] = tex.u_max; texcoords[idx * 2 + 1] = tex.v_min;
    normals[idx * 3 + 0] = normal.x; normals[idx * 3 + 1] = normal.y; normals[idx * 3 + 2] = normal.z;
    colors[idx * 4 + 0] = light; colors[idx * 4 + 1] = light; colors[idx * 4 + 2] = light; colors[idx * 4 + 3] = 255;
    idx++;

    vertices[idx * 3 + 0] = v3.x; vertices[idx * 3 + 1] = v3.y; vertices[idx * 3 + 2] = v3.z;
    texcoords[idx * 2 + 0] = tex.u_max; texcoords[idx * 2 + 1] = tex.v_max;
    normals[idx * 3 + 0] = normal.x; normals[idx * 3 + 1] = normal.y; normals[idx * 3 + 2] = normal.z;
    colors[idx * 4 + 0] = light; colors[idx * 4 + 1] = light; colors[idx * 4 + 2] = light; colors[idx * 4 + 3] = 255;
    idx++;

    // Triangle 2 (v1, v3, v4)
    vertices[idx * 3 + 0] = v1.x; vertices[idx * 3 + 1] = v1.y; vertices[idx * 3 + 2] = v1.z;
    texcoords[idx * 2 + 0] = tex.u_min; texcoords[idx * 2 + 1] = tex.v_min;
    normals[idx * 3 + 0] = normal.x; normals[idx * 3 + 1] = normal.y; normals[idx * 3 + 2] = normal.z;
    colors[idx * 4 + 0] = light; colors[idx * 4 + 1] = light; colors[idx * 4 + 2] = light; colors[idx * 4 + 3] = 255;
    idx++;

    vertices[idx * 3 + 0] = v3.x; vertices[idx * 3 + 1] = v3.y; vertices[idx * 3 + 2] = v3.z;
    texcoords[idx * 2 + 0] = tex.u_max; texcoords[idx * 2 + 1] = tex.v_max;
    normals[idx * 3 + 0] = normal.x; normals[idx * 3 + 1] = normal.y; normals[idx * 3 + 2] = normal.z;
    colors[idx * 4 + 0] = light; colors[idx * 4 + 1] = light; colors[idx * 4 + 2] = light; colors[idx * 4 + 3] = 255;
    idx++;

    vertices[idx * 3 + 0] = v4.x; vertices[idx * 3 + 1] = v4.y; vertices[idx * 3 + 2] = v4.z;
    texcoords[idx * 2 + 0] = tex.u_min; texcoords[idx * 2 + 1] = tex.v_max;
    normals[idx * 3 + 0] = normal.x; normals[idx * 3 + 1] = normal.y; normals[idx * 3 + 2] = normal.z;
    colors[idx * 4 + 0] = light; colors[idx * 4 + 1] = light; colors[idx * 4 + 2] = light; colors[idx * 4 + 3] = 255;
    idx++;

    *vertex_count = idx;
}

/**
 * Get consistent light level for a block.
 * All faces of the same block use the same light value (maximum from adjacent air).
 * This prevents jarring differences between faces when digging.
 */
static uint8_t get_block_light(Chunk* chunk, int x, int y, int z) {
    uint8_t max_light = LIGHT_MIN;

    // Check all 6 neighbors for the maximum light
    static const int offsets[6][3] = {
        {-1, 0, 0}, {1, 0, 0},
        {0, -1, 0}, {0, 1, 0},
        {0, 0, -1}, {0, 0, 1}
    };

    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];

        // Handle chunk boundaries - assume some light at edges
        if (nx < 0 || nx >= CHUNK_SIZE || nz < 0 || nz >= CHUNK_SIZE) {
            // At chunk edge, assume moderate light
            if (max_light < 8) max_light = 8;
            continue;
        }
        if (ny < 0) continue;
        if (ny >= CHUNK_HEIGHT) {
            // Above chunk = sky = full light
            max_light = LIGHT_MAX;
            continue;
        }

        Block neighbor = chunk->blocks[nx][ny][nz];
        if (neighbor.type == BLOCK_AIR || block_is_transparent(neighbor)) {
            if (neighbor.light_level > max_light) {
                max_light = neighbor.light_level;
            }
        }
    }

    // Ensure minimum ambient light so caves aren't pitch black
    if (max_light < 3) {
        max_light = 3;
    }

    return max_light;
}

/**
 * Simple meshing algorithm - each block face is independent (Luanti-style)
 * Does not merge adjacent faces, rendering each block separately
 */
static void chunk_generate_mesh_simple(Chunk* chunk, float** vertices, float** texcoords,
                                       float** normals, unsigned char** colors, int* vertex_count,
                                       bool transparent_pass) {
    *vertex_count = 0;

    // Iterate through all blocks in the chunk
    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                Block block = chunk_get_block(chunk, x, y, z);

                // Skip air blocks
                if (!block_is_solid(block)) {
                    continue;
                }

                // Two-pass rendering: separate opaque and transparent blocks
                bool is_transparent = block_is_transparent(block);
                if (transparent_pass != is_transparent) {
                    continue;  // Skip blocks not matching this pass
                }

                // Local position of this block within the chunk
                // (chunk offset will be applied via transform matrix during rendering)
                float wx = (float)x;
                float wy = (float)y;
                float wz = (float)z;

                // Get consistent light for this block (same for all faces)
                uint8_t block_light = get_block_light(chunk, x, y, z);

                // Only render faces adjacent to air/transparent blocks (culling)
                // For transparent blocks like leaves, we can see through them so render adjacent faces

                // Face: Top (+Y) - render if neighbor is air or transparent
                Block neighbor_top = (y + 1 < CHUNK_HEIGHT) ? chunk_get_block(chunk, x, y + 1, z) : (Block){BLOCK_AIR, 0, 0};
                if (!block_is_solid(neighbor_top) || block_is_transparent(neighbor_top)) {
                    Vector3 v1 = {wx, wy + 1, wz};
                    Vector3 v2 = {wx + 1, wy + 1, wz};
                    Vector3 v3 = {wx + 1, wy + 1, wz + 1};
                    Vector3 v4 = {wx, wy + 1, wz + 1};
                    Vector3 normal = {0, 1, 0};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, block_light);
                }

                // Face: Bottom (-Y) - render if neighbor is air or transparent
                Block neighbor_bottom = (y - 1 >= 0) ? chunk_get_block(chunk, x, y - 1, z) : (Block){BLOCK_AIR, 0, 0};
                if (!block_is_solid(neighbor_bottom) || block_is_transparent(neighbor_bottom)) {
                    Vector3 v1 = {wx, wy, wz + 1};
                    Vector3 v2 = {wx + 1, wy, wz + 1};
                    Vector3 v3 = {wx + 1, wy, wz};
                    Vector3 v4 = {wx, wy, wz};
                    Vector3 normal = {0, -1, 0};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, block_light);
                }

                // Face: Front (-Z) - render if neighbor is air or transparent
                Block neighbor_front = (z - 1 >= 0) ? chunk_get_block(chunk, x, y, z - 1) : (Block){BLOCK_AIR, 0, 0};
                if (!block_is_solid(neighbor_front) || block_is_transparent(neighbor_front)) {
                    Vector3 v1 = {wx, wy, wz};
                    Vector3 v2 = {wx + 1, wy, wz};
                    Vector3 v3 = {wx + 1, wy + 1, wz};
                    Vector3 v4 = {wx, wy + 1, wz};
                    Vector3 normal = {0, 0, -1};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, block_light);
                }

                // Face: Back (+Z) - render if neighbor is air or transparent
                Block neighbor_back = (z + 1 < CHUNK_SIZE) ? chunk_get_block(chunk, x, y, z + 1) : (Block){BLOCK_AIR, 0, 0};
                if (!block_is_solid(neighbor_back) || block_is_transparent(neighbor_back)) {
                    Vector3 v1 = {wx + 1, wy, wz + 1};
                    Vector3 v2 = {wx, wy, wz + 1};
                    Vector3 v3 = {wx, wy + 1, wz + 1};
                    Vector3 v4 = {wx + 1, wy + 1, wz + 1};
                    Vector3 normal = {0, 0, 1};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, block_light);
                }

                // Face: Left (-X) - render if neighbor is air or transparent
                Block neighbor_left = (x - 1 >= 0) ? chunk_get_block(chunk, x - 1, y, z) : (Block){BLOCK_AIR, 0, 0};
                if (!block_is_solid(neighbor_left) || block_is_transparent(neighbor_left)) {
                    Vector3 v1 = {wx, wy, wz + 1};
                    Vector3 v2 = {wx, wy, wz};
                    Vector3 v3 = {wx, wy + 1, wz};
                    Vector3 v4 = {wx, wy + 1, wz + 1};
                    Vector3 normal = {-1, 0, 0};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, block_light);
                }

                // Face: Right (+X) - render if neighbor is air or transparent
                Block neighbor_right = (x + 1 < CHUNK_SIZE) ? chunk_get_block(chunk, x + 1, y, z) : (Block){BLOCK_AIR, 0, 0};
                if (!block_is_solid(neighbor_right) || block_is_transparent(neighbor_right)) {
                    Vector3 v1 = {wx + 1, wy, wz};
                    Vector3 v2 = {wx + 1, wy, wz + 1};
                    Vector3 v3 = {wx + 1, wy + 1, wz + 1};
                    Vector3 v4 = {wx + 1, wy + 1, wz};
                    Vector3 normal = {1, 0, 0};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, block_light);
                }
            }
        }
    }
}

/**
 * Generate mesh for chunk using simple per-face algorithm
 */
void chunk_generate_mesh(Chunk* chunk) {
    if (!chunk) return;

    // Unload old opaque mesh if it exists
    if (chunk->mesh_generated && chunk->mesh.vboId != NULL) {
        UnloadMesh(chunk->mesh);
        chunk->mesh_generated = false;
        memset(&chunk->mesh, 0, sizeof(Mesh));
    }

    // Unload old transparent mesh if it exists
    if (chunk->transparent_mesh_generated && chunk->transparent_mesh.vboId != NULL) {
        UnloadMesh(chunk->transparent_mesh);
        chunk->transparent_mesh_generated = false;
        memset(&chunk->transparent_mesh, 0, sizeof(Mesh));
    }

    // Skip empty chunks
    if (chunk->is_empty) {
        return;
    }

    // Allocate maximum possible buffer size (worst case: all blocks visible on all 6 sides)
    int max_blocks = CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE;
    int max_vertices = max_blocks * 6 * 6;  // 6 faces * 6 vertices per face

    // === PASS 1: Generate OPAQUE mesh ===
    float* vertices = (float*)malloc(max_vertices * 3 * sizeof(float));
    float* texcoords = (float*)malloc(max_vertices * 2 * sizeof(float));
    float* normals = (float*)malloc(max_vertices * 3 * sizeof(float));
    unsigned char* colors = (unsigned char*)malloc(max_vertices * 4 * sizeof(unsigned char));
    int vertex_count = 0;

    chunk_generate_mesh_simple(chunk, &vertices, &texcoords, &normals, &colors, &vertex_count, false);

    if (vertex_count > 0) {
        chunk->mesh.vertexCount = vertex_count;
        chunk->mesh.triangleCount = vertex_count / 3;
        chunk->mesh.vertices = vertices;
        chunk->mesh.texcoords = texcoords;
        chunk->mesh.normals = normals;
        chunk->mesh.colors = colors;
        UploadMesh(&chunk->mesh, false);
        chunk->mesh_generated = true;
    } else {
        free(vertices);
        free(texcoords);
        free(normals);
        free(colors);
    }

    // === PASS 2: Generate TRANSPARENT mesh ===
    float* trans_vertices = (float*)malloc(max_vertices * 3 * sizeof(float));
    float* trans_texcoords = (float*)malloc(max_vertices * 2 * sizeof(float));
    float* trans_normals = (float*)malloc(max_vertices * 3 * sizeof(float));
    unsigned char* trans_colors = (unsigned char*)malloc(max_vertices * 4 * sizeof(unsigned char));
    int trans_vertex_count = 0;

    chunk_generate_mesh_simple(chunk, &trans_vertices, &trans_texcoords, &trans_normals, &trans_colors, &trans_vertex_count, true);

    if (trans_vertex_count > 0) {
        chunk->transparent_mesh.vertexCount = trans_vertex_count;
        chunk->transparent_mesh.triangleCount = trans_vertex_count / 3;
        chunk->transparent_mesh.vertices = trans_vertices;
        chunk->transparent_mesh.texcoords = trans_texcoords;
        chunk->transparent_mesh.normals = trans_normals;
        chunk->transparent_mesh.colors = trans_colors;
        UploadMesh(&chunk->transparent_mesh, false);
        chunk->transparent_mesh_generated = true;
    } else {
        free(trans_vertices);
        free(trans_texcoords);
        free(trans_normals);
        free(trans_colors);
    }

    chunk->needs_remesh = false;
}

/**
 * Update mesh if needed
 */
void chunk_update_mesh(Chunk* chunk) {
    if (!chunk) return;

    if (chunk->needs_remesh) {
        chunk_generate_mesh(chunk);
    }
}

/**
 * Generate mesh data without GPU upload (for worker threads)
 * Caller must upload the mesh on the main thread using chunk_worker_upload_mesh()
 */
void chunk_generate_mesh_staged(Chunk* chunk, StagedMesh* out) {
    if (!chunk || !out) return;

    // Initialize output
    out->vertices = NULL;
    out->texcoords = NULL;
    out->normals = NULL;
    out->colors = NULL;
    out->vertex_count = 0;
    out->trans_vertices = NULL;
    out->trans_texcoords = NULL;
    out->trans_normals = NULL;
    out->trans_colors = NULL;
    out->trans_vertex_count = 0;
    out->valid = false;

    // Skip empty chunks
    if (chunk->is_empty) {
        out->valid = true;  // Valid but empty
        return;
    }

    // Allocate maximum possible buffer size
    int max_blocks = CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE;
    int max_vertices = max_blocks * 6 * 6;  // 6 faces * 6 vertices per face

    // === PASS 1: Generate OPAQUE mesh ===
    float* vertices = (float*)malloc(max_vertices * 3 * sizeof(float));
    float* texcoords = (float*)malloc(max_vertices * 2 * sizeof(float));
    float* normals = (float*)malloc(max_vertices * 3 * sizeof(float));
    unsigned char* colors = (unsigned char*)malloc(max_vertices * 4 * sizeof(unsigned char));

    if (!vertices || !texcoords || !normals || !colors) {
        if (vertices) free(vertices);
        if (texcoords) free(texcoords);
        if (normals) free(normals);
        if (colors) free(colors);
        return;
    }

    int vertex_count = 0;
    chunk_generate_mesh_simple(chunk, &vertices, &texcoords, &normals, &colors, &vertex_count, false);

    if (vertex_count > 0) {
        out->vertices = vertices;
        out->texcoords = texcoords;
        out->normals = normals;
        out->colors = colors;
        out->vertex_count = vertex_count;
    } else {
        free(vertices);
        free(texcoords);
        free(normals);
        free(colors);
    }

    // === PASS 2: Generate TRANSPARENT mesh ===
    float* trans_vertices = (float*)malloc(max_vertices * 3 * sizeof(float));
    float* trans_texcoords = (float*)malloc(max_vertices * 2 * sizeof(float));
    float* trans_normals = (float*)malloc(max_vertices * 3 * sizeof(float));
    unsigned char* trans_colors = (unsigned char*)malloc(max_vertices * 4 * sizeof(unsigned char));

    if (!trans_vertices || !trans_texcoords || !trans_normals || !trans_colors) {
        if (trans_vertices) free(trans_vertices);
        if (trans_texcoords) free(trans_texcoords);
        if (trans_normals) free(trans_normals);
        if (trans_colors) free(trans_colors);
        out->valid = true;  // Opaque mesh may still be valid
        return;
    }

    int trans_vertex_count = 0;
    chunk_generate_mesh_simple(chunk, &trans_vertices, &trans_texcoords, &trans_normals, &trans_colors, &trans_vertex_count, true);

    if (trans_vertex_count > 0) {
        out->trans_vertices = trans_vertices;
        out->trans_texcoords = trans_texcoords;
        out->trans_normals = trans_normals;
        out->trans_colors = trans_colors;
        out->trans_vertex_count = trans_vertex_count;
    } else {
        free(trans_vertices);
        free(trans_texcoords);
        free(trans_normals);
        free(trans_colors);
    }

    out->valid = true;
}
