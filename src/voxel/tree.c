/**
 * Tree Generation & Leaf Decay System
 *
 * Generates procedural trees. Leaves decay over time when not connected to wood.
 */

#include "voxel/tree.h"
#include "voxel/block.h"
#include "voxel/biome.h"
#include "voxel/noise.h"
#include "voxel/world.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// ============================================================================
// TREE TEMPLATES
// ============================================================================

typedef struct {
    int8_t dx, dy, dz;  // Offset from trunk base
    BlockType type;      // BLOCK_WOOD or BLOCK_LEAVES
} TreeBlock;

// Small Oak: 5 blocks tall, narrow canopy
static const TreeBlock SMALL_OAK[] = {
    // Trunk (y=0 to y=3)
    {0, 0, 0, BLOCK_WOOD},
    {0, 1, 0, BLOCK_WOOD},
    {0, 2, 0, BLOCK_WOOD},
    {0, 3, 0, BLOCK_WOOD},

    // Leaves layer at y=2 (around trunk)
    {-1, 2, 0, BLOCK_LEAVES}, {1, 2, 0, BLOCK_LEAVES},
    {0, 2, -1, BLOCK_LEAVES}, {0, 2, 1, BLOCK_LEAVES},
    {-1, 2, -1, BLOCK_LEAVES}, {1, 2, -1, BLOCK_LEAVES},
    {-1, 2, 1, BLOCK_LEAVES}, {1, 2, 1, BLOCK_LEAVES},

    // Leaves layer at y=3 (around trunk)
    {-1, 3, 0, BLOCK_LEAVES}, {1, 3, 0, BLOCK_LEAVES},
    {0, 3, -1, BLOCK_LEAVES}, {0, 3, 1, BLOCK_LEAVES},
    {-1, 3, -1, BLOCK_LEAVES}, {1, 3, -1, BLOCK_LEAVES},
    {-1, 3, 1, BLOCK_LEAVES}, {1, 3, 1, BLOCK_LEAVES},

    // Leaves top at y=4
    {0, 4, 0, BLOCK_LEAVES},
    {-1, 4, 0, BLOCK_LEAVES}, {1, 4, 0, BLOCK_LEAVES},
    {0, 4, -1, BLOCK_LEAVES}, {0, 4, 1, BLOCK_LEAVES},
};
#define SMALL_OAK_COUNT (sizeof(SMALL_OAK) / sizeof(TreeBlock))

