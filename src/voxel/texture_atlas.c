/**
 * Texture Atlas Implementation
 */

#include "voxel/texture_atlas.h"
#include "voxel/block.h"
#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Clamp a value between min and max
 */
static inline int clamp_int(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// ============================================================================
// GLOBAL STATE
// ============================================================================

static Texture2D g_atlas_texture;
static Material g_atlas_material;
static bool g_initialized = false;

// ============================================================================
// TEXTURE GENERATION
// ============================================================================

/**
 * Generate a colored tile in the atlas
 */
static void generate_tile(Image* atlas, int tile_x, int tile_y, Color color, bool add_border) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            Color pixel_color = color;

            // Add border for visibility
            if (add_border && (x == 0 || y == 0 || x == TILE_SIZE - 1 || y == TILE_SIZE - 1)) {
                pixel_color = (Color){
                    (unsigned char)(color.r * 0.6f),
                    (unsigned char)(color.g * 0.6f),
                    (unsigned char)(color.b * 0.6f),
                    255
                };
            }

            // Add noise for texture variation
            if (x > 0 && y > 0 && x < TILE_SIZE - 1 && y < TILE_SIZE - 1) {
                int noise = (x * 7 + y * 13) % 20 - 10;
                pixel_color.r = (unsigned char)clamp_int(pixel_color.r + noise, 0, 255);
                pixel_color.g = (unsigned char)clamp_int(pixel_color.g + noise, 0, 255);
                pixel_color.b = (unsigned char)clamp_int(pixel_color.b + noise, 0, 255);
            }

            ImageDrawPixel(atlas, start_x + x, start_y + y, pixel_color);
        }
    }
}

/**
 * Generate procedural texture atlas
 */
