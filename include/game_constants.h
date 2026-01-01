/**
 * Game Constants
 *
 * Centralized constants for game logic and rendering
 */

#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

// Player interaction distances
#define PLAYER_REACH_DISTANCE 5.0f     // Max distance for block targeting
#define ENTITY_REACH_DISTANCE 4.0f     // Max distance for entity targeting

// Player dimensions
#define PLAYER_EYE_HEIGHT 1.5f         // Height of camera above feet

// Block positioning
#define BLOCK_CENTER_OFFSET 0.5f       // Offset from block origin to center

// Rendering
#define ZFIGHT_OFFSET 0.002f           // Small offset to prevent z-fighting
#define WIREFRAME_SCALE 1.01f          // Scale for selection wireframe

// UI timing
#define MESSAGE_DISPLAY_TIME 2.0f      // How long notifications stay on screen

// Crack overlay stages
#define CRACK_STAGE_COUNT 10           // Number of crack overlay stages (0-9)

#endif // GAME_CONSTANTS_H
