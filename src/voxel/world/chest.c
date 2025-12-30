/**
 * Chest System Implementation
 */

#include "voxel/world/chest.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// LOOT TABLES
// ============================================================================

typedef struct {
    ItemType item;
    int min_count;
    int max_count;
    float chance;  // 0.0 to 1.0
} LootEntry;

// Dungeon loot table - mix of common and rare items
static const LootEntry DUNGEON_LOOT[] = {
    // Common items
    {ITEM_COBBLESTONE,    4, 12, 1.0f},   // Always some cobblestone
    {ITEM_WOOD_LOG,       2,  6, 0.7f},   // Often wood
    {ITEM_STICK,          2,  8, 0.6f},   // Sticks
    {ITEM_MEAT,           1,  4, 0.5f},   // Food

    // Tools (rarer)
    {ITEM_STONE_PICKAXE,  1,  1, 0.25f},  // Stone pickaxe
    {ITEM_STONE_AXE,      1,  1, 0.20f},  // Stone axe
    {ITEM_STONE_SHOVEL,   1,  1, 0.20f},  // Stone shovel

    // Rare materials
    {ITEM_WOOD_PLANKS,    4,  8, 0.4f},   // Planks
};

#define DUNGEON_LOOT_COUNT (sizeof(DUNGEON_LOOT) / sizeof(LootEntry))

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Simple hash function for chest positions
 */
static unsigned int chest_hash(int x, int y, int z) {
    unsigned int h = 2166136261u;
    h = (h ^ (unsigned int)x) * 16777619u;
    h = (h ^ (unsigned int)y) * 16777619u;
    h = (h ^ (unsigned int)z) * 16777619u;
    return h % CHEST_REGISTRY_SIZE;
}

/**
 * Simple deterministic random from seed
 */
static float random_from_seed(unsigned int* seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return (float)*seed / (float)0x7FFFFFFF;
}

// ============================================================================
// REGISTRY MANAGEMENT
// ============================================================================

ChestRegistry* chest_registry_create(void) {
    ChestRegistry* registry = (ChestRegistry*)malloc(sizeof(ChestRegistry));
    if (!registry) return NULL;

    memset(registry->buckets, 0, sizeof(registry->buckets));
    registry->count = 0;

    printf("[CHEST] Registry created\n");
    return registry;
}

void chest_registry_destroy(ChestRegistry* registry) {
    if (!registry) return;

    // Free all chest data
    for (int i = 0; i < CHEST_REGISTRY_SIZE; i++) {
        ChestData* chest = registry->buckets[i];
        while (chest) {
            ChestData* next = chest->next;
            free(chest);
            chest = next;
        }
    }

    free(registry);
    printf("[CHEST] Registry destroyed\n");
}

ChestData* chest_create(ChestRegistry* registry, int x, int y, int z) {
    if (!registry) return NULL;

    // Check if chest already exists
    ChestData* existing = chest_get(registry, x, y, z);
    if (existing) return existing;

    // Create new chest
    ChestData* chest = (ChestData*)malloc(sizeof(ChestData));
    if (!chest) return NULL;

    chest->x = x;
    chest->y = y;
    chest->z = z;
    chest->loot_generated = false;

    // Clear all slots
    for (int i = 0; i < CHEST_SLOTS; i++) {
        chest->slots[i] = (ItemStack){ITEM_NONE, 0, 0, 0};
    }

    // Insert into hash table
    unsigned int hash = chest_hash(x, y, z);
    chest->next = registry->buckets[hash];
    registry->buckets[hash] = chest;
    registry->count++;

    return chest;
}

ChestData* chest_get(ChestRegistry* registry, int x, int y, int z) {
    if (!registry) return NULL;

    unsigned int hash = chest_hash(x, y, z);
    ChestData* chest = registry->buckets[hash];

    while (chest) {
        if (chest->x == x && chest->y == y && chest->z == z) {
            return chest;
        }
        chest = chest->next;
    }

    return NULL;
}

