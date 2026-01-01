/**
 * Chunk Worker System - Multi-threaded chunk generation
 *
 * Offloads terrain and mesh generation to worker threads
 * to eliminate frame stutters when loading new areas.
 */

#ifndef VOXEL_CHUNK_WORKER_H
#define VOXEL_CHUNK_WORKER_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include "voxel/world/chunk.h"
#include "voxel/world/terrain.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

#define WORKER_THREAD_COUNT_MIN 2
#define WORKER_THREAD_COUNT_MAX 16
#define TASK_QUEUE_SIZE 512
#define MAX_UPLOADS_PER_FRAME 32
#define LOD_DISTANCE_THRESHOLD 8  // Chunks beyond this distance use LOD mesh

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Staged mesh data - generated on worker thread, uploaded on main thread
 */
typedef struct {
    // Opaque mesh (full detail)
    float* vertices;
    float* texcoords;
    float* normals;
    unsigned char* colors;
    int vertex_count;
    // Transparent mesh (full detail)
    float* trans_vertices;
    float* trans_texcoords;
    float* trans_normals;
    unsigned char* trans_colors;
    int trans_vertex_count;
    // LOD opaque mesh (simplified for distant chunks)
    float* lod_vertices;
    float* lod_texcoords;
    float* lod_normals;
    unsigned char* lod_colors;
    int lod_vertex_count;
    // LOD transparent mesh
    float* lod_trans_vertices;
    float* lod_trans_texcoords;
    float* lod_trans_normals;
    unsigned char* lod_trans_colors;
    int lod_trans_vertex_count;
    bool valid;
} StagedMesh;

/**
 * Task for worker threads
 */
typedef struct {
    Chunk* chunk;
    TerrainParams terrain_params;
    bool valid;
    int priority;  // Lower value = higher priority (distance² from player)
} ChunkTask;

/**
 * Thread-safe task queue
 */
typedef struct {
    ChunkTask tasks[TASK_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} TaskQueue;

/**
 * Completed chunk with staged mesh (linked list node)
 */
typedef struct CompletedChunk {
    Chunk* chunk;
    StagedMesh mesh;
    struct CompletedChunk* next;
} CompletedChunk;

/**
 * Worker system
 */
typedef struct ChunkWorker {
    pthread_t* threads;              // Dynamic array of worker threads
    int thread_count;                // Number of active worker threads
    TaskQueue pending;
    CompletedChunk* completed_head;
    CompletedChunk* completed_tail;
    pthread_mutex_t completed_mutex;
    bool running;
} ChunkWorker;

// ============================================================================
// API
// ============================================================================

/**
 * Create and start the chunk worker system
 */
ChunkWorker* chunk_worker_create(void);

/**
 * Stop and destroy the chunk worker system
 */
void chunk_worker_destroy(ChunkWorker* worker);

/**
 * Enqueue a chunk for generation (non-blocking)
 * Priority is calculated based on distance from center_chunk position
 * Returns true if successfully enqueued
 */
bool chunk_worker_enqueue(ChunkWorker* worker, Chunk* chunk, TerrainParams params,
                          int center_chunk_x, int center_chunk_z);

/**
 * Poll for completed chunks (non-blocking)
 * Returns NULL if no chunks are ready
 * Caller is responsible for freeing the returned CompletedChunk
 */
CompletedChunk* chunk_worker_poll_completed(ChunkWorker* worker);

/**
 * Upload staged mesh to GPU (must be called from main thread)
 */
void chunk_worker_upload_mesh(Chunk* chunk, StagedMesh* mesh);

/**
 * Free staged mesh data
 */
void staged_mesh_free(StagedMesh* mesh);

/**
 * Get number of pending tasks
 */
int chunk_worker_pending_count(ChunkWorker* worker);

#endif // VOXEL_CHUNK_WORKER_H
