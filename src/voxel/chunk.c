/**
 * Chunk System Implementation
 */

#include "voxel/chunk.h"
#include "voxel/texture_atlas.h"
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

    chunk->blocks[x][y][z] = block;
    chunk->needs_remesh = true;

    // Update empty flag - check if chunk still has blocks
    if (block.type != BLOCK_AIR) {
        chunk->is_empty = false;
    } else {
        // Quick check: scan for any non-air blocks
        bool has_blocks = false;
        for (int bx = 0; bx < CHUNK_SIZE && !has_blocks; bx++) {
            for (int by = 0; by < CHUNK_HEIGHT && !has_blocks; by++) {
                for (int bz = 0; bz < CHUNK_SIZE && !has_blocks; bz++) {
                    if (chunk->blocks[bx][by][bz].type != BLOCK_AIR) {
                        has_blocks = true;
                    }
                }
            }
        }
        chunk->is_empty = !has_blocks;
    }
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
 * Update the is_empty flag by scanning the chunk
 * Call this after bulk terrain generation
 */
void chunk_update_empty_status(Chunk* chunk) {
    if (!chunk) return;

    bool has_blocks = false;
    for (int x = 0; x < CHUNK_SIZE && !has_blocks; x++) {
        for (int y = 0; y < CHUNK_HEIGHT && !has_blocks; y++) {
            for (int z = 0; z < CHUNK_SIZE && !has_blocks; z++) {
                if (chunk->blocks[x][y][z].type != BLOCK_AIR) {
                    has_blocks = true;
                }
            }
        }
    }

    chunk->is_empty = !has_blocks;
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
                     BlockType block_type, float width, float height) {
    int idx = *vertex_count;

    // Get face type from normal
    BlockFace face = get_face_from_normal(normal);

    // Get texture coordinates from atlas
    TextureCoords tex = texture_atlas_get_coords(block_type, face);

    // Don't scale UVs - just stretch the texture across the greedy quad
    // This prevents bleeding into other atlas tiles

    // Calculate lighting brightness for this face
    float brightness = calculate_face_brightness(normal);
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

                // Render all 6 faces - NO culling (true Luanti-style)
                // Each block is completely independent

                // Face: Top (+Y)
                {
                    Vector3 v1 = {wx, wy + 1, wz};
                    Vector3 v2 = {wx + 1, wy + 1, wz};
                    Vector3 v3 = {wx + 1, wy + 1, wz + 1};
                    Vector3 v4 = {wx, wy + 1, wz + 1};
                    Vector3 normal = {0, 1, 0};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1);
                }

                // Face: Bottom (-Y)
                {
                    Vector3 v1 = {wx, wy, wz + 1};
                    Vector3 v2 = {wx + 1, wy, wz + 1};
                    Vector3 v3 = {wx + 1, wy, wz};
                    Vector3 v4 = {wx, wy, wz};
                    Vector3 normal = {0, -1, 0};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1);
                }

                // Face: Front (-Z)
                {
                    Vector3 v1 = {wx, wy, wz};
                    Vector3 v2 = {wx + 1, wy, wz};
                    Vector3 v3 = {wx + 1, wy + 1, wz};
                    Vector3 v4 = {wx, wy + 1, wz};
                    Vector3 normal = {0, 0, -1};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1);
                }

                // Face: Back (+Z)
                {
                    Vector3 v1 = {wx + 1, wy, wz + 1};
                    Vector3 v2 = {wx, wy, wz + 1};
                    Vector3 v3 = {wx, wy + 1, wz + 1};
                    Vector3 v4 = {wx + 1, wy + 1, wz + 1};
                    Vector3 normal = {0, 0, 1};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1);
                }

                // Face: Left (-X)
                {
                    Vector3 v1 = {wx, wy, wz + 1};
                    Vector3 v2 = {wx, wy, wz};
                    Vector3 v3 = {wx, wy + 1, wz};
                    Vector3 v4 = {wx, wy + 1, wz + 1};
                    Vector3 normal = {-1, 0, 0};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1);
                }

                // Face: Right (+X)
                {
                    Vector3 v1 = {wx + 1, wy, wz};
                    Vector3 v2 = {wx + 1, wy, wz + 1};
                    Vector3 v3 = {wx + 1, wy + 1, wz + 1};
                    Vector3 v4 = {wx + 1, wy + 1, wz};
                    Vector3 normal = {1, 0, 0};
                    add_quad(*vertices, *texcoords, *normals, *colors, vertex_count,
                            v1, v2, v3, v4, normal, block.type, 1, 1);
                }
            }
        }
    }
}

/**
 * Greedy meshing algorithm - merges adjacent identical faces
 * This reduces vertex count significantly for better performance
 */
