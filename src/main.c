/**
 * Katalis
 *
 * Title screen with navigation to game screen
 */

#include <kryon_dsl.h>
#include <stdbool.h>
#include <raylib.h>

// Only include desktop renderer for standalone builds
#ifndef KRYON_KIR_ONLY
#include <ir_desktop_renderer.h>
#endif

// Global state
static IRComponent* title_screen = NULL;
static IRComponent* game_screen = NULL;

// Native canvas render callback for game rendering
void render_game_canvas(uint32_t canvas_id) {
    // This function is called every frame during rendering
    // Full Raylib API is available here

    // Example: Draw a simple game scene
    DrawText("Native Raylib Canvas!", 50, 50, 32, WHITE);
    DrawCircle(200, 200, 50, YELLOW);
    DrawRectangle(300, 150, 100, 80, BLUE);

    // You can add full game logic here:
    // - Draw sprites
    // - Render physics
    // - Custom shaders
    // - Particle systems
    // etc.
}

// Event handler - switch from title to game screen
void on_start_game(void) {
    if (title_screen) {
        kryon_set_visible(title_screen, false);
    }
    if (game_screen) {
        kryon_set_visible(game_screen, true);
    }
}

int main(void) {
    kryon_init("Katalis", 800, 600);

    printf("[MAIN] Creating UI tree...\n");

    KRYON_APP(
        COLUMN(
            FULL_SIZE,
            BG_COLOR(0x0a0e27),

            // Title Screen (initially visible)
            title_screen = COLUMN(
                FULL_SIZE,
                JUSTIFY_CENTER,
                ALIGN_CENTER,
                GAP(40),
                PADDING(60),

                // Title "KATALIS"
                TEXT("KATALIS",
                    COLOR_YELLOW,
                    FONT_SIZE(72),
                    FONT_BOLD
                ),

                // Button container - positioned to the left
                ROW(
                    WIDTH_PCT(100),
                    JUSTIFY_START,
                    PADDING_TRBL(0, 0, 0, 120),

                    BUTTON("Start Game",
                        WIDTH(200),
                        HEIGHT(60),
                        BG_COLOR(0x6366f1),
                        COLOR_WHITE,
                        ON_CLICK(on_start_game)
                    )
                )
            ),

            // Game Screen (initially hidden)
            game_screen = NATIVE_CANVAS(render_game_canvas,
                FULL_SIZE,
                BG_COLOR(0x1a1a2e),
                VISIBLE(false)
            )
        
        )
    );

    printf("[MAIN] UI tree created\n");
    printf("[MAIN] title_screen = %p (id=%u)\n", (void*)title_screen, title_screen ? title_screen->id : 0);
    printf("[MAIN] game_screen = %p (id=%u)\n", (void*)game_screen, game_screen ? game_screen->id : 0);
    if (title_screen) {
        printf("[MAIN] title_screen->style = %p\n", (void*)title_screen->style);
        if (title_screen->style) {
            printf("[MAIN] title_screen visible = %d\n", title_screen->style->visible);
        }
    }
    if (game_screen) {
        printf("[MAIN] game_screen->style = %p\n", (void*)game_screen->style);
        if (game_screen->style) {
            printf("[MAIN] game_screen visible = %d\n", game_screen->style->visible);
        } else {
            printf("[MAIN] ERROR: game_screen has no style!\n");
        }
    }

    KRYON_RUN();
}
