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
    // Use neutral brown background that won't be noticeable if it bleeds
    Color neutral_brown = {120, 100, 80, 255};  // Matches dirt/stone tones
    Image atlas = GenImageColor(ATLAS_SIZE, ATLAS_SIZE, neutral_brown);
    printf("[ATLAS] Created base image\n");

    // Generate textures for each block type
    // Layout: Each row can hold different faces, or we use simple single-texture blocks

    // GRASS - Row 0 (Vibrant green top, rich dirt bottom)
    generate_tile(&atlas, 0, 0, (Color){50, 180, 50, 255}, true);     // Top: Bright grass green
    generate_tile(&atlas, 1, 0, (Color){150, 85, 40, 255}, true);     // Bottom: Rich dirt
    generate_tile(&atlas, 2, 0, (Color){120, 160, 60, 255}, true);    // Sides: Grass-dirt blend

    // DIRT - Row 1 (Rich brown earth)
    generate_tile(&atlas, 0, 1, (Color){150, 85, 40, 255}, true);     // All faces: Rich brown

    // STONE - Row 2 (Warm light gray, not cold gray)
    generate_tile(&atlas, 0, 2, (Color){160, 160, 165, 255}, true);   // All faces: Light warm stone

    // WOOD - Row 3 (Warm oak wood)
    generate_tile(&atlas, 0, 3, (Color){120, 80, 50, 255}, true);     // Sides: Dark bark
    generate_tile(&atlas, 1, 3, (Color){200, 150, 100, 255}, true);   // Top/Bottom: Light wood rings

    // LEAVES - Row 4 (Bright foliage green)
    generate_tile(&atlas, 0, 4, (Color){60, 180, 75, 255}, true);     // All faces: Bright green leaves

    // SAND - Row 5 (Warm golden sand)
    generate_tile(&atlas, 0, 5, (Color){240, 220, 130, 255}, true);   // All faces: Golden sand

    // WATER - Row 6 (Bright cyan-blue)
    generate_tile(&atlas, 0, 6, (Color){50, 150, 255, 180}, false);   // All faces: Bright water

    // COBBLESTONE - Row 7 (Brown-gray mix, not pure gray)
    generate_tile(&atlas, 0, 7, (Color){130, 120, 110, 255}, true);   // All faces: Brown-gray cobble

    // BEDROCK - Row 8 (Dark with purple tint)
    generate_tile(&atlas, 0, 8, (Color){50, 45, 55, 255}, true);      // All faces: Dark purple-gray

    // DEEP_STONE - Row 9 (Darker stone for deep layers)
    generate_tile(&atlas, 0, 9, (Color){100, 100, 105, 255}, true);   // All faces: Dark gray stone

    // GRAVEL - Row 10 (Light gray with pebble texture)
    generate_tile(&atlas, 0, 10, (Color){140, 140, 145, 255}, true);  // All faces: Light gray gravel

    // COAL_ORE - Row 11 (Stone with black spots)
    generate_tile(&atlas, 0, 11, (Color){160, 160, 165, 255}, true);  // Base: Stone color
    generate_tile(&atlas, 1, 11, (Color){40, 40, 45, 255}, false);    // Coal spots: Very dark gray

    // IRON_ORE - Row 12 (Stone with brown/orange spots)
    generate_tile(&atlas, 0, 12, (Color){160, 160, 165, 255}, true);  // Base: Stone color
    generate_tile(&atlas, 1, 12, (Color){180, 120, 80, 255}, false);  // Iron spots: Brown-orange

    // GOLD_ORE - Row 13 (Stone with yellow spots)
    generate_tile(&atlas, 0, 13, (Color){160, 160, 165, 255}, true);  // Base: Stone color
    generate_tile(&atlas, 1, 13, (Color){255, 215, 0, 255}, false);   // Gold spots: Bright yellow

    // DIAMOND_ORE - Row 14 (Stone with cyan spots)
    generate_tile(&atlas, 0, 14, (Color){160, 160, 165, 255}, true);  // Base: Stone color
    generate_tile(&atlas, 1, 14, (Color){0, 200, 255, 255}, false);   // Diamond spots: Bright cyan

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
    SetTextureWrap(g_atlas_texture, TEXTURE_WRAP_REPEAT);     // Allow tiling for greedy quads

    // Unload CPU image
    UnloadImage(atlas_image);

    // Create material with atlas texture
    // Load custom block shader for ambient lighting
    Shader block_shader = LoadShader("shaders/block.vs", "shaders/block.fs");

    g_atlas_material = LoadMaterialDefault();
    if (block_shader.id > 0) {
        g_atlas_material.shader = block_shader;
        printf("[ATLAS] Block shader loaded successfully (ID: %d)\n", block_shader.id);
    } else {
        printf("[ATLAS] WARNING: Failed to load block shader, using default\n");
    }
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

        case BLOCK_DEEP_STONE:
            tile_x = 0; tile_y = 9;
            break;

        case BLOCK_GRAVEL:
            tile_x = 0; tile_y = 10;
            break;

        case BLOCK_COAL_ORE:
            tile_x = 0; tile_y = 11;  // For now, use base stone texture
            break;

        case BLOCK_IRON_ORE:
            tile_x = 0; tile_y = 12;
            break;

        case BLOCK_GOLD_ORE:
            tile_x = 0; tile_y = 13;
            break;

        case BLOCK_DIAMOND_ORE:
            tile_x = 0; tile_y = 14;
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
