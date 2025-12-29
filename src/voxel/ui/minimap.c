/**
 * Minimap System Implementation
 */

#include "voxel/ui/minimap.h"
#include "voxel/core/block.h"
#include "voxel/world/chunk.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// MINIMAP STRUCTURE
// ============================================================================

struct Minimap {
    RenderTexture2D texture;     // Render target for minimap
    int size;                    // Pixel size
    int radius;                  // Block radius to display
    int last_player_x;           // Last player position for dirty check
    int last_player_z;
    bool initialized;            // Has been initialized
};

// ============================================================================
// BLOCK COLOR MAPPING
// ============================================================================

static Color get_block_color(BlockType type, int height) {
    Color base;

    switch (type) {
        case BLOCK_GRASS:       base = (Color){34, 139, 34, 255}; break;    // Forest green
        case BLOCK_DIRT:        base = (Color){139, 90, 43, 255}; break;    // Brown
        case BLOCK_STONE:       base = (Color){128, 128, 128, 255}; break;  // Gray
        case BLOCK_WOOD:        base = (Color){139, 69, 19, 255}; break;    // Saddle brown
        case BLOCK_LEAVES:      base = (Color){0, 100, 0, 255}; break;      // Dark green
        case BLOCK_SAND:        base = (Color){238, 214, 175, 255}; break;  // Sandy beige
        case BLOCK_WATER:       base = (Color){30, 144, 255, 255}; break;   // Dodger blue
        case BLOCK_COBBLESTONE: base = (Color){100, 100, 100, 255}; break;  // Dark gray
        case BLOCK_BEDROCK:     base = (Color){40, 40, 40, 255}; break;     // Very dark gray
        case BLOCK_DEEP_STONE:  base = (Color){80, 80, 80, 255}; break;     // Medium dark gray
        case BLOCK_GRAVEL:      base = (Color){150, 150, 150, 255}; break;  // Light gray
        case BLOCK_COAL_ORE:    base = (Color){50, 50, 50, 255}; break;     // Coal black
        case BLOCK_IRON_ORE:    base = (Color){180, 140, 100, 255}; break;  // Iron tan
        case BLOCK_GOLD_ORE:    base = (Color){255, 215, 0, 255}; break;    // Gold
        case BLOCK_DIAMOND_ORE: base = (Color){0, 255, 255, 255}; break;    // Cyan
        case BLOCK_MOSSY_COBBLE: base = (Color){80, 110, 80, 255}; break;   // Mossy gray-green
        case BLOCK_STONE_BRICK:  base = (Color){140, 140, 140, 255}; break; // Light stone
        case BLOCK_CRACKED_BRICK: base = (Color){110, 100, 100, 255}; break; // Darker brick
        case BLOCK_CLAY:         base = (Color){180, 140, 120, 255}; break; // Clay tan
        case BLOCK_SNOW:         base = (Color){240, 250, 255, 255}; break; // White/ice blue
        case BLOCK_CACTUS:       base = (Color){50, 180, 50, 255}; break;   // Cactus green
        case BLOCK_AIR:
        default:                base = (Color){0, 0, 0, 255}; break;        // Black
    }

    // Height-based brightness adjustment
    // Base height around 64, brighter for higher, darker for lower
    float brightness = 0.6f + (height - 32.0f) / 128.0f * 0.4f;
    if (brightness < 0.4f) brightness = 0.4f;
    if (brightness > 1.0f) brightness = 1.0f;

    return (Color){
        (unsigned char)(base.r * brightness),
        (unsigned char)(base.g * brightness),
        (unsigned char)(base.b * brightness),
        255
    };
}

// ============================================================================
// MINIMAP LIFECYCLE
// ============================================================================

