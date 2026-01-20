/**
 * Texture Atlas Implementation
 */

#include "voxel/core/texture_atlas.h"
#include "voxel/core/block.h"
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
 * Draw a stick item texture (diagonal brown stick)
 */
static void generate_stick_tile(Image* atlas, int tile_x, int tile_y) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    // Clear to transparent
    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, (Color){0, 0, 0, 0});
        }
    }

    // Draw diagonal stick from bottom-left to top-right
    Color stick_dark = {100, 70, 40, 255};
    Color stick_light = {140, 100, 60, 255};

    // Stick pattern (2 pixels wide, diagonal)
    for (int i = 0; i < 12; i++) {
        int x = 3 + i;
        int y = 12 - i;
        if (x >= 0 && x < TILE_SIZE && y >= 0 && y < TILE_SIZE) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, stick_dark);
            if (x + 1 < TILE_SIZE) {
                ImageDrawPixel(atlas, start_x + x + 1, start_y + y, stick_light);
            }
        }
    }
}

/**
 * Draw a pickaxe tool texture
 */
static void generate_pickaxe_tile(Image* atlas, int tile_x, int tile_y, Color head_color) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    // Clear to transparent
    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, (Color){0, 0, 0, 0});
        }
    }

    Color stick_dark = {100, 70, 40, 255};
    Color stick_light = {140, 100, 60, 255};
    Color head_dark = {
        (unsigned char)(head_color.r * 0.7f),
        (unsigned char)(head_color.g * 0.7f),
        (unsigned char)(head_color.b * 0.7f),
        255
    };

    // Draw handle (vertical, centered under head)
    for (int y = 5; y <= 14; y++) {
        ImageDrawPixel(atlas, start_x + 7, start_y + y, stick_dark);
        ImageDrawPixel(atlas, start_x + 8, start_y + y, stick_light);
    }

    // Draw pickaxe head (horizontal with diagonal tips)
    // Top edge
    for (int x = 2; x <= 13; x++) {
        ImageDrawPixel(atlas, start_x + x, start_y + 2, head_color);
    }
    // Second row
    for (int x = 3; x <= 12; x++) {
        ImageDrawPixel(atlas, start_x + x, start_y + 3, head_color);
    }
    // Third row (narrower)
    for (int x = 5; x <= 10; x++) {
        ImageDrawPixel(atlas, start_x + x, start_y + 4, head_dark);
    }
    // Tip details
    ImageDrawPixel(atlas, start_x + 1, start_y + 3, head_dark);
    ImageDrawPixel(atlas, start_x + 14, start_y + 3, head_dark);
}

/**
 * Draw a shovel tool texture
 */
static void generate_shovel_tile(Image* atlas, int tile_x, int tile_y, Color head_color) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    // Clear to transparent
    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, (Color){0, 0, 0, 0});
        }
    }

    Color stick_dark = {100, 70, 40, 255};
    Color stick_light = {140, 100, 60, 255};
    Color head_dark = {
        (unsigned char)(head_color.r * 0.7f),
        (unsigned char)(head_color.g * 0.7f),
        (unsigned char)(head_color.b * 0.7f),
        255
    };

    // Draw handle (vertical, centered under head)
    for (int y = 8; y <= 14; y++) {
        ImageDrawPixel(atlas, start_x + 10, start_y + y, stick_dark);
        ImageDrawPixel(atlas, start_x + 11, start_y + y, stick_light);
    }

    // Draw shovel head (rounded rectangle at top)
    // Top row
    for (int x = 9; x <= 12; x++) {
        ImageDrawPixel(atlas, start_x + x, start_y + 1, head_color);
    }
    // Middle rows
    for (int y = 2; y <= 5; y++) {
        for (int x = 8; x <= 13; x++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, head_color);
        }
    }
    // Bottom rounded
    for (int x = 9; x <= 12; x++) {
        ImageDrawPixel(atlas, start_x + x, start_y + 6, head_dark);
    }
    ImageDrawPixel(atlas, start_x + 10, start_y + 7, head_dark);
    ImageDrawPixel(atlas, start_x + 11, start_y + 7, head_dark);
}

/**
 * Draw an axe tool texture
 */