void chest_remove(ChestRegistry* registry, int x, int y, int z) {
    if (!registry) return;

    unsigned int hash = chest_hash(x, y, z);
    ChestData* chest = registry->buckets[hash];
    ChestData* prev = NULL;

    while (chest) {
        if (chest->x == x && chest->y == y && chest->z == z) {
            if (prev) {
                prev->next = chest->next;
            } else {
                registry->buckets[hash] = chest->next;
            }
            free(chest);
            registry->count--;
            return;
        }
        prev = chest;
        chest = chest->next;
    }
}

// ============================================================================
// LOOT GENERATION
// ============================================================================

void chest_generate_dungeon_loot(ChestData* chest, unsigned int seed) {
    if (!chest || chest->loot_generated) return;

    chest->loot_generated = true;

    int slot = 0;

    // Roll for each loot entry
    for (size_t i = 0; i < DUNGEON_LOOT_COUNT && slot < CHEST_SLOTS; i++) {
        const LootEntry* entry = &DUNGEON_LOOT[i];

        // Check if this item appears
        if (random_from_seed(&seed) < entry->chance) {
            // Determine count
            int range = entry->max_count - entry->min_count + 1;
            int count = entry->min_count;
            if (range > 1) {
                count += (int)(random_from_seed(&seed) * range);
            }

            // Get item properties
            const ItemProperties* props = item_get_properties(entry->item);
            if (!props) continue;

            // Handle stacking
            if (props->max_stack_size > 1 && count > props->max_stack_size) {
                count = props->max_stack_size;
            }

            // Place in chest
            chest->slots[slot] = (ItemStack){
                .type = entry->item,
                .count = (uint8_t)count,
                .durability = props->durability,
                .max_durability = props->durability
            };
            slot++;
        }
    }

    // Shuffle slots for more natural distribution
    for (int i = slot - 1; i > 0; i--) {
        int j = (int)(random_from_seed(&seed) * (i + 1));
        ItemStack temp = chest->slots[i];
        chest->slots[i] = chest->slots[j];
        chest->slots[j] = temp;
    }

    printf("[CHEST] Generated dungeon loot at (%d, %d, %d): %d items\n",
           chest->x, chest->y, chest->z, slot);
}

// ============================================================================
// ITEM OPERATIONS
// ============================================================================

bool chest_is_empty(const ChestData* chest) {
    if (!chest) return true;

    for (int i = 0; i < CHEST_SLOTS; i++) {
        if (chest->slots[i].type != ITEM_NONE && chest->slots[i].count > 0) {
            return false;
        }
    }
    return true;
}

bool chest_add_item(ChestData* chest, ItemStack item) {
    if (!chest || item.type == ITEM_NONE || item.count == 0) return false;

    const ItemProperties* props = item_get_properties(item.type);
    if (!props) return false;

    // First try to stack with existing items
    if (props->max_stack_size > 1) {
        for (int i = 0; i < CHEST_SLOTS; i++) {
            if (chest->slots[i].type == item.type) {
                int space = props->max_stack_size - chest->slots[i].count;
                if (space >= item.count) {
                    chest->slots[i].count += item.count;
                    return true;
                }
            }
        }
    }

    // Find first empty slot
    for (int i = 0; i < CHEST_SLOTS; i++) {
        if (chest->slots[i].type == ITEM_NONE || chest->slots[i].count == 0) {
            chest->slots[i] = item;
            return true;
        }
    }

    return false;  // Chest is full
}

ItemStack chest_take_item(ChestData* chest, int slot) {
    if (!chest || slot < 0 || slot >= CHEST_SLOTS) {
        return (ItemStack){ITEM_NONE, 0, 0, 0};
    }

    ItemStack item = chest->slots[slot];
    chest->slots[slot] = (ItemStack){ITEM_NONE, 0, 0, 0};
    return item;
}
