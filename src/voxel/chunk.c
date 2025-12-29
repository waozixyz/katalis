/**
 * Chunk System Implementation
 */

#include "voxel/chunk.h"
#include "voxel/texture_atlas.h"
#include "voxel/light.h"
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
    chunk->solid_block_count = 0;

    // Initialize all blocks to air
    memset(chunk->blocks, 0, sizeof(chunk->blocks));

    // Initialize mesh to zero
    memset(&chunk->mesh, 0, sizeof(Mesh));

    return chunk;
}

/**
 * Destroy chunk
 */
void chunk_destroy(Chunk* chunk) {
    if (!chunk) return;

    // Unload mesh from GPU if it was generated
    if (chunk->mesh_generated && chunk->mesh.vboId != NULL) {
        UnloadMesh(chunk->mesh);
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
 * Get maximum light from all adjacent air/transparent blocks.
 * Used when a face is adjacent to a solid block to find nearby light.
 */
static uint8_t get_max_adjacent_light(Chunk* chunk, int x, int y, int z) {
    static const int offsets[6][3] = {
        {-1, 0, 0}, {1, 0, 0},
        {0, -1, 0}, {0, 1, 0},
        {0, 0, -1}, {0, 0, 1}
    };

    uint8_t max_light = 0;

    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];

        // Skip out of bounds
        if (nx < 0 || nx >= CHUNK_SIZE || nz < 0 || nz >= CHUNK_SIZE) continue;
        if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

        Block neighbor = chunk->blocks[nx][ny][nz];
        if (neighbor.type == BLOCK_AIR || block_is_transparent(neighbor)) {
            if (neighbor.light_level > max_light) {
                max_light = neighbor.light_level;
            }
        }
    }

    return max_light;
}

/**
 * Get light level for a block face by checking the adjacent block.
 * If adjacent block is air/transparent, use its light level.
 * Otherwise use max light from any adjacent air block for uniform lighting.
 */
static uint8_t get_face_light(Chunk* chunk, int x, int y, int z, int dx, int dy, int dz) {
    int nx = x + dx;
    int ny = y + dy;
    int nz = z + dz;

    // Check bounds - if outside chunk, use max adjacent light
    if (nx < 0 || nx >= CHUNK_SIZE || nz < 0 || nz >= CHUNK_SIZE) {
        return get_max_adjacent_light(chunk, x, y, z);
    }
    if (ny < 0) return LIGHT_MIN;
    if (ny >= CHUNK_HEIGHT) return LIGHT_MAX;

    Block neighbor = chunk->blocks[nx][ny][nz];

    // If neighbor is air or transparent, use its light level
    if (neighbor.type == BLOCK_AIR || block_is_transparent(neighbor)) {
        return neighbor.light_level;
    }

    // Adjacent to solid block - use max light from any adjacent air
    // This prevents dark faces on tree trunks and similar structures
    return get_max_adjacent_light(chunk, x, y, z);
}

/**
 * Simple meshing algorithm - each block face is independent (Luanti-style)
 * Does not merge adjacent faces, rendering each block separately
 */