static void generate_axe_tile(Image* atlas, int tile_x, int tile_y, Color head_color) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    // Clear to transparent
    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, (Color){0, 0, 0, 0});
        }
    }

    Color stick_dark = {100, 70, 40, 255};
    Color stick_light = {140, 100, 60, 255};
    Color head_dark = {
        (unsigned char)(head_color.r * 0.7f),
        (unsigned char)(head_color.g * 0.7f),
        (unsigned char)(head_color.b * 0.7f),
        255
    };

    // Draw handle (diagonal from bottom-left)
    for (int i = 0; i < 10; i++) {
        int x = 3 + i;
        int y = 13 - i;
        ImageDrawPixel(atlas, start_x + x, start_y + y, stick_dark);
        if (x + 1 < TILE_SIZE) {
            ImageDrawPixel(atlas, start_x + x + 1, start_y + y, stick_light);
        }
    }

    // Draw axe head (angular shape on right side)
    // Main blade
    for (int y = 2; y <= 6; y++) {
        for (int x = 9; x <= 13 - (y - 2); x++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, head_color);
        }
    }
    // Cutting edge (darker)
    for (int y = 2; y <= 6; y++) {
        int x = 13 - (y - 2);
        ImageDrawPixel(atlas, start_x + x, start_y + y, head_dark);
    }
    // Back of axe (near handle)
    for (int y = 3; y <= 5; y++) {
        ImageDrawPixel(atlas, start_x + 8, start_y + y, head_dark);
    }
}

/**
 * Draw a hand with long arm texture for first-person view
 * Layout: fingers at top, hand in upper portion, long arm extending to bottom
 */
static void generate_hand_tile(Image* atlas, int tile_x, int tile_y) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    // Clear to transparent
    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, (Color){0, 0, 0, 0});
        }
    }

    // Skin colors with better gradients
    Color skin = {220, 180, 140, 255};        // Main skin tone
    Color skin_dark = {180, 140, 100, 255};   // Shadow
    Color skin_light = {245, 210, 175, 255};  // Highlight
    Color skin_mid = {200, 160, 120, 255};    // Mid-tone

    // Sleeve colors (blue shirt) with gradient
    Color sleeve = {70, 110, 170, 255};        // Base blue
    Color sleeve_dark = {45, 75, 120, 255};    // Dark edge
    Color sleeve_light = {95, 140, 200, 255};  // Light edge
    Color sleeve_fold = {55, 90, 145, 255};    // Fold line

    // SLEEVE - large portion at bottom (y = 7 to 15 = 9 pixels, ~56% of tile)
    // Arm is wide: x = 2 to 13 = 12 pixels
    for (int y = 7; y <= 15; y++) {
        for (int x = 2; x <= 13; x++) {
            Color c = sleeve;
            // Horizontal shading - darker at edges
            if (x <= 3) c = sleeve_dark;
            else if (x >= 12) c = sleeve_light;
            // Vertical shading - darker at bottom
            if (y >= 14) c = sleeve_dark;
            // Fold detail in middle
            if (y == 9 && x >= 5 && x <= 10) c = sleeve_fold;
            if (y == 10 && x >= 4 && x <= 11) c = sleeve_fold;
            ImageDrawPixel(atlas, start_x + x, start_y + y, c);
        }
    }

    // FOREARM - bare skin above sleeve (y = 4 to 7)
    // Wide arm: x = 3 to 12
    for (int y = 4; y <= 7; y++) {
        for (int x = 3; x <= 12; x++) {
            Color c = skin;
            // Shading
            if (x <= 4) c = skin_dark;
            else if (x >= 11) c = skin_light;
            else if (x >= 6 && x <= 8) c = skin_mid;
            // Top of forearm slightly lighter
            if (y == 4) c = skin_light;
            ImageDrawPixel(atlas, start_x + x, start_y + y, c);
        }
    }

    // PALM/HAND area (y = 2 to 4)
    // Slightly wider for palm: x = 2 to 13
    for (int y = 2; y <= 4; y++) {
        for (int x = 2; x <= 13; x++) {
            Color c = skin;
            if (x <= 3) c = skin_dark;
            else if (x >= 12) c = skin_light;
            if (y == 4) c = skin_mid;
            ImageDrawPixel(atlas, start_x + x, start_y + y, c);
        }
    }

    // THUMB (left side, pointing up) - wider
    for (int y = 0; y <= 3; y++) {
        for (int x = 1; x <= 4; x++) {
            Color c = skin;
            if (x == 1) c = skin_dark;
            if (x == 4 || y == 0) c = skin_light;
            ImageDrawPixel(atlas, start_x + x, start_y + y, c);
        }
    }

    // FINGERS (curled on right side, gripping position)
    // Index finger
    for (int y = 0; y <= 2; y++) {
        for (int x = 11; x <= 14; x++) {
            Color c = skin;
            if (y == 2) c = skin_dark;
            if (y == 0) c = skin_light;
            if (x == 14) c = skin_light;
            ImageDrawPixel(atlas, start_x + x, start_y + y, c);
        }
    }
    // Middle finger
    for (int y = 1; y <= 3; y++) {
        for (int x = 12; x <= 15; x++) {
            Color c = skin;
            if (y == 3) c = skin_dark;
            if (y == 1) c = skin_light;
            ImageDrawPixel(atlas, start_x + x, start_y + y, c);
        }
    }
    // Ring finger
    for (int y = 2; y <= 4; y++) {
        for (int x = 11; x <= 14; x++) {
            Color c = skin;
            if (y == 4) c = skin_dark;
            ImageDrawPixel(atlas, start_x + x, start_y + y, c);
        }
    }
    // Pinky finger
    for (int y = 3; y <= 5; y++) {
        for (int x = 10; x <= 13; x++) {
            Color c = skin;
            if (y == 5) c = skin_dark;
            ImageDrawPixel(atlas, start_x + x, start_y + y, c);
        }
    }
}

