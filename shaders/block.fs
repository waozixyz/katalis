#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragWorldPos;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec3 u_ambient_light;

// Fog uniforms
uniform vec3 u_camera_pos;
uniform float u_fog_start;
uniform float u_fog_end;
uniform vec3 u_fog_color;

// Underwater effect
uniform int u_underwater;

// Water animation
uniform float u_time;

// Atlas constants (must match texture_atlas.h)
const float TILES_PER_ROW = 32.0;
const float TILE_UV_SIZE = 1.0 / TILES_PER_ROW;

void main() {
    vec2 texCoord = fragTexCoord;

    // Water animation: detect if UV is in water row (row 6) and animate
    // Water is at row 6, columns 0-3 (4 animation frames)
    float water_row_start = 6.0 * TILE_UV_SIZE;
    float water_row_end = 7.0 * TILE_UV_SIZE;

    if (texCoord.y >= water_row_start && texCoord.y < water_row_end) {
        // Calculate current animation frame (4 frames, 4 FPS = 0.25s per frame)
        float frame = floor(mod(u_time * 4.0, 4.0));

        // Get position within the current tile (0-1 within tile)
        float tile_local_u = mod(texCoord.x, TILE_UV_SIZE);

        // Offset U to the correct frame (frames are at columns 0,1,2,3)
        texCoord.x = frame * TILE_UV_SIZE + tile_local_u;
    }

    vec4 texColor = texture(texture0, texCoord);

    // Apply vertex color (from greedy meshing) and ambient light
    vec3 color = texColor.rgb * fragColor.rgb * u_ambient_light;

    // Calculate distance fog
    float dist = distance(fragWorldPos, u_camera_pos);
    float fogFactor = clamp((dist - u_fog_start) / (u_fog_end - u_fog_start), 0.0, 1.0);

    // Smooth quadratic fog curve
    fogFactor = fogFactor * fogFactor;

    // Apply underwater effects if submerged
    if (u_underwater == 1) {
        // Blue tint underwater
        color = mix(color, vec3(0.2, 0.4, 0.8), 0.3);

        // Much shorter fog distance underwater
        float underwaterFogFactor = clamp(dist / 32.0, 0.0, 1.0);
        underwaterFogFactor = underwaterFogFactor * underwaterFogFactor;
        color = mix(color, vec3(0.1, 0.3, 0.5), underwaterFogFactor);
    } else {
        // Normal fog
        color = mix(color, u_fog_color, fogFactor);
    }

    finalColor = vec4(color, texColor.a);
}
