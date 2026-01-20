#define _POSIX_C_SOURCE 200112L
/**
 * Katalis
 *
 * Direct Raylib initialization - Kryon framework removed
 */

#include <raylib.h>
#include <stdio.h>
#include "game.h"

int main(void) {
    // Initialize Raylib window
    const int screen_width = 800;
    const int screen_height = 600;
    InitWindow(screen_width, screen_height, "Katalis");
    SetTargetFPS(60);

    printf("[MAIN] Katalis starting...\n");
    printf("[MAIN] Window initialized: %dx%d\n", screen_width, screen_height);

    // Main game loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        game_run();
        EndDrawing();
    }

    printf("[MAIN] Window closed, cleaning up...\n");

    // Cleanup game resources before closing window
    game_shutdown();

    // Close window
    CloseWindow();

    printf("[MAIN] Katalis exited cleanly\n");
    return 0;
}
