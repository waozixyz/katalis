/**
 * Texture Atlas Implementation
 */

#include "voxel/texture_atlas.h"
#include "voxel/block.h"
#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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
 * Generate a leaf tile with real semi-transparency
 * All pixels have alpha ~180 for see-through foliage effect
 */
static void generate_leaf_tile(Image* atlas, int tile_x, int tile_y, Color base_color) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            // Color variation with noise
            int noise = (x * 11 + y * 17) % 30 - 15;
            Color pixel_color = {
                (unsigned char)clamp_int(base_color.r + noise, 0, 255),
                (unsigned char)clamp_int(base_color.g + noise, 0, 255),
                (unsigned char)clamp_int(base_color.b + noise / 2, 0, 255),
                180  // Semi-transparent alpha
            };
            ImageDrawPixel(atlas, start_x + x, start_y + y, pixel_color);
        }
    }
}

/**
 * Draw a THICK black crack line with thin edge
 */
static void draw_thick_crack(Image* atlas, int start_x, int start_y,
                             int x1, int y1, int x2, int y2, int thickness) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int cx = x1, cy = y1;

    // First pass: thin light edge (1 pixel around the crack)
    while (1) {
        int edge = thickness / 2 + 1;
        for (int ty = -edge; ty <= edge; ty++) {
            for (int tx = -edge; tx <= edge; tx++) {
                int px = cx + tx;
                int py = cy + ty;
                if (px >= 0 && px < TILE_SIZE && py >= 0 && py < TILE_SIZE) {
                    ImageDrawPixel(atlas, start_x + px, start_y + py,
                                  (Color){150, 150, 150, 80});
                }
            }
        }
        if (cx == x2 && cy == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; cx += sx; }
        if (e2 < dx) { err += dx; cy += sy; }
    }

    // Reset for second pass
    cx = x1; cy = y1;
    err = dx - dy;

    // Second pass: dark black core
    while (1) {
        for (int ty = -thickness / 2; ty <= thickness / 2; ty++) {
            for (int tx = -thickness / 2; tx <= thickness / 2; tx++) {
                int px = cx + tx;
                int py = cy + ty;
                if (px >= 0 && px < TILE_SIZE && py >= 0 && py < TILE_SIZE) {
                    ImageDrawPixel(atlas, start_x + px, start_y + py,
                                  (Color){25, 25, 25, 160});
                }
            }
        }
        if (cx == x2 && cy == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; cx += sx; }
        if (e2 < dx) { err += dx; cy += sy; }
    }
}

/**
 * Generate a crack overlay tile for mining animation
 * Stage 0-9 represents increasing damage with THICK visible fracture lines
 */
