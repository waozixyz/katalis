/**
 * Sky Rendering System
 * Implements atmospheric scattering for realistic sky colors
 */

#include "voxel/render/sky.h"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>
#include <math.h>

static Shader sky_shader;
static bool initialized = false;
static int sun_loc = -1;
static int time_loc = -1;
static int cam_forward_loc = -1;
static int cam_right_loc = -1;
static int cam_up_loc = -1;

void sky_init(void) {
    // Load sky shader
    sky_shader = LoadShader("shaders/sky.vs", "shaders/sky.fs");

    if (sky_shader.id > 0) {
        printf("[SKY] Sky shader loaded (ID: %d)\n", sky_shader.id);
        sun_loc = GetShaderLocation(sky_shader, "u_sun_direction");
        time_loc = GetShaderLocation(sky_shader, "u_time_of_day");
        cam_forward_loc = GetShaderLocation(sky_shader, "u_cam_forward");
        cam_right_loc = GetShaderLocation(sky_shader, "u_cam_right");
        cam_up_loc = GetShaderLocation(sky_shader, "u_cam_up");
    } else {
        printf("[SKY] WARNING: Sky shader failed to load\n");
    }

    initialized = true;
}

void sky_render(Camera3D camera, float time_of_day) {
    if (!initialized || sky_shader.id == 0) return;

    // Calculate sun direction from time
    // 6.0 = sunrise, 12.0 = noon (sun overhead), 18.0 = sunset, 0/24 = midnight
    float sun_angle = (time_of_day - 6.0f) / 24.0f * 2.0f * PI;
    Vector3 sun_direction = {
        0.0f,
        sinf(sun_angle),  // Y: -1 (midnight) to +1 (noon)
        cosf(sun_angle)
    };

    // Calculate camera basis vectors
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    // Set shader uniforms
    SetShaderValue(sky_shader, sun_loc, &sun_direction, SHADER_UNIFORM_VEC3);
    SetShaderValue(sky_shader, time_loc, &time_of_day, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sky_shader, cam_forward_loc, &forward, SHADER_UNIFORM_VEC3);
    SetShaderValue(sky_shader, cam_right_loc, &right, SHADER_UNIFORM_VEC3);
    SetShaderValue(sky_shader, cam_up_loc, &up, SHADER_UNIFORM_VEC3);

    // Draw fullscreen quad with sky shader
    BeginShaderMode(sky_shader);

    // Use Raylib's 2D drawing to create a fullscreen quad
    // The texture coords will go from (0,0) to (1,1)
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Draw a textured rectangle covering the whole screen
    // We use DrawTexturePro with a 1x1 white texture to get proper UV mapping
    rlSetTexture(rlGetTextureIdDefault());  // Use default white texture

    rlBegin(RL_QUADS);
        // Bottom-left
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex2f(0, screenHeight);

        // Bottom-right
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex2f(screenWidth, screenHeight);

        // Top-right
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex2f(screenWidth, 0);

        // Top-left
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex2f(0, 0);
    rlEnd();

    rlSetTexture(0);

    EndShaderMode();
}

void sky_destroy(void) {
    if (initialized && sky_shader.id > 0) {
        UnloadShader(sky_shader);
    }
    initialized = false;
}
