/**
 * Chest System
 *
 * Handles chest data storage, loot generation, and registry
 */

#ifndef VOXEL_CHEST_H
#define VOXEL_CHEST_H

#include "voxel/core/item.h"
#include <stdbool.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define CHEST_SLOTS 27          // 3 rows x 9 columns
#define CHEST_REGISTRY_SIZE 64  // Hash table size

// ============================================================================
// DATA STRUCTURES
// ============================================================================

typedef struct ChestData {
    int x, y, z;                    // World position
    ItemStack slots[CHEST_SLOTS];   // Inventory contents
    bool loot_generated;            // Has initial loot been created
    struct ChestData* next;         // Hash chain
} ChestData;

typedef struct ChestRegistry {
    ChestData* buckets[CHEST_REGISTRY_SIZE];
    int count;
} ChestRegistry;

// ============================================================================
// API
// ============================================================================

/**
 * Create a new chest registry
 */
ChestRegistry* chest_registry_create(void);

/**
 * Destroy a chest registry and all its data
 */
void chest_registry_destroy(ChestRegistry* registry);

/**
 * Create chest data at world position
 * Returns existing chest if one already exists at position
 */
ChestData* chest_create(ChestRegistry* registry, int x, int y, int z);

/**
 * Get chest at world position (returns NULL if not found)
 */
ChestData* chest_get(ChestRegistry* registry, int x, int y, int z);

/**
 * Remove chest at world position
 */
void chest_remove(ChestRegistry* registry, int x, int y, int z);

/**
 * Generate random dungeon loot for a chest
 * Only generates loot if loot_generated is false
 */
void chest_generate_dungeon_loot(ChestData* chest, unsigned int seed);

/**
 * Check if chest is empty (all slots are ITEM_NONE)
 */
bool chest_is_empty(const ChestData* chest);

/**
 * Try to add an item to chest (first available slot)
 * Returns true if item was added successfully
 */
bool chest_add_item(ChestData* chest, ItemStack item);

/**
 * Take an item from a chest slot
 * Returns the item and clears the slot
 */
ItemStack chest_take_item(ChestData* chest, int slot);

#endif // VOXEL_CHEST_H
