/**
 * Noise Generation - Perlin/Simplex Noise
 *
 * Used for procedural terrain generation
 */

#ifndef VOXEL_NOISE_H
#define VOXEL_NOISE_H

#include <stdint.h>

// ============================================================================
// PERLIN NOISE
// ============================================================================

/**
 * Initialize noise generator with seed
 */
void noise_init(uint32_t seed);

/**
 * 2D Perlin noise - returns value between -1.0 and 1.0
 * Used for heightmaps
 */
float noise_2d(float x, float y);

/**
 * 3D Perlin noise - returns value between -1.0 and 1.0
 * Used for caves and 3D terrain features
 */
float noise_3d(float x, float y, float z);

/**
 * Fractional Brownian Motion (fBm) - layered noise
 * Combines multiple octaves for more natural terrain
 *
 * octaves: Number of noise layers (higher = more detail)
 * frequency: Base frequency multiplier
 * amplitude: Base amplitude multiplier
 * lacunarity: Frequency multiplier per octave (typically 2.0)
 * persistence: Amplitude multiplier per octave (typically 0.5)
 */
float noise_fbm_2d(float x, float y, int octaves, float frequency,
                   float amplitude, float lacunarity, float persistence);

float noise_fbm_3d(float x, float y, float z, int octaves, float frequency,
                   float amplitude, float lacunarity, float persistence);

#endif // VOXEL_NOISE_H
