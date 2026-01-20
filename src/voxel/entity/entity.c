/**
 * Entity System Implementation
 */

#include "voxel/entity/entity.h"
#include <stdlib.h>
#include <stdio.h>

// ============================================================================
// ENTITY MANAGER
// ============================================================================

EntityManager* entity_manager_create(void) {
    EntityManager* manager = (EntityManager*)malloc(sizeof(EntityManager));
    if (!manager) {
        printf("[ENTITY] Failed to allocate entity manager\n");
        return NULL;
    }

    manager->entities = NULL;
    manager->next_id = 1;  // Start IDs at 1 (0 = invalid)
    manager->entity_count = 0;

    return manager;
}

void entity_manager_destroy(EntityManager* manager) {
    if (!manager) return;

    // Destroy all entities in the linked list
    Entity* current = manager->entities;
    while (current) {
        Entity* next = current->next;
        entity_destroy(current);
        current = next;
    }

    free(manager);
}

// ============================================================================
// ENTITY CREATION/DESTRUCTION
// ============================================================================

Entity* entity_create(EntityType type) {
    Entity* entity = (Entity*)malloc(sizeof(Entity));
    if (!entity) {
        printf("[ENTITY] Failed to allocate entity\n");
        return NULL;
    }

    // Initialize with defaults
    entity->id = 0;  // Will be assigned by manager
    entity->type = type;
    entity->position = (Vector3){0, 0, 0};
    entity->rotation = (Vector3){0, 0, 0};
    entity->velocity = (Vector3){0, 0, 0};
    entity->bbox_min = (Vector3){-0.5f, 0.0f, -0.5f};
    entity->bbox_max = (Vector3){0.5f, 2.0f, 0.5f};
    entity->update = NULL;
    entity->render = NULL;
    entity->destroy_data = NULL;
    entity->data = NULL;
    entity->next = NULL;
    entity->active = true;

    return entity;
}

void entity_destroy(Entity* entity) {
    if (!entity) return;

    // Call type-specific cleanup if provided
    if (entity->destroy_data) {
        entity->destroy_data(entity);
    }

    free(entity);
}

// ============================================================================
// ENTITY MANAGER OPERATIONS
// ============================================================================

void entity_manager_add(EntityManager* manager, Entity* entity) {
    if (!manager || !entity) return;

    // Assign unique ID
    entity->id = manager->next_id++;

    // Add to front of linked list
    entity->next = manager->entities;
    manager->entities = entity;
    manager->entity_count++;
}

void entity_manager_remove(EntityManager* manager, Entity* entity) {
    if (!manager || !entity) return;

    // Find and remove from linked list
    Entity** current = &manager->entities;
    while (*current) {
        if (*current == entity) {
            *current = entity->next;
            manager->entity_count--;
            printf("[ENTITY] Removed entity #%u\n", entity->id);
            return;
        }
        current = &(*current)->next;
    }
}

void entity_manager_update(EntityManager* manager, struct World* world, float dt) {
    if (!manager) return;

    Entity* current = manager->entities;
    while (current) {
        if (current->active && current->update) {
            current->update(current, world, dt);
        }
        current = current->next;
    }
}

void entity_manager_render(EntityManager* manager) {
    if (!manager) return;

    Entity* current = manager->entities;
    while (current) {
        if (current->active && current->render) {
            current->render(current);
        }
        current = current->next;
    }
}

int entity_manager_get_count(EntityManager* manager) {
    return manager ? manager->entity_count : 0;
}