/**
 * Draw a sword weapon texture
 */
static void generate_sword_tile(Image* atlas, int tile_x, int tile_y, Color blade_color) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    // Clear to transparent
    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, (Color){0, 0, 0, 0});
        }
    }

    Color stick_dark = {100, 70, 40, 255};
    Color blade_dark = {
        (unsigned char)(blade_color.r * 0.7f),
        (unsigned char)(blade_color.g * 0.7f),
        (unsigned char)(blade_color.b * 0.7f),
        255
    };
    Color guard = {80, 80, 80, 255};

    // Draw handle (bottom)
    for (int i = 0; i < 4; i++) {
        int x = 6 + i;
        int y = 12 - i;
        ImageDrawPixel(atlas, start_x + x, start_y + y, stick_dark);
        ImageDrawPixel(atlas, start_x + x + 1, start_y + y, stick_dark);
    }

    // Draw guard (crossguard)
    for (int x = 7; x <= 11; x++) {
        ImageDrawPixel(atlas, start_x + x, start_y + 8, guard);
    }

    // Draw blade (diagonal upward)
    for (int i = 0; i < 7; i++) {
        int x = 10 + i;
        int y = 7 - i;
        if (x < TILE_SIZE && y >= 0) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, blade_color);
            if (x + 1 < TILE_SIZE) {
                ImageDrawPixel(atlas, start_x + x + 1, start_y + y, blade_dark);
            }
        }
    }
    // Tip
    ImageDrawPixel(atlas, start_x + 14, start_y + 1, blade_dark);
}

/**
 * Generate an animated water tile frame
 * Creates subtle wave patterns that vary per frame
 */
static void generate_water_tile(Image* atlas, int tile_x, int tile_y, Color base_color, int frame) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    // Phase offset based on frame (0-3)
    float phase = (float)frame * 1.57f;  // PI/2 offset per frame

    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            Color pixel = base_color;

            // Create wave pattern using multiple sine waves
            float wave1 = sinf((float)x * 0.5f + phase) * 0.5f + 0.5f;
            float wave2 = sinf((float)y * 0.4f + phase * 0.7f) * 0.5f + 0.5f;
            float wave3 = sinf((float)(x + y) * 0.3f + phase * 1.3f) * 0.5f + 0.5f;

            // Combine waves for natural water look
            float combined = (wave1 + wave2 + wave3) / 3.0f;

            // Variation in brightness (-15 to +15)
            int brightness = (int)((combined - 0.5f) * 30.0f);

            // Add some extra noise for texture
            int noise = ((x * 7 + y * 13 + frame * 11) % 10) - 5;

            pixel.r = (unsigned char)clamp_int(pixel.r + brightness + noise, 0, 255);
            pixel.g = (unsigned char)clamp_int(pixel.g + brightness + noise, 0, 255);
            pixel.b = (unsigned char)clamp_int(pixel.b + brightness / 2 + noise, 0, 255);

            // Keep alpha for water transparency
            pixel.a = base_color.a;

            ImageDrawPixel(atlas, start_x + x, start_y + y, pixel);
        }
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
 * Generate a wood planks tile with horizontal grain pattern
 */