Minimap* minimap_create(void) {
    Minimap* minimap = (Minimap*)malloc(sizeof(Minimap));
    if (!minimap) return NULL;

    minimap->size = MINIMAP_SIZE;
    minimap->radius = MINIMAP_RADIUS;
    minimap->last_player_x = -9999;  // Force initial update
    minimap->last_player_z = -9999;
    minimap->initialized = false;

    // Create render texture
    minimap->texture = LoadRenderTexture(MINIMAP_SIZE, MINIMAP_SIZE);

    // Clear it initially
    BeginTextureMode(minimap->texture);
    ClearBackground(BLACK);
    EndTextureMode();

    minimap->initialized = true;
    printf("[MINIMAP] Created (%dx%d, radius=%d blocks)\n",
           minimap->size, minimap->size, minimap->radius);

    return minimap;
}

void minimap_destroy(Minimap* minimap) {
    if (!minimap) return;

    if (minimap->initialized) {
        UnloadRenderTexture(minimap->texture);
    }

    free(minimap);
    printf("[MINIMAP] Destroyed\n");
}

// ============================================================================
// MINIMAP UPDATE
// ============================================================================

void minimap_update(Minimap* minimap, World* world, Player* player) {
    if (!minimap || !world || !player) return;

    int player_x = (int)player->position.x;
    int player_z = (int)player->position.z;

    // Only update if player moved at least 4 blocks (performance optimization)
    int dx = player_x - minimap->last_player_x;
    int dz = player_z - minimap->last_player_z;
    if (dx * dx + dz * dz < 16) {
        return;  // No significant movement
    }

    minimap->last_player_x = player_x;
    minimap->last_player_z = player_z;

    // Begin drawing to render texture
    BeginTextureMode(minimap->texture);
    ClearBackground((Color){20, 20, 30, 255});  // Dark blue-gray for unloaded areas

    // Calculate scale (pixels per block)
    float scale = (float)minimap->size / (minimap->radius * 2.0f);
    int half_size = minimap->size / 2;

    // Draw terrain centered on player
    for (int dz_block = -minimap->radius; dz_block < minimap->radius; dz_block++) {
        for (int dx_block = -minimap->radius; dx_block < minimap->radius; dx_block++) {
            int world_x = player_x + dx_block;
            int world_z = player_z + dz_block;

            // Find surface block (scan from top down)
            // Start from 200 to cover terrain surface at y=160
            int surface_y = 0;
            BlockType surface_type = BLOCK_AIR;

            for (int y = 200; y >= 0; y--) {
                Block block = world_get_block(world, world_x, y, world_z);
                if (block.type != BLOCK_AIR) {
                    surface_y = y;
                    surface_type = (BlockType)block.type;
                    break;
                }
            }

            // Map to pixel position
            int px = half_size + (int)(dx_block * scale);
            int py = half_size + (int)(dz_block * scale);

            // Only draw if within bounds
            if (px >= 0 && px < minimap->size && py >= 0 && py < minimap->size) {
                Color color = get_block_color(surface_type, surface_y);

                // Draw a small rectangle if scale > 1 for smoother look
                if (scale >= 1.0f) {
                    int rect_size = (int)ceilf(scale);
                    DrawRectangle(px, py, rect_size, rect_size, color);
                } else {
                    DrawPixel(px, py, color);
                }
            }
        }
    }

    EndTextureMode();
}

// ============================================================================
// MINIMAP RENDERING
// ============================================================================