static void generate_crack_tile(Image* atlas, int stage, int tile_x, int tile_y) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;
    int center = TILE_SIZE / 2;

    // Clear tile to transparent first
    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, (Color){0, 0, 0, 0});
        }
    }

    // Thickness increases with damage (1 at start, 2-3 at end)
    int thickness = 1 + stage / 4;  // 1,1,1,1,2,2,2,2,3,3

    // Crack reach from center increases with stage
    int reach = 2 + (stage * 6) / 9;  // 2 to 8 pixels

    // 8 directions: cardinal + diagonal
    int dirs[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},  // Cardinal
        {1, 1}, {-1, -1}, {1, -1}, {-1, 1} // Diagonal
    };

    // Number of cracks based on stage (2 to 8)
    int num_cracks = 2 + (stage * 6) / 9;
    if (num_cracks > 8) num_cracks = 8;

    // Draw main cracks from center outward
    for (int i = 0; i < num_cracks; i++) {
        // Pick direction based on stage and index for variety
        int dir_idx = (i + stage) % 8;

        // Add jitter for natural look
        int jitter_x = ((i * 7 + stage * 3) % 5) - 2;  // -2 to +2
        int jitter_y = ((i * 11 + stage * 5) % 5) - 2;

        int end_x = center + dirs[dir_idx][0] * reach + jitter_x;
        int end_y = center + dirs[dir_idx][1] * reach + jitter_y;

        // Clamp to tile bounds
        if (end_x < 1) end_x = 1;
        if (end_x > TILE_SIZE - 2) end_x = TILE_SIZE - 2;
        if (end_y < 1) end_y = 1;
        if (end_y > TILE_SIZE - 2) end_y = TILE_SIZE - 2;

        draw_thick_crack(atlas, start_x, start_y, center, center, end_x, end_y, thickness);

        // Add branch from midpoint at higher stages
        if (stage >= 4 && i < 4) {
            int mid_x = center + (end_x - center) / 2;
            int mid_y = center + (end_y - center) / 2;
            int branch_dir = (dir_idx + 2) % 8;
            int branch_x = mid_x + dirs[branch_dir][0] * (reach / 2);
            int branch_y = mid_y + dirs[branch_dir][1] * (reach / 2);

            if (branch_x >= 0 && branch_x < TILE_SIZE &&
                branch_y >= 0 && branch_y < TILE_SIZE) {
                draw_thick_crack(atlas, start_x, start_y, mid_x, mid_y, branch_x, branch_y, thickness);
            }
        }
    }

    // At high damage (7+), add corner-to-center cracks
    if (stage >= 7) {
        draw_thick_crack(atlas, start_x, start_y, 1, 1, center - 2, center - 2, thickness);
        draw_thick_crack(atlas, start_x, start_y, TILE_SIZE - 2, 1, center + 2, center - 2, thickness);
    }
    if (stage >= 8) {
        draw_thick_crack(atlas, start_x, start_y, 1, TILE_SIZE - 2, center - 2, center + 2, thickness);
        draw_thick_crack(atlas, start_x, start_y, TILE_SIZE - 2, TILE_SIZE - 2, center + 2, center + 2, thickness);
    }
}

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

    // LEAVES - Row 4 (Bright foliage green with transparency holes - Luanti style)
    generate_leaf_tile(&atlas, 0, 4, (Color){60, 180, 75, 255});     // All faces: Semi-transparent leaves

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

    // MOSSY_COBBLE - Row 15 (Cobblestone with green moss)
    generate_tile(&atlas, 0, 15, (Color){90, 120, 90, 255}, true);    // Mossy green-gray cobble

    // STONE_BRICK - Row 16 (Clean cut stone bricks) - use column 1 for variety
    generate_tile(&atlas, 0, 16, (Color){140, 140, 145, 255}, true);  // Clean gray bricks

    // CRACKED_BRICK - Row 16 col 1 (Damaged stone bricks)
    generate_tile(&atlas, 1, 16, (Color){120, 115, 110, 255}, true);  // Darker cracked bricks

    // CLAY - Row 17 (Blue-gray clay)
    generate_tile(&atlas, 0, 17, (Color){160, 165, 180, 255}, true);  // Blue-gray clay

    // SNOW - Row 18 (white with slight blue tint)
    generate_tile(&atlas, 0, 18, (Color){250, 250, 255, 255}, true);  // Snow surface

    // CACTUS - Row 19 (dark green)
    generate_tile(&atlas, 0, 19, (Color){40, 120, 40, 255}, true);    // Cactus

    // ITEM TEXTURES (Row 0, columns 3+)
    // MEAT - Raw meat item (pinkish-red)
    generate_tile(&atlas, 6, 0, (Color){200, 100, 100, 255}, true);   // Raw meat

    // CRACK TEXTURES - Row 20 (10 stages of block damage)
    for (int i = 0; i < 10; i++) {
        generate_crack_tile(&atlas, i, i, 20);  // Columns 0-9, Row 20
    }

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

        case BLOCK_MOSSY_COBBLE:
            tile_x = 0; tile_y = 15;
            break;

        case BLOCK_STONE_BRICK:
            tile_x = 0; tile_y = 16;
            break;

        case BLOCK_CRACKED_BRICK:
            tile_x = 1; tile_y = 16;
            break;

        case BLOCK_CLAY:
            tile_x = 0; tile_y = 17;
            break;

        case BLOCK_SNOW:
            tile_x = 0; tile_y = 18;
            break;

        case BLOCK_CACTUS:
            tile_x = 0; tile_y = 19;
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

/**
 * Get texture coordinates for a crack overlay stage (0-9)
 */
TextureCoords texture_atlas_get_crack_coords(int stage) {
    TextureCoords coords = {0};

    // Clamp stage to valid range
    if (stage < 0) stage = 0;
    if (stage > 9) stage = 9;

    float tile_uv_size = 1.0f / (float)TILES_PER_ROW;
    float padding = 0.001f;

    // Crack textures are at row 20, columns 0-9
    int tile_x = stage;
    int tile_y = 20;

    coords.u_min = (float)tile_x * tile_uv_size + padding;
    coords.v_min = (float)tile_y * tile_uv_size + padding;
    coords.u_max = coords.u_min + tile_uv_size - padding * 2.0f;
    coords.v_max = coords.v_min + tile_uv_size - padding * 2.0f;

    return coords;
}
