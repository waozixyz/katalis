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
#include "voxel/chunk.h"
#include "voxel/terrain.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

#define WORKER_THREAD_COUNT 4
#define TASK_QUEUE_SIZE 128
#define MAX_UPLOADS_PER_FRAME 3

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Staged mesh data - generated on worker thread, uploaded on main thread
 */
typedef struct {
    float* vertices;
    float* texcoords;
    float* normals;
    unsigned char* colors;
    int vertex_count;
    bool valid;
} StagedMesh;

/**
 * Task for worker threads
 */
typedef struct {
    Chunk* chunk;
    TerrainParams terrain_params;
    bool valid;
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
typedef struct {
    pthread_t threads[WORKER_THREAD_COUNT];
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
 * Returns true if successfully enqueued
 */
bool chunk_worker_enqueue(ChunkWorker* worker, Chunk* chunk, TerrainParams params);

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