static Image generate_atlas_image(void) {
    printf("[ATLAS] Generating atlas image...\n");
    // Use black background instead of pink to avoid texture bleeding
    Image atlas = GenImageColor(ATLAS_SIZE, ATLAS_SIZE, BLACK);
    printf("[ATLAS] Created base image\n");

    // Generate textures for each block type
    // Layout: Each row can hold different faces, or we use simple single-texture blocks

    // GRASS - Row 0
    generate_tile(&atlas, 0, 0, (Color){34, 139, 34, 255}, true);    // Top: Green grass
    generate_tile(&atlas, 1, 0, (Color){139, 69, 19, 255}, true);    // Bottom: Dirt
    generate_tile(&atlas, 2, 0, (Color){107, 142, 35, 255}, true);   // Sides: Grass side

    // DIRT - Row 1
    generate_tile(&atlas, 0, 1, (Color){139, 69, 19, 255}, true);    // All faces: Brown dirt

    // STONE - Row 2
    generate_tile(&atlas, 0, 2, (Color){128, 128, 128, 255}, true);  // All faces: Gray stone

    // WOOD - Row 3
    generate_tile(&atlas, 0, 3, (Color){139, 90, 43, 255}, true);    // Sides: Brown bark
    generate_tile(&atlas, 1, 3, (Color){205, 133, 63, 255}, true);   // Top/Bottom: Rings

    // LEAVES - Row 4
    generate_tile(&atlas, 0, 4, (Color){46, 125, 50, 255}, true);    // All faces: Dark green

    // SAND - Row 5
    generate_tile(&atlas, 0, 5, (Color){238, 214, 175, 255}, true);  // All faces: Beige sand

    // WATER - Row 6 (semi-transparent blue)
    generate_tile(&atlas, 0, 6, (Color){64, 164, 223, 180}, false);  // All faces: Blue water

    // COBBLESTONE - Row 7
    generate_tile(&atlas, 0, 7, (Color){112, 112, 112, 255}, true);  // All faces: Dark gray

    // BEDROCK - Row 8
    generate_tile(&atlas, 0, 8, (Color){64, 64, 64, 255}, true);     // All faces: Very dark gray

    return atlas;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize texture atlas system
 */
void texture_atlas_init(void) {
    if (g_initialized) return;

    printf("[ATLAS] Generating procedural texture atlas (%dx%d)...\n", ATLAS_SIZE, ATLAS_SIZE);

    // Generate atlas image
    Image atlas_image = generate_atlas_image();

    // Upload to GPU
    g_atlas_texture = LoadTextureFromImage(atlas_image);
    SetTextureFilter(g_atlas_texture, TEXTURE_FILTER_POINT);  // Pixelated look
    SetTextureWrap(g_atlas_texture, TEXTURE_WRAP_CLAMP);      // Prevent bleeding

    // Unload CPU image
    UnloadImage(atlas_image);

    // Create material with atlas texture
    g_atlas_material = LoadMaterialDefault();
    g_atlas_material.maps[MATERIAL_MAP_DIFFUSE].texture = g_atlas_texture;

    g_initialized = true;

    printf("[ATLAS] Texture atlas created successfully (ID: %d)\n", g_atlas_texture.id);
}

/**
 * Destroy texture atlas
 */
void texture_atlas_destroy(void) {
    if (!g_initialized) return;

    UnloadTexture(g_atlas_texture);
    UnloadMaterial(g_atlas_material);

    g_initialized = false;
    printf("[ATLAS] Texture atlas destroyed\n");
}

// ============================================================================
// TEXTURE COORDINATE LOOKUP
// ============================================================================

/**
 * Get texture coordinates for a block face
 */
TextureCoords texture_atlas_get_coords(BlockType block_type, BlockFace face) {
    TextureCoords coords = {0};

    // Calculate UV coordinates based on tile position
    // UV range is 0.0 to 1.0
    float tile_uv_size = 1.0f / (float)TILES_PER_ROW;

    int tile_x = 0;
    int tile_y = 0;

    // Map block types to tile positions
    switch (block_type) {
        case BLOCK_GRASS:
            if (face == FACE_TOP) {
                tile_x = 0; tile_y = 0;  // Green grass top
            } else if (face == FACE_BOTTOM) {
                tile_x = 1; tile_y = 0;  // Dirt bottom
            } else {
                tile_x = 2; tile_y = 0;  // Grass side
            }
            break;

        case BLOCK_DIRT:
            tile_x = 0; tile_y = 1;
            break;

        case BLOCK_STONE:
            tile_x = 0; tile_y = 2;
            break;

        case BLOCK_WOOD:
            if (face == FACE_TOP || face == FACE_BOTTOM) {
                tile_x = 1; tile_y = 3;  // Wood rings
            } else {
                tile_x = 0; tile_y = 3;  // Bark
            }
            break;

        case BLOCK_LEAVES:
            tile_x = 0; tile_y = 4;
            break;

        case BLOCK_SAND:
            tile_x = 0; tile_y = 5;
            break;

        case BLOCK_WATER:
            tile_x = 0; tile_y = 6;
            break;

        case BLOCK_COBBLESTONE:
            tile_x = 0; tile_y = 7;
            break;

        case BLOCK_BEDROCK:
            tile_x = 0; tile_y = 8;
            break;

        default:
            tile_x = 0; tile_y = 0;  // Default to first tile
            break;
    }

    // Calculate UV coordinates with small padding to prevent bleeding
    float padding = 0.001f;  // Small inset to prevent sampling adjacent tiles

    coords.u_min = (float)tile_x * tile_uv_size + padding;
    coords.v_min = (float)tile_y * tile_uv_size + padding;
    coords.u_max = coords.u_min + tile_uv_size - padding * 2.0f;
    coords.v_max = coords.v_min + tile_uv_size - padding * 2.0f;

    return coords;
}

/**
 * Get the texture atlas texture
 */
Texture2D texture_atlas_get_texture(void) {
    return g_atlas_texture;
}

/**
 * Get material with texture atlas
 */
Material texture_atlas_get_material(void) {
    return g_atlas_material;
}
