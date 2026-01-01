/**
 * Chunk Worker System Implementation
 *
 * Multi-threaded chunk generation using pthread
 */

#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <unistd.h>
#include "voxel/world/chunk_worker.h"
#include "voxel/world/terrain.h"
#include "voxel/entity/tree.h"
#include "voxel/render/light.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>

// ============================================================================
// DYNAMIC THREAD COUNT
// ============================================================================

static int get_optimal_thread_count(void) {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) cores = 4;  // Fallback

    int threads = (int)cores - 2;  // Reserve 2 for main thread + OS
    if (threads < WORKER_THREAD_COUNT_MIN) threads = WORKER_THREAD_COUNT_MIN;
    if (threads > WORKER_THREAD_COUNT_MAX) threads = WORKER_THREAD_COUNT_MAX;
    return threads;
}

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

    // Return false if queue is full (non-blocking)
    if (q->count >= TASK_QUEUE_SIZE) {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }

    // Priority-sorted insertion: find position to insert
    // Lower priority value = higher priority (closer to player)
    // Tasks are stored so head has highest priority (lowest value)
    int insert_pos = q->head;
    int items_checked = 0;

    // Find first task with lower priority (higher value) than new task
    while (items_checked < q->count) {
        int idx = (q->head + items_checked) % TASK_QUEUE_SIZE;
        if (q->tasks[idx].priority > task.priority) {
            insert_pos = idx;
            break;
        }
        items_checked++;
    }

    // If we checked all items, insert at tail
    if (items_checked == q->count) {
        q->tasks[q->tail] = task;
    } else {
        // Shift tasks from insert_pos to tail to make room
        int shift_count = q->count - items_checked;
        for (int i = shift_count - 1; i >= 0; i--) {
            int src = (insert_pos + i) % TASK_QUEUE_SIZE;
            int dst = (insert_pos + i + 1) % TASK_QUEUE_SIZE;
            q->tasks[dst] = q->tasks[src];
        }
        q->tasks[insert_pos] = task;
    }

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

    // Determine optimal thread count based on CPU cores
    worker->thread_count = get_optimal_thread_count();

    // Allocate thread array
    worker->threads = (pthread_t*)malloc(worker->thread_count * sizeof(pthread_t));
    if (!worker->threads) {
        printf("[WORKER] Failed to allocate thread array\n");
        free(worker);
        return NULL;
    }

    task_queue_init(&worker->pending);
    worker->completed_head = NULL;
    worker->completed_tail = NULL;
    pthread_mutex_init(&worker->completed_mutex, NULL);
    worker->running = true;

    // Start worker threads
    for (int i = 0; i < worker->thread_count; i++) {
        if (pthread_create(&worker->threads[i], NULL, worker_thread_func, worker) != 0) {
            printf("[WORKER] Failed to create thread %d\n", i);
        }
    }

    printf("[WORKER] Created %d worker threads (detected %ld CPU cores)\n",
           worker->thread_count, sysconf(_SC_NPROCESSORS_ONLN));
    return worker;
}

