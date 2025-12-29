/**
 * Light System Implementation
 *
 * Skylight propagation from sky downward, then spreads horizontally.
 * Creates realistic gradual falloff in caves and tunnels.
 */

#include "voxel/light.h"
#include "voxel/block.h"
#include <stdio.h>
#include <string.h>

/**
 * Calculate initial skylight for a single column (x, z)
 * This is the first pass - direct sunlight from above
 */
static void calculate_column_skylight(Chunk* chunk, int x, int z) {
    int light = LIGHT_MAX;  // Start at full skylight from sky

    // Scan from top to bottom
    for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
        Block* block = &chunk->blocks[x][y][z];

        if (block->type == BLOCK_AIR) {
            // Air: full light passes through unchanged
            block->light_level = light;
        } else if (block_is_transparent(*block)) {
            // Transparent blocks (leaves, water): light passes through but reduced
            block->light_level = light;
            if (light > 0) light--;
        } else {
            // Solid block: gets current light level on top surface, then blocks all light
            block->light_level = light;
            light = 0;
        }
    }
}

/**
 * Propagate light horizontally through the chunk.
 * Light spreads to adjacent air blocks with -1 reduction per step.
 * Uses iterative passes until no more changes occur.
 */
static void propagate_light(Chunk* chunk) {
    // Direction offsets for 6 neighbors (including up/down for cave openings)
    static const int dx[] = {-1, 1, 0, 0, 0, 0};
    static const int dy[] = {0, 0, -1, 1, 0, 0};
    static const int dz[] = {0, 0, 0, 0, -1, 1};

    bool changed = true;
    int iterations = 0;
    const int max_iterations = 16;  // Limit iterations (light can only travel 15 blocks max)

    while (changed && iterations < max_iterations) {
        changed = false;
        iterations++;

        // Scan all blocks
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    Block* block = &chunk->blocks[x][y][z];

                    // Only propagate through air and transparent blocks
                    if (block->type != BLOCK_AIR && !block_is_transparent(*block)) {
                        continue;
                    }

                    // Check all 6 neighbors
                    for (int i = 0; i < 6; i++) {
                        int nx = x + dx[i];
                        int ny = y + dy[i];
                        int nz = z + dz[i];

                        // Bounds check
                        if (nx < 0 || nx >= CHUNK_SIZE) continue;
                        if (ny < 0 || ny >= CHUNK_HEIGHT) continue;
                        if (nz < 0 || nz >= CHUNK_SIZE) continue;

                        Block* neighbor = &chunk->blocks[nx][ny][nz];

                        // If neighbor has more light, we can receive some
                        if (neighbor->light_level > block->light_level + 1) {
                            block->light_level = neighbor->light_level - 1;
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}

/**
 * Calculate skylight for entire chunk with horizontal propagation
 */
void light_calculate_chunk(Chunk* chunk) {
    if (!chunk) return;

    // Pass 1: Calculate direct skylight from above
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            calculate_column_skylight(chunk, x, z);
        }
    }

    // Pass 2: Propagate light horizontally through caves/tunnels
    propagate_light(chunk);

    // Mark chunk as needing mesh regeneration
    chunk->needs_remesh = true;
}

/**
 * Recalculate light for a single column and repropagate
 */
void light_update_column(Chunk* chunk, int x, int z) {
    if (!chunk) return;
    if (x < 0 || x >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) return;

    calculate_column_skylight(chunk, x, z);
    propagate_light(chunk);
    chunk->needs_remesh = true;
}