static void generate_planks_tile(Image* atlas, int tile_x, int tile_y, Color base_color) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            Color pixel = base_color;

            // Horizontal plank divider lines (every 5 pixels)
            if (y % 5 == 0) {
                pixel.r = (unsigned char)(base_color.r * 0.6f);
                pixel.g = (unsigned char)(base_color.g * 0.6f);
                pixel.b = (unsigned char)(base_color.b * 0.6f);
            } else {
                // Horizontal wood grain noise
                int grain = ((x * 3 + y) % 12) - 6;
                pixel.r = (unsigned char)clamp_int(pixel.r + grain, 0, 255);
                pixel.g = (unsigned char)clamp_int(pixel.g + grain, 0, 255);
                pixel.b = (unsigned char)clamp_int(pixel.b + grain / 2, 0, 255);
            }

            // Border
            if (x == 0 || y == 0 || x == TILE_SIZE - 1 || y == TILE_SIZE - 1) {
                pixel.r = (unsigned char)(base_color.r * 0.5f);
                pixel.g = (unsigned char)(base_color.g * 0.5f);
                pixel.b = (unsigned char)(base_color.b * 0.5f);
            }

            ImageDrawPixel(atlas, start_x + x, start_y + y, pixel);
        }
    }
}

/**
 * Generate a bed tile with mattress, sheet, and pillow
 */
static void generate_bed_tile(Image* atlas, int tile_x, int tile_y, Color wool_color, Color sheet_color) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    // Bed frame (wood)
    Color frame_dark = (Color){100, 70, 40, 255};
    Color frame_light = (Color){130, 90, 50, 255};

    // Draw mattress (wool color)
    for (int y = 4; y <= 12; y++) {
        for (int x = 0; x <= 15; x++) {
            int noise = (x * 7 + y * 13) % 10 - 5;
            Color pixel = {
                (unsigned char)clamp_int(wool_color.r + noise, 0, 255),
                (unsigned char)clamp_int(wool_color.g + noise, 0, 255),
                (unsigned char)clamp_int(wool_color.b + noise, 0, 255),
                255
            };
            ImageDrawPixel(atlas, start_x + x, start_y + y, pixel);
        }
    }

    // Draw sheet (sheet color with pillow)
    for (int y = 5; y <= 11; y++) {
        for (int x = 1; x <= 14; x++) {
            if (x >= 2 && x <= 5 && y >= 6 && y <= 9) {
                // Pillow (white)
                ImageDrawPixel(atlas, start_x + x, start_y + y, WHITE);
            } else {
                int noise = (x * 5 + y * 11) % 15 - 7;
                Color pixel = {
                    (unsigned char)clamp_int(sheet_color.r + noise, 0, 255),
                    (unsigned char)clamp_int(sheet_color.g + noise, 0, 255),
                    (unsigned char)clamp_int(sheet_color.b + noise, 0, 255),
                    255
                };
                ImageDrawPixel(atlas, start_x + x, start_y + y, pixel);
            }
        }
    }

    // Draw frame edges
    for (int x = 0; x <= 15; x++) {
        ImageDrawPixel(atlas, start_x + x, start_y + 13, frame_dark);
        ImageDrawPixel(atlas, start_x + x, start_y + 12, frame_light);
    }
}

/**
 * Generate a door tile with frame and optional window
 */