static void chunk_generate_mesh_simple(Chunk* chunk, float** vertices, float** texcoords,
                                       float** normals, unsigned char** colors, int* vertex_count) {
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

                // Local position of this block within the chunk
                // (chunk offset will be applied via transform matrix during rendering)
                float wx = (float)x;
                float wy = (float)y;
                float wz = (float)z;

                // Render all 6 faces - each face uses the light level of the adjacent block
                // This makes faces lit based on the light in the air next to them

                // Face: Top (+Y) - use light from block above
                {
                    Vector3 v1 = {wx, wy + 1, wz};
                    Vector3 v2 = {wx + 1, wy + 1, wz};
                    Vector3 v3 = {wx + 1, wy + 1, wz + 1};
                    Vector3 v4 = {wx, wy + 1, wz + 1};
                    Vector3 normal = {0, 1, 0};
                    uint8_t face_light = get_face_light(chunk, x, y, z, 0, 1, 0);
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, face_light);
                }

                // Face: Bottom (-Y) - use light from block below
                {
                    Vector3 v1 = {wx, wy, wz + 1};
                    Vector3 v2 = {wx + 1, wy, wz + 1};
                    Vector3 v3 = {wx + 1, wy, wz};
                    Vector3 v4 = {wx, wy, wz};
                    Vector3 normal = {0, -1, 0};
                    uint8_t face_light = get_face_light(chunk, x, y, z, 0, -1, 0);
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, face_light);
                }

                // Face: Front (-Z) - use light from block in front
                {
                    Vector3 v1 = {wx, wy, wz};
                    Vector3 v2 = {wx + 1, wy, wz};
                    Vector3 v3 = {wx + 1, wy + 1, wz};
                    Vector3 v4 = {wx, wy + 1, wz};
                    Vector3 normal = {0, 0, -1};
                    uint8_t face_light = get_face_light(chunk, x, y, z, 0, 0, -1);
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, face_light);
                }

                // Face: Back (+Z) - use light from block behind
                {
                    Vector3 v1 = {wx + 1, wy, wz + 1};
                    Vector3 v2 = {wx, wy, wz + 1};
                    Vector3 v3 = {wx, wy + 1, wz + 1};
                    Vector3 v4 = {wx + 1, wy + 1, wz + 1};
                    Vector3 normal = {0, 0, 1};
                    uint8_t face_light = get_face_light(chunk, x, y, z, 0, 0, 1);
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, face_light);
                }

                // Face: Left (-X) - use light from block to the left
                {
                    Vector3 v1 = {wx, wy, wz + 1};
                    Vector3 v2 = {wx, wy, wz};
                    Vector3 v3 = {wx, wy + 1, wz};
                    Vector3 v4 = {wx, wy + 1, wz + 1};
                    Vector3 normal = {-1, 0, 0};
                    uint8_t face_light = get_face_light(chunk, x, y, z, -1, 0, 0);
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, face_light);
                }

                // Face: Right (+X) - use light from block to the right
                {
                    Vector3 v1 = {wx + 1, wy, wz};
                    Vector3 v2 = {wx + 1, wy, wz + 1};
                    Vector3 v3 = {wx + 1, wy + 1, wz + 1};
                    Vector3 v4 = {wx + 1, wy + 1, wz};
                    Vector3 normal = {1, 0, 0};
                    uint8_t face_light = get_face_light(chunk, x, y, z, 1, 0, 0);
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1, face_light);
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


    // Unload old mesh if it exists
    if (chunk->mesh_generated && chunk->mesh.vboId != NULL) {
        UnloadMesh(chunk->mesh);
        chunk->mesh_generated = false;
        // Clear the mesh struct to avoid GPU ID conflicts
        memset(&chunk->mesh, 0, sizeof(Mesh));
    }

    // Skip empty chunks
    if (chunk->is_empty) {
        return;
    }

    // Allocate maximum possible buffer size (worst case: all blocks visible on all 6 sides)
    int max_blocks = CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE;
    int max_vertices = max_blocks * 6 * 6;  // 6 faces * 6 vertices per face
    float* vertices = (float*)malloc(max_vertices * 3 * sizeof(float));
    float* texcoords = (float*)malloc(max_vertices * 2 * sizeof(float));
    float* normals = (float*)malloc(max_vertices * 3 * sizeof(float));
    unsigned char* colors = (unsigned char*)malloc(max_vertices * 4 * sizeof(unsigned char));

    int vertex_count = 0;

    // Use simple meshing algorithm (each block face is independent)
    chunk_generate_mesh_simple(chunk, &vertices, &texcoords, &normals, &colors, &vertex_count);

    if (vertex_count == 0) {
        free(vertices);
        free(texcoords);
        free(normals);
        free(colors);
        return;
    }

    // Create Raylib mesh
    chunk->mesh.vertexCount = vertex_count;
    chunk->mesh.triangleCount = vertex_count / 3;
    chunk->mesh.vertices = vertices;
    chunk->mesh.texcoords = texcoords;
    chunk->mesh.normals = normals;
    chunk->mesh.colors = colors;

    // Upload to GPU
    UploadMesh(&chunk->mesh, false);
    chunk->mesh_generated = true;
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
