/**
 * Texture Atlas - Block Textures
 *
 * Manages a texture atlas containing all block textures
 * for efficient rendering with minimal texture swaps
 */

#ifndef VOXEL_TEXTURE_ATLAS_H
#define VOXEL_TEXTURE_ATLAS_H

#include <raylib.h>
#include "block.h"

// ============================================================================
// TEXTURE ATLAS CONFIGURATION
// ============================================================================

#define ATLAS_SIZE 512          // Atlas texture size (512x512)
#define TILE_SIZE 16            // Each tile is 16x16 pixels
#define TILES_PER_ROW 32        // 32 tiles per row (512/16)

// ============================================================================
// TEXTURE COORDINATES
// ============================================================================

typedef struct {
    float u_min;  // Left U coordinate (0.0 to 1.0)
    float v_min;  // Top V coordinate (0.0 to 1.0)
    float u_max;  // Right U coordinate (0.0 to 1.0)
    float v_max;  // Bottom V coordinate (0.0 to 1.0)
} TextureCoords;

// Face indices for texture coordinates
typedef enum {
    FACE_TOP = 0,
    FACE_BOTTOM = 1,
    FACE_FRONT = 2,
    FACE_BACK = 3,
    FACE_LEFT = 4,
    FACE_RIGHT = 5
} BlockFace;

// ============================================================================
// API
// ============================================================================

/**
 * Initialize texture atlas system
 * Creates a procedurally generated texture atlas
 */
void texture_atlas_init(void);

/**
 * Destroy texture atlas
 */
void texture_atlas_destroy(void);

/**
 * Get texture coordinates for a block face
 */
TextureCoords texture_atlas_get_coords(BlockType block_type, BlockFace face);

/**
 * Get the texture atlas texture (for binding during rendering)
 */
Texture2D texture_atlas_get_texture(void);

/**
 * Get material with texture atlas
 */
Material texture_atlas_get_material(void);

/**
 * Get texture coordinates for a crack overlay stage (0-9)
 */
TextureCoords texture_atlas_get_crack_coords(int stage);

#endif // VOXEL_TEXTURE_ATLAS_H