// Medium Oak: 7 blocks tall, wider canopy
static const TreeBlock MEDIUM_OAK[] = {
    // Trunk (y=0 to y=4)
    {0, 0, 0, BLOCK_WOOD},
    {0, 1, 0, BLOCK_WOOD},
    {0, 2, 0, BLOCK_WOOD},
    {0, 3, 0, BLOCK_WOOD},
    {0, 4, 0, BLOCK_WOOD},

    // Leaves layer at y=3 (wide, 5x5 with corners missing)
    {-2, 3, 0, BLOCK_LEAVES}, {2, 3, 0, BLOCK_LEAVES},
    {0, 3, -2, BLOCK_LEAVES}, {0, 3, 2, BLOCK_LEAVES},
    {-1, 3, 0, BLOCK_LEAVES}, {1, 3, 0, BLOCK_LEAVES},
    {0, 3, -1, BLOCK_LEAVES}, {0, 3, 1, BLOCK_LEAVES},
    {-1, 3, -1, BLOCK_LEAVES}, {1, 3, -1, BLOCK_LEAVES},
    {-1, 3, 1, BLOCK_LEAVES}, {1, 3, 1, BLOCK_LEAVES},
    {-2, 3, -1, BLOCK_LEAVES}, {-2, 3, 1, BLOCK_LEAVES},
    {2, 3, -1, BLOCK_LEAVES}, {2, 3, 1, BLOCK_LEAVES},
    {-1, 3, -2, BLOCK_LEAVES}, {1, 3, -2, BLOCK_LEAVES},
    {-1, 3, 2, BLOCK_LEAVES}, {1, 3, 2, BLOCK_LEAVES},

    // Leaves layer at y=4 (around trunk, 5x5 with corners missing)
    {-2, 4, 0, BLOCK_LEAVES}, {2, 4, 0, BLOCK_LEAVES},
    {0, 4, -2, BLOCK_LEAVES}, {0, 4, 2, BLOCK_LEAVES},
    {-1, 4, 0, BLOCK_LEAVES}, {1, 4, 0, BLOCK_LEAVES},
    {0, 4, -1, BLOCK_LEAVES}, {0, 4, 1, BLOCK_LEAVES},
    {-1, 4, -1, BLOCK_LEAVES}, {1, 4, -1, BLOCK_LEAVES},
    {-1, 4, 1, BLOCK_LEAVES}, {1, 4, 1, BLOCK_LEAVES},
    {-2, 4, -1, BLOCK_LEAVES}, {-2, 4, 1, BLOCK_LEAVES},
    {2, 4, -1, BLOCK_LEAVES}, {2, 4, 1, BLOCK_LEAVES},
    {-1, 4, -2, BLOCK_LEAVES}, {1, 4, -2, BLOCK_LEAVES},
    {-1, 4, 2, BLOCK_LEAVES}, {1, 4, 2, BLOCK_LEAVES},

    // Leaves layer at y=5 (3x3)
    {0, 5, 0, BLOCK_LEAVES},
    {-1, 5, 0, BLOCK_LEAVES}, {1, 5, 0, BLOCK_LEAVES},
    {0, 5, -1, BLOCK_LEAVES}, {0, 5, 1, BLOCK_LEAVES},
    {-1, 5, -1, BLOCK_LEAVES}, {1, 5, -1, BLOCK_LEAVES},
    {-1, 5, 1, BLOCK_LEAVES}, {1, 5, 1, BLOCK_LEAVES},

    // Leaves top at y=6
    {0, 6, 0, BLOCK_LEAVES},
    {-1, 6, 0, BLOCK_LEAVES}, {1, 6, 0, BLOCK_LEAVES},
    {0, 6, -1, BLOCK_LEAVES}, {0, 6, 1, BLOCK_LEAVES},
};
#define MEDIUM_OAK_COUNT (sizeof(MEDIUM_OAK) / sizeof(TreeBlock))

