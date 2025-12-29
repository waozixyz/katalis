/**
 * Light System - Skylight Propagation
 *
 * Calculates light levels for blocks based on exposure to sky.
 * Caves and underground areas are dark, surface is fully lit.
 */

#ifndef VOXEL_LIGHT_H
#define VOXEL_LIGHT_H

#include "voxel/world/chunk.h"

// Light level constants (0-15 range like Minecraft)
#define LIGHT_MAX 15
#define LIGHT_MIN 0

/**
 * Calculate skylight for entire chunk.
 * Light propagates straight down from the sky.
 * Call this after terrain generation is complete.
 */
void light_calculate_chunk(Chunk* chunk);

/**
 * Recalculate light for a single column when a block changes.
 * More efficient than recalculating entire chunk.
 */
void light_update_column(Chunk* chunk, int x, int z);

#endif // VOXEL_LIGHT_H