void minimap_draw(Minimap* minimap, Player* player, NetworkContext* network) {
    if (!minimap || !player) return;

    int screen_width = 800;  // Match game window size
    int x = screen_width - minimap->size - MINIMAP_MARGIN;
    int y = MINIMAP_MARGIN;

    // Draw dark background/border
    DrawRectangle(x - 3, y - 3, minimap->size + 6, minimap->size + 6, (Color){0, 0, 0, 200});

    // Draw minimap texture
    // Note: RenderTexture is flipped vertically, so we use negative height in source rect
    Rectangle src = {0, 0, (float)minimap->size, -(float)minimap->size};
    Rectangle dst = {(float)x, (float)y, (float)minimap->size, (float)minimap->size};
    DrawTexturePro(minimap->texture.texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);

    // Calculate scale for position mapping
    float scale = (float)minimap->size / (minimap->radius * 2.0f);
    int center_x = x + minimap->size / 2;
    int center_y = y + minimap->size / 2;
    int player_x = (int)player->position.x;
    int player_z = (int)player->position.z;

    // Draw remote players as colored dots
    if (network) {
        NetworkMode mode = network_get_mode(network);

        if (mode == NET_MODE_HOST && network->server) {
            // Draw connected clients
            for (int i = 0; i < NET_MAX_CLIENTS; i++) {
                if (network->server->clients[i].connected) {
                    NetPlayerState* state = &network->server->clients[i].last_state;
                    int dx = (int)state->pos_x - player_x;
                    int dz = (int)state->pos_z - player_z;

                    // Check if within minimap radius
                    if (abs(dx) < minimap->radius && abs(dz) < minimap->radius) {
                        int px = center_x + (int)(dx * scale);
                        int py = center_y + (int)(dz * scale);

                        // Draw player dot (blue for other players)
                        DrawCircle(px, py, 4, (Color){0, 150, 255, 255});
                        DrawCircleLines(px, py, 4, WHITE);
                    }
                }
            }
        } else if (mode == NET_MODE_CLIENT && network->client) {
            // Draw other remote players
            for (int i = 0; i < NET_MAX_CLIENTS; i++) {
                if (network->client->player_active[i] &&
                    i != network->client->my_client_id) {
                    NetPlayerState* state = &network->client->remote_players[i];
                    int dx = (int)state->pos_x - player_x;
                    int dz = (int)state->pos_z - player_z;

                    // Check if within minimap radius
                    if (abs(dx) < minimap->radius && abs(dz) < minimap->radius) {
                        int px = center_x + (int)(dx * scale);
                        int py = center_y + (int)(dz * scale);

                        // Draw player dot (blue for other players)
                        DrawCircle(px, py, 4, (Color){0, 150, 255, 255});
                        DrawCircleLines(px, py, 4, WHITE);
                    }
                }
            }

            // Draw host player
            if (network->client->player_active[0]) {
                NetPlayerState* state = &network->client->remote_players[0];
                int dx = (int)state->pos_x - player_x;
                int dz = (int)state->pos_z - player_z;

                if (abs(dx) < minimap->radius && abs(dz) < minimap->radius) {
                    int px = center_x + (int)(dx * scale);
                    int py = center_y + (int)(dz * scale);

                    // Draw host as green dot
                    DrawCircle(px, py, 4, (Color){0, 255, 100, 255});
                    DrawCircleLines(px, py, 4, WHITE);
                }
            }
        }
    }

    // Draw local player arrow in center
    // Player yaw: 0 = looking at -Z, 90 = looking at +X
    float angle = player->yaw * DEG2RAD;

    // Arrow dimensions
    float arrow_length = 10.0f;
    float arrow_width = 6.0f;

    // Calculate arrow points
    float tip_x = center_x + sinf(angle) * arrow_length;
    float tip_y = center_y - cosf(angle) * arrow_length;

    float back_x = center_x - sinf(angle) * (arrow_length * 0.3f);
    float back_y = center_y + cosf(angle) * (arrow_length * 0.3f);

    float left_x = back_x + cosf(angle) * arrow_width;
    float left_y = back_y + sinf(angle) * arrow_width;

    float right_x = back_x - cosf(angle) * arrow_width;
    float right_y = back_y - sinf(angle) * arrow_width;

    Vector2 tip = {tip_x, tip_y};
    Vector2 left = {left_x, left_y};
    Vector2 right = {right_x, right_y};

    // Draw arrow with outline for visibility
    DrawTriangle(tip, right, left, WHITE);
    DrawTriangleLines(tip, right, left, BLACK);

    // Draw white border
    DrawRectangleLines(x, y, minimap->size, minimap->size, WHITE);

    // Draw cardinal directions
    int font_size = 10;
    DrawText("N", x + minimap->size / 2 - 3, y + 2, font_size, WHITE);
}
