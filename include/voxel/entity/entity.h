/**
 * Entity System - Core entity management
 *
 * Provides a generic entity system for managing game entities (NPCs, mobs, etc.)
 * Uses polymorphic behavior via function pointers for extensibility.
 */

#ifndef ENTITY_H
#define ENTITY_H

#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
// Note: World typedef is defined in world.h - we just declare the struct here
struct World;
typedef struct Entity Entity;

// ============================================================================
// ENTITY TYPES
// ============================================================================

typedef enum {
    ENTITY_TYPE_NONE = 0,
    ENTITY_TYPE_BLOCK_HUMAN,
    ENTITY_TYPE_SHEEP,
    ENTITY_TYPE_PIG,
    // Future entity types:
    // ENTITY_TYPE_ZOMBIE,
    // ENTITY_TYPE_ITEM_DROP,
} EntityType;

// ============================================================================
// ENTITY ID SYSTEM
// ============================================================================

typedef uint32_t EntityId;

// ============================================================================
// ENTITY CALLBACKS (Polymorphic Behavior)
// ============================================================================

/**
 * Update callback - called every frame for active entities
 * @param entity The entity to update
 * @param world World reference (for collision, etc.)
 * @param dt Delta time in seconds
 */
typedef void (*EntityUpdateFunc)(Entity* entity, struct World* world, float dt);

/**
 * Render callback - called to draw the entity
 * @param entity The entity to render
 */
typedef void (*EntityRenderFunc)(Entity* entity);

/**
 * Destroy callback - called to cleanup type-specific data
 * @param entity The entity being destroyed
 */
typedef void (*EntityDestroyFunc)(Entity* entity);

// ============================================================================
// ENTITY STRUCTURE
// ============================================================================

struct Entity {
    // Identity
    EntityId id;                        // Unique identifier
    EntityType type;                    // Entity type

    // Transform
    Vector3 position;                   // World position (at feet/base)
    Vector3 rotation;                   // Euler angles (pitch, yaw, roll)
    Vector3 velocity;                   // Physics velocity

    // Bounding box (for collision)
    Vector3 bbox_min;                   // Relative to position
    Vector3 bbox_max;                   // Relative to position

    // Polymorphic behavior
    EntityUpdateFunc update;            // Update function pointer
    EntityRenderFunc render;            // Render function pointer
    EntityDestroyFunc destroy_data;     // Cleanup function pointer

    // Type-specific data
    void* data;                         // Points to BlockHumanData, etc.

    // Linked list management
    struct Entity* next;                // Next entity in list

    // State
    bool active;                        // Is entity alive/active?
};

// ============================================================================
// ENTITY MANAGER
// ============================================================================

typedef struct EntityManager {
    Entity* entities;                   // Linked list head
    EntityId next_id;                   // ID counter
    int entity_count;                   // Number of active entities
} EntityManager;

// ============================================================================
// ENTITY MANAGER API
// ============================================================================

/**
 * Create a new entity manager
 * @return Pointer to new entity manager (must be freed with entity_manager_destroy)
 */
EntityManager* entity_manager_create(void);

/**
 * Destroy entity manager and all entities
 * @param manager Entity manager to destroy
 */
void entity_manager_destroy(EntityManager* manager);

/**
 * Create a new entity (generic)
 * @param type Entity type
 * @return Pointer to new entity (must be added to manager)
 */
Entity* entity_create(EntityType type);

/**
 * Destroy a single entity
 * @param entity Entity to destroy
 */
void entity_destroy(Entity* entity);

/**
 * Add entity to manager
 * @param manager Entity manager
 * @param entity Entity to add
 */
void entity_manager_add(EntityManager* manager, Entity* entity);

/**
 * Remove entity from manager (does not destroy it)
 * @param manager Entity manager
 * @param entity Entity to remove
 */
void entity_manager_remove(EntityManager* manager, Entity* entity);

/**
 * Update all entities
 * @param manager Entity manager
 * @param world World reference
 * @param dt Delta time in seconds
 */
void entity_manager_update(EntityManager* manager, struct World* world, float dt);

/**
 * Render all entities
 * @param manager Entity manager
 */
void entity_manager_render(EntityManager* manager);

/**
 * Get entity count
 * @param manager Entity manager
 * @return Number of active entities
 */
int entity_manager_get_count(EntityManager* manager);

#endif // ENTITY_H
