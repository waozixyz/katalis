/**
 * Spawn System - Biome-aware animal spawning with herds
 *
 * Handles procedural spawning of animals based on biome type.
 * Animals spawn in herds (groups of same species) for natural behavior.
 */

#ifndef VOXEL_WORLD_SPAWN_H
#define VOXEL_WORLD_SPAWN_H

#include "voxel/entity/entity.h"
#include "voxel/world/biome.h"
#include "voxel/world/terrain.h"
#include <stdbool.h>

// Forward declarations
struct World;

// ============================================================================
// SPAWN CONFIGURATION
// ============================================================================

#define MAX_HERD_RULES_PER_BIOME 4

/**
 * Defines how a single herd type spawns
 */
typedef struct {
    EntityType animal_type;     // ENTITY_TYPE_SHEEP, ENTITY_TYPE_PIG, etc.
    int min_herd_size;          // Minimum animals in herd
    int max_herd_size;          // Maximum animals in herd
    float herd_radius;          // Radius to spread herd members
    float spawn_chance;         // Per-chunk spawn chance (0.0 - 1.0)
} HerdSpawnRule;

/**
 * Per-biome spawn configuration
 */
typedef struct {
    HerdSpawnRule herd_rules[MAX_HERD_RULES_PER_BIOME];
    int herd_rule_count;        // Number of active herd rules
} BiomeSpawnRules;

// ============================================================================
// SPAWN SYSTEM API
// ============================================================================

/**
 * Initialize the spawn system
 * Sets up biome spawn rules
 */
void spawn_system_init(void);

/**
 * Spawn animals for a newly loaded chunk
 * Uses deterministic seeding for reproducible spawns
 *
 * @param world World containing entity manager
 * @param chunk_x Chunk X coordinate
 * @param chunk_z Chunk Z coordinate
 * @param params Terrain parameters for height lookup
 */
void spawn_animals_for_chunk(struct World* world, int chunk_x, int chunk_z, TerrainParams params);

/**
 * Spawn a herd of animals at a position
 *
 * @param manager Entity manager to spawn into
 * @param type Animal type to spawn
 * @param center Center position of herd
 * @param count Number of animals to spawn
 * @param radius Spread radius for herd
 * @param params Terrain parameters for height lookup
 */
void spawn_herd(EntityManager* manager, EntityType type, Vector3 center,
                int count, float radius, TerrainParams params);

/**
 * Get spawn rules for a specific biome
 *
 * @param biome Biome type
 * @return Pointer to spawn rules for that biome
 */
const BiomeSpawnRules* spawn_get_biome_rules(BiomeType biome);

#endif // VOXEL_WORLD_SPAWN_H