void chunk_worker_destroy(ChunkWorker* worker) {
    if (!worker) return;

    printf("[WORKER] Shutting down %d threads...\n", worker->thread_count);

    // Signal threads to stop
    worker->running = false;

    // Wake up all waiting threads
    pthread_mutex_lock(&worker->pending.mutex);
    pthread_cond_broadcast(&worker->pending.not_empty);
    pthread_mutex_unlock(&worker->pending.mutex);

    // Wait for threads to finish
    for (int i = 0; i < worker->thread_count; i++) {
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

    // Free thread array
    free(worker->threads);
    free(worker);
    printf("[WORKER] Shutdown complete\n");
}

bool chunk_worker_enqueue(ChunkWorker* worker, Chunk* chunk, TerrainParams params,
                          int center_chunk_x, int center_chunk_z) {
    if (!worker || !chunk) return false;

    // Don't enqueue if already generating or complete
    if (chunk->state != CHUNK_STATE_EMPTY) {
        return false;
    }

    // Calculate priority based on distance² from center (lower = higher priority)
    int dx = chunk->x - center_chunk_x;
    int dz = chunk->z - center_chunk_z;
    int priority = dx * dx + dz * dz;

    ChunkTask task = {
        .chunk = chunk,
        .terrain_params = params,
        .valid = true,
        .priority = priority
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

    // === Upload OPAQUE mesh ===
    if (chunk->mesh_generated && chunk->mesh.vboId != NULL) {
        UnloadMesh(chunk->mesh);
        chunk->mesh_generated = false;
        memset(&chunk->mesh, 0, sizeof(Mesh));
    }

    if (mesh->vertex_count > 0) {
        chunk->mesh.vertexCount = mesh->vertex_count;
        chunk->mesh.triangleCount = mesh->vertex_count / 3;
        chunk->mesh.vertices = mesh->vertices;
        chunk->mesh.texcoords = mesh->texcoords;
        chunk->mesh.normals = mesh->normals;
        chunk->mesh.colors = mesh->colors;
        UploadMesh(&chunk->mesh, false);
        chunk->mesh_generated = true;

        // Don't free buffers - they're now owned by the mesh
        mesh->vertices = NULL;
        mesh->texcoords = NULL;
        mesh->normals = NULL;
        mesh->colors = NULL;
    }

    // === Upload TRANSPARENT mesh ===
    if (chunk->transparent_mesh_generated && chunk->transparent_mesh.vboId != NULL) {
        UnloadMesh(chunk->transparent_mesh);
        chunk->transparent_mesh_generated = false;
        memset(&chunk->transparent_mesh, 0, sizeof(Mesh));
    }

    if (mesh->trans_vertex_count > 0) {
        chunk->transparent_mesh.vertexCount = mesh->trans_vertex_count;
        chunk->transparent_mesh.triangleCount = mesh->trans_vertex_count / 3;
        chunk->transparent_mesh.vertices = mesh->trans_vertices;
        chunk->transparent_mesh.texcoords = mesh->trans_texcoords;
        chunk->transparent_mesh.normals = mesh->trans_normals;
        chunk->transparent_mesh.colors = mesh->trans_colors;
        UploadMesh(&chunk->transparent_mesh, false);
        chunk->transparent_mesh_generated = true;

        // Don't free buffers - they're now owned by the mesh
        mesh->trans_vertices = NULL;
        mesh->trans_texcoords = NULL;
        mesh->trans_normals = NULL;
        mesh->trans_colors = NULL;
    }

    // === Upload LOD OPAQUE mesh ===
    if (chunk->lod_generated && chunk->mesh_lod.vboId != NULL) {
        UnloadMesh(chunk->mesh_lod);
        memset(&chunk->mesh_lod, 0, sizeof(Mesh));
    }

    if (mesh->lod_vertex_count > 0) {
        chunk->mesh_lod.vertexCount = mesh->lod_vertex_count;
        chunk->mesh_lod.triangleCount = mesh->lod_vertex_count / 3;
        chunk->mesh_lod.vertices = mesh->lod_vertices;
        chunk->mesh_lod.texcoords = mesh->lod_texcoords;
        chunk->mesh_lod.normals = mesh->lod_normals;
        chunk->mesh_lod.colors = mesh->lod_colors;
        UploadMesh(&chunk->mesh_lod, false);

        mesh->lod_vertices = NULL;
        mesh->lod_texcoords = NULL;
        mesh->lod_normals = NULL;
        mesh->lod_colors = NULL;
    }

    // === Upload LOD TRANSPARENT mesh ===
    if (chunk->lod_generated && chunk->transparent_mesh_lod.vboId != NULL) {
        UnloadMesh(chunk->transparent_mesh_lod);
        memset(&chunk->transparent_mesh_lod, 0, sizeof(Mesh));
    }

    if (mesh->lod_trans_vertex_count > 0) {
        chunk->transparent_mesh_lod.vertexCount = mesh->lod_trans_vertex_count;
        chunk->transparent_mesh_lod.triangleCount = mesh->lod_trans_vertex_count / 3;
        chunk->transparent_mesh_lod.vertices = mesh->lod_trans_vertices;
        chunk->transparent_mesh_lod.texcoords = mesh->lod_trans_texcoords;
        chunk->transparent_mesh_lod.normals = mesh->lod_trans_normals;
        chunk->transparent_mesh_lod.colors = mesh->lod_trans_colors;
        UploadMesh(&chunk->transparent_mesh_lod, false);

        mesh->lod_trans_vertices = NULL;
        mesh->lod_trans_texcoords = NULL;
        mesh->lod_trans_normals = NULL;
        mesh->lod_trans_colors = NULL;
    }

    // Mark LOD as generated if any LOD mesh was uploaded
    chunk->lod_generated = (mesh->lod_vertex_count > 0 || mesh->lod_trans_vertex_count > 0);

    chunk->needs_remesh = false;
    chunk->state = CHUNK_STATE_COMPLETE;
    mesh->valid = false;
}

void staged_mesh_free(StagedMesh* mesh) {
    if (!mesh) return;

    // Free opaque mesh buffers
    if (mesh->vertices) free(mesh->vertices);
    if (mesh->texcoords) free(mesh->texcoords);
    if (mesh->normals) free(mesh->normals);
    if (mesh->colors) free(mesh->colors);

    // Free transparent mesh buffers
    if (mesh->trans_vertices) free(mesh->trans_vertices);
    if (mesh->trans_texcoords) free(mesh->trans_texcoords);
    if (mesh->trans_normals) free(mesh->trans_normals);
    if (mesh->trans_colors) free(mesh->trans_colors);

    // Free LOD opaque mesh buffers
    if (mesh->lod_vertices) free(mesh->lod_vertices);
    if (mesh->lod_texcoords) free(mesh->lod_texcoords);
    if (mesh->lod_normals) free(mesh->lod_normals);
    if (mesh->lod_colors) free(mesh->lod_colors);

    // Free LOD transparent mesh buffers
    if (mesh->lod_trans_vertices) free(mesh->lod_trans_vertices);
    if (mesh->lod_trans_texcoords) free(mesh->lod_trans_texcoords);
    if (mesh->lod_trans_normals) free(mesh->lod_trans_normals);
    if (mesh->lod_trans_colors) free(mesh->lod_trans_colors);

    mesh->vertices = NULL;
    mesh->texcoords = NULL;
    mesh->normals = NULL;
    mesh->colors = NULL;
    mesh->vertex_count = 0;
    mesh->trans_vertices = NULL;
    mesh->trans_texcoords = NULL;
    mesh->trans_normals = NULL;
    mesh->trans_colors = NULL;
    mesh->trans_vertex_count = 0;
    mesh->lod_vertices = NULL;
    mesh->lod_texcoords = NULL;
    mesh->lod_normals = NULL;
    mesh->lod_colors = NULL;
    mesh->lod_vertex_count = 0;
    mesh->lod_trans_vertices = NULL;
    mesh->lod_trans_texcoords = NULL;
    mesh->lod_trans_normals = NULL;
    mesh->lod_trans_colors = NULL;
    mesh->lod_trans_vertex_count = 0;
    mesh->valid = false;
}

int chunk_worker_pending_count(ChunkWorker* worker) {
    if (!worker) return 0;

    pthread_mutex_lock(&worker->pending.mutex);
    int count = worker->pending.count;
    pthread_mutex_unlock(&worker->pending.mutex);

    return count;
}
