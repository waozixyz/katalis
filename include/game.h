/**
 * Game - Love2D-Style Lifecycle
 *
 * Entry point for game logic with init/update/draw lifecycle
 */

#ifndef GAME_H
#define GAME_H

#include <stdint.h>

/**
 * Game Entry Point - Love2D-style lifecycle
 *
 * Called every frame by main loop.
 * Manages init/update/draw lifecycle internally.
 *
 * Lifecycle:
 *   - game_init()        Called once on first frame
 *   - game_update(dt)    Called every frame before draw
 *   - game_draw()        Renderer-agnostic drawing
 *   - game_draw2d()      Raylib 2D camera mode (optional)
 *   - game_draw3d()      Raylib 3D mode (optional)
 */
void game_run(void);

/**
 * Clean up all game resources
 * Call this BEFORE CloseWindow() to free GPU resources
 */
void game_shutdown(void);

#endif // GAME_H
