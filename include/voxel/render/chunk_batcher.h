/**
 * Chunk Batcher - Combines multiple chunks into single draw calls
 *
 * Reduces draw calls by merging 2x2 chunk groups into single meshes.
 * At view distance 8: ~289 chunks → ~72 batched meshes (75% reduction)
 */

#ifndef VOXEL_CHUNK_BATCHER_H
#define VOXEL_CHUNK_BATCHER_H

#include <stdbool.h>
#include <raylib.h>
#include "voxel/world/chunk.h"

// Forward declarations
typedef struct World World;

// ============================================================================
// CONFIGURATION
// ============================================================================

#define BATCH_SIZE 2              // 2x2 chunks per batch
#define BATCH_MAX_COUNT 512       // Max batches (supports up to 2048 chunks)
#define BATCH_REBUILDS_PER_FRAME 16  // Max batches to rebuild each frame

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * A batch combines 2x2 chunks into a single mesh for fewer draw calls
 */
typedef struct ChunkBatch {
    int batch_x, batch_z;           // Batch coordinates (chunk coords / 2)
    Mesh opaque_mesh;               // Combined opaque mesh for all 4 chunks
    Mesh transparent_mesh;          // Combined transparent mesh
    bool opaque_valid;              // Opaque mesh uploaded to GPU
    bool transparent_valid;         // Transparent mesh uploaded to GPU
    bool dirty;                     // Needs rebuild
    Chunk* chunks[BATCH_SIZE][BATCH_SIZE];  // References to chunks (may be NULL)
    int chunk_count;                // Number of non-NULL chunks
} ChunkBatch;

/**
 * Hash map node for batch lookup
 */
typedef struct BatchNode {
    ChunkBatch batch;
    struct BatchNode* next;
} BatchNode;

/**
 * Entry for transparent render sorting (can be batch or individual chunk)
 * Pre-allocated in ChunkBatcher to avoid per-frame malloc
 */
typedef struct SortEntry {
    union {
        ChunkBatch* batch;
        Chunk* chunk;
    };
    float dist_sq;
    bool is_batch;  // true=batch, false=chunk
} SortEntry;

/**
 * Chunk batcher system
 */
typedef struct ChunkBatcher {
    BatchNode* buckets[BATCH_MAX_COUNT];
    int batch_count;
    int dirty_count;                // Number of batches needing rebuild
    SortEntry* sort_buffer;         // Pre-allocated buffer for transparent sorting
    int sort_buffer_capacity;       // Size of sort buffer
} ChunkBatcher;

// ============================================================================
// API
// ============================================================================

/**
 * Create a new chunk batcher
 */
ChunkBatcher* chunk_batcher_create(void);

/**
 * Destroy the chunk batcher and free all batches
 */
void chunk_batcher_destroy(ChunkBatcher* batcher);

/**
 * Get or create batch for a chunk at given chunk coordinates
 * Returns the batch that contains this chunk
 */
ChunkBatch* chunk_batcher_get_batch(ChunkBatcher* batcher, int chunk_x, int chunk_z);

/**
 * Register a chunk with its batch
 * Call this when a chunk is created/loaded
 */
void chunk_batcher_register_chunk(ChunkBatcher* batcher, Chunk* chunk);

/**
 * Unregister a chunk from its batch
 * Call this when a chunk is destroyed/unloaded
 */
void chunk_batcher_unregister_chunk(ChunkBatcher* batcher, Chunk* chunk);

/**
 * Mark the batch containing this chunk as needing rebuild
 * Call this when a chunk's mesh changes
 */
void chunk_batcher_invalidate(ChunkBatcher* batcher, int chunk_x, int chunk_z);

/**
 * Rebuild dirty batches (call once per frame)
 * @param max_rebuilds Maximum batches to rebuild per frame (0 = use default)
 */
void chunk_batcher_update(ChunkBatcher* batcher, int max_rebuilds);

/**
 * Render all batched opaque meshes
 * camera_pos and center_chunk for frustum/distance culling
 */
void chunk_batcher_render_opaque(ChunkBatcher* batcher, World* world,
                                  Material material, Vector3 camera_pos);

/**
 * Render all batched transparent meshes (call after opaque, sorted back-to-front)
 */
void chunk_batcher_render_transparent(ChunkBatcher* batcher, World* world,
                                       Material material, Vector3 camera_pos);

/**
 * Get batch coordinates from chunk coordinates
 */
static inline void chunk_to_batch_coords(int chunk_x, int chunk_z, int* batch_x, int* batch_z) {
    // Use floor division to handle negative coordinates correctly
    *batch_x = (chunk_x >= 0) ? (chunk_x / BATCH_SIZE) : ((chunk_x - BATCH_SIZE + 1) / BATCH_SIZE);
    *batch_z = (chunk_z >= 0) ? (chunk_z / BATCH_SIZE) : ((chunk_z - BATCH_SIZE + 1) / BATCH_SIZE);
}

#endif // VOXEL_CHUNK_BATCHER_H
