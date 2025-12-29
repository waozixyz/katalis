/**
 * Minimap System
 *
 * Renders a top-down view of the terrain in the top-right corner.
 * Shows block types as colors with height-based shading and a player indicator.
 */

#ifndef VOXEL_MINIMAP_H
#define VOXEL_MINIMAP_H

#include <raylib.h>
#include "world.h"
#include "player.h"

// Minimap configuration
#define MINIMAP_SIZE 150         // Size in pixels (square)
#define MINIMAP_RADIUS 64        // Radius in blocks to display
#define MINIMAP_MARGIN 10        // Margin from screen edge

// Opaque minimap type
typedef struct Minimap Minimap;

/**
 * Create minimap
 */
Minimap* minimap_create(void);

/**
 * Destroy minimap and free resources
 */
void minimap_destroy(Minimap* minimap);

/**
 * Update minimap texture from world data
 * Call this each frame (internally throttles updates)
 */
void minimap_update(Minimap* minimap, World* world, Player* player);

/**
 * Draw minimap on screen (top-right corner)
 */
void minimap_draw(Minimap* minimap, Player* player);

#endif // VOXEL_MINIMAP_H