// Large Oak: 9 blocks tall, biggest canopy
static const TreeBlock LARGE_OAK[] = {
    // Trunk (y=0 to y=6)
    {0, 0, 0, BLOCK_WOOD},
    {0, 1, 0, BLOCK_WOOD},
    {0, 2, 0, BLOCK_WOOD},
    {0, 3, 0, BLOCK_WOOD},
    {0, 4, 0, BLOCK_WOOD},
    {0, 5, 0, BLOCK_WOOD},
    {0, 6, 0, BLOCK_WOOD},

    // Leaves layer at y=4 (wide 5x5)
    {-2, 4, 0, BLOCK_LEAVES}, {2, 4, 0, BLOCK_LEAVES},
    {0, 4, -2, BLOCK_LEAVES}, {0, 4, 2, BLOCK_LEAVES},
    {-1, 4, 0, BLOCK_LEAVES}, {1, 4, 0, BLOCK_LEAVES},
    {0, 4, -1, BLOCK_LEAVES}, {0, 4, 1, BLOCK_LEAVES},
    {-1, 4, -1, BLOCK_LEAVES}, {1, 4, -1, BLOCK_LEAVES},
    {-1, 4, 1, BLOCK_LEAVES}, {1, 4, 1, BLOCK_LEAVES},
    {-2, 4, -1, BLOCK_LEAVES}, {-2, 4, 1, BLOCK_LEAVES},
    {2, 4, -1, BLOCK_LEAVES}, {2, 4, 1, BLOCK_LEAVES},
    {-1, 4, -2, BLOCK_LEAVES}, {1, 4, -2, BLOCK_LEAVES},
    {-1, 4, 2, BLOCK_LEAVES}, {1, 4, 2, BLOCK_LEAVES},
    {-2, 4, -2, BLOCK_LEAVES}, {2, 4, -2, BLOCK_LEAVES},
    {-2, 4, 2, BLOCK_LEAVES}, {2, 4, 2, BLOCK_LEAVES},

    // Leaves layer at y=5 (wide 5x5)
    {-2, 5, 0, BLOCK_LEAVES}, {2, 5, 0, BLOCK_LEAVES},
    {0, 5, -2, BLOCK_LEAVES}, {0, 5, 2, BLOCK_LEAVES},
    {-1, 5, 0, BLOCK_LEAVES}, {1, 5, 0, BLOCK_LEAVES},
    {0, 5, -1, BLOCK_LEAVES}, {0, 5, 1, BLOCK_LEAVES},
    {-1, 5, -1, BLOCK_LEAVES}, {1, 5, -1, BLOCK_LEAVES},
    {-1, 5, 1, BLOCK_LEAVES}, {1, 5, 1, BLOCK_LEAVES},
    {-2, 5, -1, BLOCK_LEAVES}, {-2, 5, 1, BLOCK_LEAVES},
    {2, 5, -1, BLOCK_LEAVES}, {2, 5, 1, BLOCK_LEAVES},
    {-1, 5, -2, BLOCK_LEAVES}, {1, 5, -2, BLOCK_LEAVES},
    {-1, 5, 2, BLOCK_LEAVES}, {1, 5, 2, BLOCK_LEAVES},
    {-2, 5, -2, BLOCK_LEAVES}, {2, 5, -2, BLOCK_LEAVES},
    {-2, 5, 2, BLOCK_LEAVES}, {2, 5, 2, BLOCK_LEAVES},

    // Leaves layer at y=6 (around trunk, 5x5)
    {-2, 6, 0, BLOCK_LEAVES}, {2, 6, 0, BLOCK_LEAVES},
    {0, 6, -2, BLOCK_LEAVES}, {0, 6, 2, BLOCK_LEAVES},
    {-1, 6, 0, BLOCK_LEAVES}, {1, 6, 0, BLOCK_LEAVES},
    {0, 6, -1, BLOCK_LEAVES}, {0, 6, 1, BLOCK_LEAVES},
    {-1, 6, -1, BLOCK_LEAVES}, {1, 6, -1, BLOCK_LEAVES},
    {-1, 6, 1, BLOCK_LEAVES}, {1, 6, 1, BLOCK_LEAVES},
    {-2, 6, -1, BLOCK_LEAVES}, {-2, 6, 1, BLOCK_LEAVES},
    {2, 6, -1, BLOCK_LEAVES}, {2, 6, 1, BLOCK_LEAVES},
    {-1, 6, -2, BLOCK_LEAVES}, {1, 6, -2, BLOCK_LEAVES},
    {-1, 6, 2, BLOCK_LEAVES}, {1, 6, 2, BLOCK_LEAVES},

    // Leaves layer at y=7 (3x3)
    {0, 7, 0, BLOCK_LEAVES},
    {-1, 7, 0, BLOCK_LEAVES}, {1, 7, 0, BLOCK_LEAVES},
    {0, 7, -1, BLOCK_LEAVES}, {0, 7, 1, BLOCK_LEAVES},
    {-1, 7, -1, BLOCK_LEAVES}, {1, 7, -1, BLOCK_LEAVES},
    {-1, 7, 1, BLOCK_LEAVES}, {1, 7, 1, BLOCK_LEAVES},

    // Leaves top at y=8
    {0, 8, 0, BLOCK_LEAVES},
    {-1, 8, 0, BLOCK_LEAVES}, {1, 8, 0, BLOCK_LEAVES},
    {0, 8, -1, BLOCK_LEAVES}, {0, 8, 1, BLOCK_LEAVES},
};
#define LARGE_OAK_COUNT (sizeof(LARGE_OAK) / sizeof(TreeBlock))

