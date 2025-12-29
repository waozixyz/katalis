/**
 * Perlin Noise Implementation
 *
 * Based on Ken Perlin's improved noise (2002)
 */

#include "voxel/world/noise.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// ============================================================================
// PERMUTATION TABLE
// ============================================================================

static int permutation[512];
static int p[512];

/**
 * Initialize permutation table with seed
 */
void noise_init(uint32_t seed) {
    // Generate initial permutation
    for (int i = 0; i < 256; i++) {
        permutation[i] = i;
    }

    // Fisher-Yates shuffle with seed
    srand(seed);
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = permutation[i];
        permutation[i] = permutation[j];
        permutation[j] = temp;
    }

    // Duplicate for wrapping
    for (int i = 0; i < 256; i++) {
        p[i] = permutation[i];
        p[256 + i] = permutation[i];
    }

    printf("[NOISE] Initialized with seed %u\n", seed);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Fade function for smooth interpolation
 */
static float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/**
 * Linear interpolation
 */
static float lerp(float t, float a, float b) {
    return a + t * (b - a);
}

/**
 * Gradient function for 2D
 */
static float grad_2d(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

/**
 * Gradient function for 3D
 */
static float grad_3d(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

// ============================================================================
// PERLIN NOISE
// ============================================================================

float noise_2d(float x, float y) {
    // Find unit grid cell containing point
    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;

    // Get relative position within cell
    x -= floorf(x);
    y -= floorf(y);

    // Compute fade curves
    float u = fade(x);
    float v = fade(y);

    // Hash coordinates of the 4 corners
    int a = p[X] + Y;
    int b = p[X + 1] + Y;

    // Blend results from 4 corners
    float result = lerp(v,
        lerp(u, grad_2d(p[a], x, y), grad_2d(p[b], x - 1.0f, y)),
        lerp(u, grad_2d(p[a + 1], x, y - 1.0f), grad_2d(p[b + 1], x - 1.0f, y - 1.0f))
    );

    return result;
}

float noise_3d(float x, float y, float z) {
    // Find unit cube containing point
    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;
    int Z = (int)floorf(z) & 255;

    // Get relative position within cube
    x -= floorf(x);
    y -= floorf(y);
    z -= floorf(z);

    // Compute fade curves
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);

    // Hash coordinates of the 8 cube corners
    int a = p[X] + Y;
    int aa = p[a] + Z;
    int ab = p[a + 1] + Z;
    int b = p[X + 1] + Y;
    int ba = p[b] + Z;
    int bb = p[b + 1] + Z;

    // Blend results from 8 corners
    float result = lerp(w,
        lerp(v,
            lerp(u, grad_3d(p[aa], x, y, z), grad_3d(p[ba], x - 1.0f, y, z)),
            lerp(u, grad_3d(p[ab], x, y - 1.0f, z), grad_3d(p[bb], x - 1.0f, y - 1.0f, z))
        ),
        lerp(v,
            lerp(u, grad_3d(p[aa + 1], x, y, z - 1.0f), grad_3d(p[ba + 1], x - 1.0f, y, z - 1.0f)),
            lerp(u, grad_3d(p[ab + 1], x, y - 1.0f, z - 1.0f), grad_3d(p[bb + 1], x - 1.0f, y - 1.0f, z - 1.0f))
        )
    );

    return result;
}

// ============================================================================
// FRACTIONAL BROWNIAN MOTION (FBM)
// ============================================================================

float noise_fbm_2d(float x, float y, int octaves, float frequency,
                   float amplitude, float lacunarity, float persistence) {
    float sum = 0.0f;
    float freq = frequency;
    float amp = amplitude;

    for (int i = 0; i < octaves; i++) {
        sum += noise_2d(x * freq, y * freq) * amp;
        freq *= lacunarity;
        amp *= persistence;
    }

    return sum;
}

float noise_fbm_3d(float x, float y, float z, int octaves, float frequency,
                   float amplitude, float lacunarity, float persistence) {
    float sum = 0.0f;
    float freq = frequency;
    float amp = amplitude;

    for (int i = 0; i < octaves; i++) {
        sum += noise_3d(x * freq, y * freq, z * freq) * amp;
        freq *= lacunarity;
        amp *= persistence;
    }

    return sum;
}