static void generate_door_tile(Image* atlas, int tile_x, int tile_y, Color door_color, Color frame_color, bool has_window) {
    int start_x = tile_x * TILE_SIZE;
    int start_y = tile_y * TILE_SIZE;

    // Draw door frame
    for (int x = 1; x <= 14; x++) {
        for (int y = 0; y <= 15; y++) {
            // Edge/frame
            if (x == 1 || x == 14 || y == 0 || y == 15) {
                int noise = (x * 3 + y * 7) % 20 - 10;
                Color pixel = {
                    (unsigned char)clamp_int(frame_color.r + noise, 0, 255),
                    (unsigned char)clamp_int(frame_color.g + noise, 0, 255),
                    (unsigned char)clamp_int(frame_color.b + noise, 0, 255),
                    255
                };
                ImageDrawPixel(atlas, start_x + x, start_y + y, pixel);
            } else {
                // Door surface
                int noise = (x * 5 + y * 11) % 15 - 7;
                Color pixel = {
                    (unsigned char)clamp_int(door_color.r + noise, 0, 255),
                    (unsigned char)clamp_int(door_color.g + noise, 0, 255),
                    (unsigned char)clamp_int(door_color.b + noise, 0, 255),
                    255
                };
                ImageDrawPixel(atlas, start_x + x, start_y + y, pixel);
            }
        }
    }

    // Window (for iron door)
    if (has_window) {
        for (int x = 5; x <= 10; x++) {
            for (int y = 2; y <= 6; y++) {
                ImageDrawPixel(atlas, start_x + x, start_y + y, (Color){150, 200, 220, 200});
            }
        }
    }

    // Door handle
    for (int x = 11; x <= 12; x++) {
        for (int y = 7; y <= 8; y++) {
            ImageDrawPixel(atlas, start_x + x, start_y + y, (Color){80, 60, 40, 255});
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
    generate_tile(&atlas, 1, 3, (Color){150, 110, 70, 255}, true);    // Top/Bottom: Wood rings (darker to match block)
    generate_planks_tile(&atlas, 3, 3, (Color){180, 130, 80, 255});   // Planks: Processed wood boards

    // LEAVES - Row 4 (Bright foliage green with transparency holes - Luanti style)
    generate_leaf_tile(&atlas, 0, 4, (Color){60, 180, 75, 255});     // All faces: Semi-transparent leaves

    // SAND - Row 5 (Warm golden sand)
    generate_tile(&atlas, 0, 5, (Color){240, 220, 130, 255}, true);   // All faces: Golden sand

    // WATER - Row 6 (Bright cyan-blue) - 4 animation frames
    Color water_color = {50, 150, 255, 180};
    generate_water_tile(&atlas, 0, 6, water_color, 0);  // Frame 0
    generate_water_tile(&atlas, 1, 6, water_color, 1);  // Frame 1
    generate_water_tile(&atlas, 2, 6, water_color, 2);  // Frame 2
    generate_water_tile(&atlas, 3, 6, water_color, 3);  // Frame 3

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

    // BIRCH_WOOD - Row 21 (white/cream bark with darker rings)
    generate_tile(&atlas, 0, 21, (Color){230, 225, 210, 255}, true);  // Sides: Cream bark
    generate_tile(&atlas, 1, 21, (Color){200, 180, 150, 255}, true);  // Top: Wood rings

    // BIRCH_LEAVES - Row 22 (lighter green)
    generate_leaf_tile(&atlas, 0, 22, (Color){100, 200, 90, 255});

    // SPRUCE_WOOD - Row 23 (dark brown bark)
    generate_tile(&atlas, 0, 23, (Color){60, 40, 25, 255}, true);     // Sides: Dark bark
    generate_tile(&atlas, 1, 23, (Color){120, 90, 60, 255}, true);    // Top: Wood rings

    // SPRUCE_LEAVES - Row 24 (dark blue-green needles)
    generate_leaf_tile(&atlas, 0, 24, (Color){30, 80, 50, 255});

    // ACACIA_WOOD - Row 25 (orange-brown bark)
    generate_tile(&atlas, 0, 25, (Color){170, 100, 50, 255}, true);   // Sides: Orange bark
    generate_tile(&atlas, 1, 25, (Color){180, 130, 80, 255}, true);   // Top: Wood rings

    // ACACIA_LEAVES - Row 26 (yellow-green)
    generate_leaf_tile(&atlas, 0, 26, (Color){140, 180, 60, 255});

    // STALACTITE - Row 27 (gray pointed stone, darker than regular stone)
    generate_tile(&atlas, 0, 27, (Color){100, 100, 105, 255}, true);

    // STALAGMITE - Row 28 (gray pointed stone, slightly different shade)
    generate_tile(&atlas, 0, 28, (Color){95, 95, 100, 255}, true);

    // CHEST - Row 29 (brown wood with darker band)
    generate_tile(&atlas, 0, 29, (Color){139, 90, 43, 255}, true);   // Sides: Wood
    generate_tile(&atlas, 1, 29, (Color){100, 60, 30, 255}, true);   // Top: Darker lid

    // =========================================================================
    // ITEM TEXTURES (Columns 3+)
    // =========================================================================

    // Tool head colors
    Color wood_color = {160, 120, 70, 255};    // Warm wood color for tool heads
    Color stone_color = {140, 140, 145, 255};  // Gray stone color for tool heads

    // PICKAXES - Column 3
    generate_pickaxe_tile(&atlas, 3, 0, wood_color);   // Wooden pickaxe
    generate_pickaxe_tile(&atlas, 3, 1, stone_color);  // Stone pickaxe

    // SHOVELS - Column 4
    generate_shovel_tile(&atlas, 4, 0, wood_color);    // Wooden shovel
    generate_shovel_tile(&atlas, 4, 1, stone_color);   // Stone shovel

    // AXES - Column 5
    generate_axe_tile(&atlas, 5, 0, wood_color);       // Wooden axe
    generate_axe_tile(&atlas, 5, 1, stone_color);      // Stone axe

    // SWORDS - Column 7
    generate_sword_tile(&atlas, 7, 0, wood_color);     // Wooden sword
    generate_sword_tile(&atlas, 7, 1, stone_color);    // Stone sword
    generate_sword_tile(&atlas, 7, 2, (Color){200, 200, 210, 255});  // Iron sword
    generate_sword_tile(&atlas, 7, 3, (Color){150, 220, 255, 255});  // Diamond sword

    // MEAT - Raw meat item (pinkish-red)
    generate_tile(&atlas, 6, 0, (Color){200, 100, 100, 255}, true);   // Raw meat

    // STICK - Crafting material
    generate_stick_tile(&atlas, 2, 3);  // Stick at (2, 3) - already correct in item.c

    // HAND - For first-person held item display
    generate_hand_tile(&atlas, 8, 0);   // Hand at (8, 0)

    // CRACK TEXTURES - Row 20 (10 stages of block damage)
    for (int i = 0; i < 10; i++) {
        generate_crack_tile(&atlas, i, i, 20);  // Columns 0-9, Row 20
    }

    // BEDS - Rows 30-45 (16 colors)
    const Color bed_colors[] = {
        {255, 255, 255, 255},  // White
        {180, 180, 180, 255},  // Light Gray
        {100, 100, 100, 255},  // Gray
        {30, 30, 30, 255},     // Black
        {120, 80, 40, 255},    // Brown
        {200, 50, 50, 255},    // Red
        {220, 120, 30, 255},   // Orange
        {240, 240, 80, 255},   // Yellow
        {100, 200, 80, 255},   // Lime
        {50, 150, 50, 255},    // Green
        {50, 200, 200, 255},   // Cyan
        {80, 150, 220, 255},   // Light Blue
        {50, 50, 200, 255},    // Blue
        {150, 50, 220, 255},   // Purple
        {220, 50, 180, 255},   // Magenta
        {240, 150, 180, 255},  // Pink
    };
    Color sheet_color = {220, 220, 255, 255};  // Light blue sheet
    for (int i = 0; i < 16; i++) {
        generate_bed_tile(&atlas, 0, 30 + i, bed_colors[i], sheet_color);
    }

    // WOOL - Rows 30-45 (using same colors as beds, column 1)
    for (int i = 0; i < 16; i++) {
        generate_tile(&atlas, 1, 30 + i, bed_colors[i], true);
    }

    // DOORS - Row 46
    generate_door_tile(&atlas, 0, 46, (Color){130, 90, 50, 255}, (Color){100, 70, 40, 255}, false);  // Wood door
    generate_door_tile(&atlas, 1, 46, (Color){180, 180, 190, 255}, (Color){150, 150, 160, 255}, true); // Iron door with window

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

        case BLOCK_BIRCH_WOOD:
            if (face == FACE_TOP || face == FACE_BOTTOM) {
                tile_x = 1; tile_y = 21;  // Wood rings
            } else {
                tile_x = 0; tile_y = 21;  // Cream bark
            }
            break;

        case BLOCK_BIRCH_LEAVES:
            tile_x = 0; tile_y = 22;
            break;

        case BLOCK_SPRUCE_WOOD:
            if (face == FACE_TOP || face == FACE_BOTTOM) {
                tile_x = 1; tile_y = 23;  // Wood rings
            } else {
                tile_x = 0; tile_y = 23;  // Dark bark
            }
            break;

        case BLOCK_SPRUCE_LEAVES:
            tile_x = 0; tile_y = 24;
            break;

        case BLOCK_ACACIA_WOOD:
            if (face == FACE_TOP || face == FACE_BOTTOM) {
                tile_x = 1; tile_y = 25;  // Wood rings
            } else {
                tile_x = 0; tile_y = 25;  // Orange bark
            }
            break;

        case BLOCK_ACACIA_LEAVES:
            tile_x = 0; tile_y = 26;
            break;

        case BLOCK_STALACTITE:
            tile_x = 0; tile_y = 27;
            break;

        case BLOCK_STALAGMITE:
            tile_x = 0; tile_y = 28;
            break;

        case BLOCK_CHEST:
            if (face == FACE_TOP || face == FACE_BOTTOM) {
                tile_x = 1; tile_y = 29;  // Darker lid
            } else {
                tile_x = 0; tile_y = 29;  // Wood sides
            }
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