// Template accessor
static const TreeBlock* get_template(TreeSize size, int* count) {
    switch (size) {
        case TREE_SMALL:
            *count = SMALL_OAK_COUNT;
            return SMALL_OAK;
        case TREE_MEDIUM:
            *count = MEDIUM_OAK_COUNT;
            return MEDIUM_OAK;
        case TREE_LARGE:
            *count = LARGE_OAK_COUNT;
            return LARGE_OAK;
        default:
            *count = SMALL_OAK_COUNT;
            return SMALL_OAK;
    }
}

// ============================================================================
// TREE PLACEMENT
// ============================================================================

void tree_place_at(Chunk* chunk, int local_x, int base_y, int local_z, TreeSize size) {
    if (!chunk) return;

    int count;
    const TreeBlock* template = get_template(size, &count);

    for (int i = 0; i < count; i++) {
        int x = local_x + template[i].dx;
        int y = base_y + template[i].dy;
        int z = local_z + template[i].dz;

        // Check bounds - skip blocks outside chunk
        if (x < 0 || x >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) continue;
        if (y < 0 || y >= CHUNK_HEIGHT) continue;

        // Don't overwrite existing solid blocks (except air)
        Block existing = chunk_get_block(chunk, x, y, z);
        if (existing.type != BLOCK_AIR && existing.type != BLOCK_LEAVES) continue;

        // Create block with metadata=1 for natural tree
        Block block = {
            .type = template[i].type,
            .light_level = 0,
            .metadata = 1  // Mark as natural tree
        };

        chunk_set_block(chunk, x, y, z, block);
    }
}

// ============================================================================
// TREE GENERATION
// ============================================================================

// Find the surface height at a column (checks for any biome surface block)
static int find_surface_y(Chunk* chunk, int x, int z, BlockType surface_block) {
    for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
        Block block = chunk_get_block(chunk, x, y, z);
        if (block.type == surface_block) {
            return y;
        }
        // Stop if we hit other solid blocks (not air)
        if (block.type != BLOCK_AIR && block.type != BLOCK_LEAVES) {
            return -1;  // Not expected surface
        }
    }
    return -1;  // No surface found
}

// Place a cactus at position (1-3 blocks tall)
static void place_cactus(Chunk* chunk, int x, int surface_y, int z) {
    int world_x = chunk->x * CHUNK_SIZE + x;
    int world_z = chunk->z * CHUNK_SIZE + z;

    // Use position hash for height variation
    uint32_t hash = (uint32_t)(world_x * 73856093 ^ world_z * 19349663);
    int height = 1 + (hash % 3);  // 1-3 blocks tall

    for (int i = 0; i < height; i++) {
        int y = surface_y + 1 + i;
        if (y < CHUNK_HEIGHT) {
            Block block = {BLOCK_CACTUS, 0, 0};
            chunk_set_block(chunk, x, y, z, block);
        }
    }
}