static void chunk_generate_mesh_greedy(Chunk* chunk, float** vertices, float** texcoords,
                                       float** normals, unsigned char** colors, int* vertex_count) {
    // Temporary mask for greedy meshing (max size: CHUNK_SIZE * CHUNK_HEIGHT)
    bool mask[CHUNK_SIZE * CHUNK_HEIGHT];

    // For each axis (X, Y, Z)
    for (int axis = 0; axis < 3; axis++) {
        int u_axis = (axis + 1) % 3;  // U axis for 2D slice
        int v_axis = (axis + 2) % 3;  // V axis for 2D slice

        int axis_size[3] = {CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE};
        int u_size = axis_size[u_axis];
        int v_size = axis_size[v_axis];

        // For each direction along axis (+1 and -1)
        for (int dir = -1; dir <= 1; dir += 2) {
            // Sweep along the axis
            for (int d = 0; d < axis_size[axis]; d++) {
                // Build mask for this slice
                memset(mask, 0, sizeof(mask));

                for (int u = 0; u < u_size; u++) {
                    for (int v = 0; v < v_size; v++) {
                        // Get block coordinates
                        int pos[3];
                        pos[axis] = d;
                        pos[u_axis] = u;
                        pos[v_axis] = v;

                        int x = pos[0], y = pos[1], z = pos[2];

                        // Check if block is solid
                        if (!chunk_in_bounds(x, y, z)) continue;
                        Block block = chunk->blocks[x][y][z];
                        if (block.type == BLOCK_AIR) continue;

                        // Check neighbor in the direction we're facing
                        int nx = x + (axis == 0 ? dir : 0);
                        int ny = y + (axis == 1 ? dir : 0);
                        int nz = z + (axis == 2 ? dir : 0);

                        // If neighbor is air/transparent, this face is visible
                        if (!has_solid_block_at(chunk, nx, ny, nz)) {
                            mask[u + v * u_size] = true;
                        }
                    }
                }

                // Generate mesh from mask using greedy algorithm
                for (int v = 0; v < v_size; v++) {
                    for (int u = 0; u < u_size; ) {
                        if (!mask[u + v * u_size]) {
                            u++;
                            continue;
                        }

                        // Find width of this quad
                        int width = 1;
                        while (u + width < u_size && mask[u + width + v * u_size]) {
                            width++;
                        }

                        // Find height of this quad
                        int height = 1;
                        bool done = false;
                        while (v + height < v_size && !done) {
                            for (int k = 0; k < width; k++) {
                                if (!mask[u + k + (v + height) * u_size]) {
                                    done = true;
                                    break;
                                }
                            }
                            if (!done) height++;
                        }

                        // Clear the mask for this quad
                        for (int h = 0; h < height; h++) {
                            for (int w = 0; w < width; w++) {
                                mask[u + w + (v + h) * u_size] = false;
                            }
                        }

                        // Create quad vertices
                        int pos[3];
                        pos[axis] = d + (dir > 0 ? 1 : 0);
                        pos[u_axis] = u;
                        pos[v_axis] = v;

                        int du[3] = {0}, dv[3] = {0};
                        du[u_axis] = width;
                        dv[v_axis] = height;

                        Vector3 normal = {0};
                        ((float*)&normal)[axis] = (float)dir;

                        // Get block type from the original quad position (u, v, d)
                        // pos[axis] is the face position, we need the block position
                        int block_pos[3];
                        block_pos[axis] = d;
                        block_pos[u_axis] = u;
                        block_pos[v_axis] = v;

                        Block block = chunk->blocks[block_pos[0]][block_pos[1]][block_pos[2]];
                        BlockType block_type = block.type;

                        // Calculate quad corners
                        Vector3 v1 = {(float)pos[0], (float)pos[1], (float)pos[2]};
                        Vector3 v2 = {(float)(pos[0] + du[0]), (float)(pos[1] + du[1]), (float)(pos[2] + du[2])};
                        Vector3 v3 = {(float)(pos[0] + du[0] + dv[0]), (float)(pos[1] + du[1] + dv[1]), (float)(pos[2] + du[2] + dv[2])};
                        Vector3 v4 = {(float)(pos[0] + dv[0]), (float)(pos[1] + dv[1]), (float)(pos[2] + dv[2])};

                        // Add quad to mesh (respecting winding order based on direction)
                        if (dir > 0) {
                            add_quad(*vertices, *texcoords, *normals, *colors, vertex_count, v1, v2, v3, v4, normal,
                                   block_type, (float)width, (float)height);
                        } else {
                            add_quad(*vertices, *texcoords, *normals, *colors, vertex_count, v1, v4, v3, v2, normal,
                                   block_type, (float)width, (float)height);
                        }

                        u += width;
                    }
                }
            }
        }
    }
}

/**
 * Generate mesh for chunk using greedy meshing algorithm
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

    // Use simple meshing algorithm (each block face is independent, like Luanti)
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
