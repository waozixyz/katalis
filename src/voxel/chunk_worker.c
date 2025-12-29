/**
 * Chunk Worker System Implementation
 *
 * Multi-threaded chunk generation using pthread
 */

#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include "voxel/chunk_worker.h"
#include "voxel/terrain.h"
#include "voxel/tree.h"
#include "voxel/light.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

// Defined in chunk.c - generates mesh data without GPU upload
void chunk_generate_mesh_staged(Chunk* chunk, StagedMesh* out);

// ============================================================================
// TASK QUEUE OPERATIONS
// ============================================================================

static void task_queue_init(TaskQueue* q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

static void task_queue_destroy(TaskQueue* q) {
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

static bool task_queue_push(TaskQueue* q, ChunkTask task) {
    pthread_mutex_lock(&q->mutex);

    // Wait if queue is full (with timeout to check running flag)
    while (q->count >= TASK_QUEUE_SIZE) {
        pthread_mutex_unlock(&q->mutex);
        return false;  // Non-blocking: return false if full
    }

    q->tasks[q->tail] = task;
    q->tail = (q->tail + 1) % TASK_QUEUE_SIZE;
    q->count++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

static bool task_queue_pop(TaskQueue* q, ChunkTask* out, bool* running) {
    pthread_mutex_lock(&q->mutex);

    // Wait for task or shutdown
    while (q->count == 0 && *running) {
        // Use timed wait to periodically check running flag
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 100000000;  // 100ms timeout
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        pthread_cond_timedwait(&q->not_empty, &q->mutex, &ts);
    }

    if (q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }

    *out = q->tasks[q->head];
    q->head = (q->head + 1) % TASK_QUEUE_SIZE;
    q->count--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

// ============================================================================
// WORKER THREAD
// ============================================================================

static void* worker_thread_func(void* arg) {
    ChunkWorker* worker = (ChunkWorker*)arg;

    printf("[WORKER] Thread started\n");

    while (worker->running) {
        ChunkTask task;
        if (!task_queue_pop(&worker->pending, &task, &worker->running)) {
            continue;  // Timeout or shutdown
        }

        if (!task.valid || !task.chunk) {
            continue;
        }

        Chunk* chunk = task.chunk;

        // Mark chunk as generating
        chunk->state = CHUNK_STATE_GENERATING;

        // Generate terrain
        terrain_generate_chunk(chunk, task.terrain_params);

        // Update empty status
        chunk_update_empty_status(chunk);

        // Calculate lighting
        light_calculate_chunk(chunk);

        // Generate mesh data (CPU only, no GPU upload)
        StagedMesh mesh = {0};
        chunk_generate_mesh_staged(chunk, &mesh);

        // Add to completed list
        CompletedChunk* completed = (CompletedChunk*)malloc(sizeof(CompletedChunk));
        completed->chunk = chunk;
        completed->mesh = mesh;
        completed->next = NULL;

        pthread_mutex_lock(&worker->completed_mutex);
        if (worker->completed_tail) {
            worker->completed_tail->next = completed;
            worker->completed_tail = completed;
        } else {
            worker->completed_head = completed;
            worker->completed_tail = completed;
        }
        pthread_mutex_unlock(&worker->completed_mutex);

        // Mark chunk as ready for upload
        chunk->state = CHUNK_STATE_READY;
    }

    printf("[WORKER] Thread exiting\n");
    return NULL;
}

// ============================================================================
// PUBLIC API
// ============================================================================

ChunkWorker* chunk_worker_create(void) {
    ChunkWorker* worker = (ChunkWorker*)calloc(1, sizeof(ChunkWorker));
    if (!worker) {
        printf("[WORKER] Failed to allocate worker\n");
        return NULL;
    }

    task_queue_init(&worker->pending);
    worker->completed_head = NULL;
    worker->completed_tail = NULL;
    pthread_mutex_init(&worker->completed_mutex, NULL);
    worker->running = true;

    // Start worker threads
    for (int i = 0; i < WORKER_THREAD_COUNT; i++) {
        if (pthread_create(&worker->threads[i], NULL, worker_thread_func, worker) != 0) {
            printf("[WORKER] Failed to create thread %d\n", i);
        }
    }

    printf("[WORKER] Created %d worker threads\n", WORKER_THREAD_COUNT);
    return worker;
}

void chunk_worker_destroy(ChunkWorker* worker) {
    if (!worker) return;

    printf("[WORKER] Shutting down...\n");

    // Signal threads to stop
    worker->running = false;

    // Wake up all waiting threads
    pthread_mutex_lock(&worker->pending.mutex);
    pthread_cond_broadcast(&worker->pending.not_empty);
    pthread_mutex_unlock(&worker->pending.mutex);

    // Wait for threads to finish
    for (int i = 0; i < WORKER_THREAD_COUNT; i++) {
        pthread_join(worker->threads[i], NULL);
    }

    // Clean up pending tasks
    task_queue_destroy(&worker->pending);

    // Clean up completed list
    pthread_mutex_lock(&worker->completed_mutex);
    CompletedChunk* node = worker->completed_head;
    while (node) {
        CompletedChunk* next = node->next;
        staged_mesh_free(&node->mesh);
        free(node);
        node = next;
    }
    pthread_mutex_unlock(&worker->completed_mutex);
    pthread_mutex_destroy(&worker->completed_mutex);

    free(worker);
    printf("[WORKER] Shutdown complete\n");
}

bool chunk_worker_enqueue(ChunkWorker* worker, Chunk* chunk, TerrainParams params) {
    if (!worker || !chunk) return false;

    // Don't enqueue if already generating or complete
    if (chunk->state != CHUNK_STATE_EMPTY) {
        return false;
    }

    ChunkTask task = {
        .chunk = chunk,
        .terrain_params = params,
        .valid = true
    };

    return task_queue_push(&worker->pending, task);
}

CompletedChunk* chunk_worker_poll_completed(ChunkWorker* worker) {
    if (!worker) return NULL;

    pthread_mutex_lock(&worker->completed_mutex);

    CompletedChunk* result = worker->completed_head;
    if (result) {
        worker->completed_head = result->next;
        if (!worker->completed_head) {
            worker->completed_tail = NULL;
        }
        result->next = NULL;
    }

    pthread_mutex_unlock(&worker->completed_mutex);
    return result;
}

void chunk_worker_upload_mesh(Chunk* chunk, StagedMesh* mesh) {
    if (!chunk || !mesh || !mesh->valid) return;

    // Unload old mesh if it exists
    if (chunk->mesh_generated && chunk->mesh.vboId != NULL) {
        UnloadMesh(chunk->mesh);
        chunk->mesh_generated = false;
        memset(&chunk->mesh, 0, sizeof(Mesh));
    }

    if (mesh->vertex_count == 0) {
        chunk->state = CHUNK_STATE_COMPLETE;
        return;
    }

    // Create Raylib mesh
    chunk->mesh.vertexCount = mesh->vertex_count;
    chunk->mesh.triangleCount = mesh->vertex_count / 3;
    chunk->mesh.vertices = mesh->vertices;
    chunk->mesh.texcoords = mesh->texcoords;
    chunk->mesh.normals = mesh->normals;
    chunk->mesh.colors = mesh->colors;

    // Upload to GPU
    UploadMesh(&chunk->mesh, false);

    chunk->mesh_generated = true;
    chunk->needs_remesh = false;
    chunk->state = CHUNK_STATE_COMPLETE;

    // Don't free buffers - they're now owned by the mesh
    mesh->vertices = NULL;
    mesh->texcoords = NULL;
    mesh->normals = NULL;
    mesh->colors = NULL;
    mesh->valid = false;
}

void staged_mesh_free(StagedMesh* mesh) {
    if (!mesh) return;

    if (mesh->vertices) free(mesh->vertices);
    if (mesh->texcoords) free(mesh->texcoords);
    if (mesh->normals) free(mesh->normals);
    if (mesh->colors) free(mesh->colors);

    mesh->vertices = NULL;
    mesh->texcoords = NULL;
    mesh->normals = NULL;
    mesh->colors = NULL;
    mesh->vertex_count = 0;
    mesh->valid = false;
}

int chunk_worker_pending_count(ChunkWorker* worker) {
    if (!worker) return 0;

    pthread_mutex_lock(&worker->pending.mutex);
    int count = worker->pending.count;
    pthread_mutex_unlock(&worker->pending.mutex);

    return count;
}