void tree_generate_for_chunk(Chunk* chunk) {
    if (!chunk) return;

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            // Calculate world coordinates
            int world_x = chunk->x * CHUNK_SIZE + x;
            int world_z = chunk->z * CHUNK_SIZE + z;

            // Get biome at this position
            BiomeType biome = biome_get_at(world_x, world_z);
            const BiomeProperties* bp = biome_get_properties(biome);

            // Find surface using biome-specific surface block
            int surface_y = find_surface_y(chunk, x, z, bp->surface_block);
            if (surface_y < 0) continue;

            // Vegetation noise (offset to be different from terrain)
            float veg_noise = noise_2d(
                (float)world_x * 0.08f + 5000.0f,
                (float)world_z * 0.08f + 5000.0f
            );

            // Desert biome: place cacti instead of trees
            if (bp->has_cacti) {
                // Cacti are sparse (use different threshold)
                if (veg_noise > 0.75f) {
                    place_cactus(chunk, x, surface_y, z);
                }
                continue;  // No trees in desert
            }

            // Skip if biome doesn't have trees
            if (!bp->has_trees) continue;

            // Calculate tree placement threshold based on biome density
            // Higher density = lower threshold = more trees
            float threshold = 1.0f - bp->tree_density;

            // Only place trees where noise exceeds threshold
            if (veg_noise > threshold) {
                // Check for minimum spacing (skip if another tree trunk nearby)
                bool too_close = false;
                for (int dx = -3; dx <= 3 && !too_close; dx++) {
                    for (int dz = -3; dz <= 3 && !too_close; dz++) {
                        if (dx == 0 && dz == 0) continue;
                        int nx = x + dx;
                        int nz = z + dz;
                        if (nx >= 0 && nx < CHUNK_SIZE && nz >= 0 && nz < CHUNK_SIZE) {
                            // Check if there's wood or cactus near the surface
                            for (int dy = 0; dy <= 3; dy++) {
                                Block neighbor = chunk_get_block(chunk, nx, surface_y + 1 + dy, nz);
                                if (neighbor.type == BLOCK_WOOD || neighbor.type == BLOCK_CACTUS) {
                                    too_close = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (!too_close) {
                    // Pick tree size based on position hash
                    uint32_t hash = (uint32_t)(world_x * 73856093 ^ world_z * 19349663);
                    TreeSize size = (TreeSize)(hash % TREE_SIZE_COUNT);

                    // Place tree (trunk starts above surface)
                    tree_place_at(chunk, x, surface_y + 1, z, size);
                }
            }
        }
    }

    // Update chunk status after placing vegetation
    chunk->needs_remesh = true;
    chunk->is_empty = false;
}

// ============================================================================
// LEAF DECAY SYSTEM
// ============================================================================

#define MAX_DECAY_QUEUE 256
#define LEAF_DECAY_RANGE 4  // Max distance leaves can be from wood

typedef struct {
    int x, y, z;
    float timer;  // Seconds until decay check
} DecayEntry;

static DecayEntry g_decay_queue[MAX_DECAY_QUEUE];
static int g_decay_count = 0;

void leaf_decay_init(void) {
    g_decay_count = 0;
}

// Check if position is already in decay queue
static bool is_in_decay_queue(int x, int y, int z) {
    for (int i = 0; i < g_decay_count; i++) {
        if (g_decay_queue[i].x == x &&
            g_decay_queue[i].y == y &&
            g_decay_queue[i].z == z) {
            return true;
        }
    }
    return false;
}

// Add a leaf to the decay queue
static void add_to_decay_queue(int x, int y, int z) {
    if (g_decay_count >= MAX_DECAY_QUEUE) return;
    if (is_in_decay_queue(x, y, z)) return;

    g_decay_queue[g_decay_count].x = x;
    g_decay_queue[g_decay_count].y = y;
    g_decay_queue[g_decay_count].z = z;
    // Random delay between 0.5 and 2.0 seconds
    g_decay_queue[g_decay_count].timer = 0.5f + ((float)(rand() % 150) / 100.0f);
    g_decay_count++;
}

void leaf_decay_on_wood_removed(struct World* world, int x, int y, int z) {
    if (!world) return;

    // Check all blocks within LEAF_DECAY_RANGE for leaves
    for (int dx = -LEAF_DECAY_RANGE; dx <= LEAF_DECAY_RANGE; dx++) {
        for (int dy = -LEAF_DECAY_RANGE; dy <= LEAF_DECAY_RANGE; dy++) {
            for (int dz = -LEAF_DECAY_RANGE; dz <= LEAF_DECAY_RANGE; dz++) {
                int nx = x + dx;
                int ny = y + dy;
                int nz = z + dz;

                Block block = world_get_block(world, nx, ny, nz);
                if (block.type == BLOCK_LEAVES) {
                    add_to_decay_queue(nx, ny, nz);
                }
            }
        }
    }
}

// Check if a leaf at (x,y,z) is connected to wood within range
static bool leaf_is_supported(struct World* world, int x, int y, int z) {
    // BFS to find wood within LEAF_DECAY_RANGE blocks
    typedef struct { int x, y, z, dist; } SearchNode;
    SearchNode queue[512];
    int queue_head = 0, queue_tail = 0;

    // Simple visited tracking
    #define VISITED_SIZE 512
    int visited_x[VISITED_SIZE];
    int visited_y[VISITED_SIZE];
    int visited_z[VISITED_SIZE];
    int visited_count = 0;

    // Start from the leaf position
    queue[queue_tail++] = (SearchNode){x, y, z, 0};

    while (queue_head < queue_tail) {
        SearchNode node = queue[queue_head++];

        if (node.dist > LEAF_DECAY_RANGE) continue;

        // Check if already visited
        bool already_visited = false;
        for (int i = 0; i < visited_count; i++) {
            if (visited_x[i] == node.x && visited_y[i] == node.y && visited_z[i] == node.z) {
                already_visited = true;
                break;
            }
        }
        if (already_visited) continue;

        // Mark visited
        if (visited_count < VISITED_SIZE) {
            visited_x[visited_count] = node.x;
            visited_y[visited_count] = node.y;
            visited_z[visited_count] = node.z;
            visited_count++;
        }

        Block block = world_get_block(world, node.x, node.y, node.z);

        // Found wood! Leaf is supported
        if (block.type == BLOCK_WOOD) {
            return true;
        }

        // Continue searching through leaves
        if (block.type == BLOCK_LEAVES || (node.x == x && node.y == y && node.z == z)) {
            // Add neighbors
            if (queue_tail < 500) {
                queue[queue_tail++] = (SearchNode){node.x + 1, node.y, node.z, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x - 1, node.y, node.z, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x, node.y + 1, node.z, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x, node.y - 1, node.z, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x, node.y, node.z + 1, node.dist + 1};
                queue[queue_tail++] = (SearchNode){node.x, node.y, node.z - 1, node.dist + 1};
            }
        }
    }

    return false;  // No wood found within range
}

void leaf_decay_update(struct World* world, float dt) {
    if (!world || g_decay_count == 0) return;

    // Process decay queue
    for (int i = g_decay_count - 1; i >= 0; i--) {
        g_decay_queue[i].timer -= dt;

        if (g_decay_queue[i].timer <= 0) {
            int x = g_decay_queue[i].x;
            int y = g_decay_queue[i].y;
            int z = g_decay_queue[i].z;

            // Check if block is still a leaf
            Block block = world_get_block(world, x, y, z);
            if (block.type == BLOCK_LEAVES) {
                // Check if connected to wood
                if (!leaf_is_supported(world, x, y, z)) {
                    // Decay the leaf
                    Block air = {BLOCK_AIR, 0, 0};
                    world_set_block(world, x, y, z, air);

                    // Add nearby leaves to queue (chain reaction)
                    for (int dx = -1; dx <= 1; dx++) {
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dz = -1; dz <= 1; dz++) {
                                if (dx == 0 && dy == 0 && dz == 0) continue;
                                Block neighbor = world_get_block(world, x + dx, y + dy, z + dz);
                                if (neighbor.type == BLOCK_LEAVES) {
                                    add_to_decay_queue(x + dx, y + dy, z + dz);
                                }
                            }
                        }
                    }
                }
            }

            // Remove from queue (swap with last)
            g_decay_queue[i] = g_decay_queue[g_decay_count - 1];
            g_decay_count--;
        }
    }
}
