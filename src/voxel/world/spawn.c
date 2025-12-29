/**
 * Spawn System Implementation
 *
 * Biome-aware animal spawning with natural herd clustering
 */

#include "voxel/world/spawn.h"
#include "voxel/entity/sheep.h"
#include "voxel/entity/pig.h"
#include "voxel/world/chunk.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// BIOME SPAWN RULES
// ============================================================================

static BiomeSpawnRules biome_spawn_rules[BIOME_COUNT];
static bool spawn_initialized = false;

void spawn_system_init(void) {
    if (spawn_initialized) return;

    // PLAINS - Most animals, moderate herds
    biome_spawn_rules[BIOME_PLAINS] = (BiomeSpawnRules){
        .herd_rules = {
            { ENTITY_TYPE_SHEEP, 3, 6, 8.0f, 0.4f },   // 40% chance, 3-6 sheep
            { ENTITY_TYPE_PIG, 2, 4, 6.0f, 0.3f },     // 30% chance, 2-4 pigs
        },
        .herd_rule_count = 2
    };

    // FOREST - Both animals, slightly smaller herds (trees in way)
    biome_spawn_rules[BIOME_FOREST] = (BiomeSpawnRules){
        .herd_rules = {
            { ENTITY_TYPE_SHEEP, 2, 4, 6.0f, 0.3f },   // 30% chance, 2-4 sheep
            { ENTITY_TYPE_PIG, 2, 5, 7.0f, 0.35f },    // 35% chance, 2-5 pigs
        },
        .herd_rule_count = 2
    };

    // DESERT - No animals
    biome_spawn_rules[BIOME_DESERT] = (BiomeSpawnRules){
        .herd_rules = {},
        .herd_rule_count = 0
    };

    // TUNDRA - Only sheep (hardy animals), larger herds
    biome_spawn_rules[BIOME_TUNDRA] = (BiomeSpawnRules){
        .herd_rules = {
            { ENTITY_TYPE_SHEEP, 4, 8, 10.0f, 0.25f }, // 25% chance, 4-8 sheep
        },
        .herd_rule_count = 1
    };

    spawn_initialized = true;
    printf("[SPAWN] Spawn system initialized with biome rules\n");
}

// ============================================================================
// RANDOM HELPERS
// ============================================================================

// Simple deterministic hash for chunk coordinates
static unsigned int hash_chunk_coords(int x, int z) {
    unsigned int h = (unsigned int)(x * 374761393 + z * 668265263);
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

// Random float [0, 1) using current rand state
static float random_float(void) {
    return (float)rand() / (float)RAND_MAX;
}

// Gaussian-ish distribution using Box-Muller (simplified)
// Returns value roughly in [-2, 2] with most values near 0
static float random_gaussian(void) {
    // Use sum of uniform randoms for approximate gaussian
    float sum = 0.0f;
    for (int i = 0; i < 3; i++) {
        sum += random_float();
    }
    return (sum / 3.0f - 0.5f) * 4.0f;  // Center around 0, scale
}

// ============================================================================
// HERD SPAWNING
// ============================================================================

void spawn_herd(EntityManager* manager, EntityType type, Vector3 center,
                int count, float radius, TerrainParams params) {
    if (!manager) return;

    for (int i = 0; i < count; i++) {
        // Spread animals around herd center using gaussian distribution
        // This clusters animals near the center for natural herding
        float angle = random_float() * 2.0f * 3.14159f;
        float dist = fabsf(random_gaussian()) * radius * 0.4f;

        float x = center.x + cosf(angle) * dist;
        float z = center.z + sinf(angle) * dist;
        float y = (float)terrain_get_height_at((int)x, (int)z, params) + 1.0f;

        Vector3 pos = { x + 0.5f, y, z + 0.5f };

        Entity* entity = NULL;
        if (type == ENTITY_TYPE_SHEEP) {
            entity = sheep_spawn(manager, pos);
        } else if (type == ENTITY_TYPE_PIG) {
            entity = pig_spawn(manager, pos);
        }

        if (entity && i == 0) {
            // Log first animal of herd
            const char* type_name = (type == ENTITY_TYPE_SHEEP) ? "sheep" : "pig";
            printf("[SPAWN] Herd of %d %s at (%.1f, %.1f, %.1f)\n",
                   count, type_name, center.x, center.y, center.z);
        }
    }
}

// ============================================================================
// PER-CHUNK SPAWNING
// ============================================================================

void spawn_animals_for_chunk(struct World* world, int chunk_x, int chunk_z, TerrainParams params) {
    if (!world) return;

    // Get entity manager from world
    // Note: World needs to expose entity_manager or we need a getter
    extern EntityManager* world_get_entity_manager(struct World* world);
    EntityManager* manager = world_get_entity_manager(world);
    if (!manager) return;

    // Get chunk center in world coordinates
    int world_x = chunk_x * CHUNK_SIZE + CHUNK_SIZE / 2;
    int world_z = chunk_z * CHUNK_SIZE + CHUNK_SIZE / 2;

    // Determine biome at chunk center
    BiomeType biome = biome_get_at(world_x, world_z);
    const BiomeSpawnRules* rules = &biome_spawn_rules[biome];

    // No animals in this biome?
    if (rules->herd_rule_count == 0) return;

    // Use deterministic seed for this chunk (reproducible world)
    unsigned int seed = hash_chunk_coords(chunk_x, chunk_z);
    srand(seed);

    // Try spawning each herd type
    for (int i = 0; i < rules->herd_rule_count; i++) {
        const HerdSpawnRule* herd = &rules->herd_rules[i];

        // Roll for spawn chance
        if (random_float() < herd->spawn_chance) {
            // Pick random position within chunk
            float herd_x = (float)world_x + (random_float() - 0.5f) * CHUNK_SIZE;
            float herd_z = (float)world_z + (random_float() - 0.5f) * CHUNK_SIZE;
            float herd_y = (float)terrain_get_height_at((int)herd_x, (int)herd_z, params) + 1.0f;

            Vector3 center = { herd_x, herd_y, herd_z };

            // Determine herd size
            int size_range = herd->max_herd_size - herd->min_herd_size + 1;
            int count = herd->min_herd_size + (rand() % size_range);

            // Spawn the herd
            spawn_herd(manager, herd->animal_type, center, count, herd->herd_radius, params);
        }
    }
}

const BiomeSpawnRules* spawn_get_biome_rules(BiomeType biome) {
    if (biome < 0 || biome >= BIOME_COUNT) return NULL;
    return &biome_spawn_rules[biome];
}
