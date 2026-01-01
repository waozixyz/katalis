/**
 * Entity Utilities Implementation
 *
 * Shared helper functions for entity implementations
 */

#include "voxel/entity/entity_utils.h"
#include <stdlib.h>
#include <math.h>

float entity_random_range(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

Vector3 entity_random_direction(void) {
    float angle = entity_random_range(0, 2.0f * PI);
    return (Vector3){cosf(angle), 0, sinf(angle)};
}

Color entity_apply_ambient(Color base, Vector3 ambient) {
    return (Color){
        (unsigned char)(base.r * ambient.x),
        (unsigned char)(base.g * ambient.y),
        (unsigned char)(base.b * ambient.z),
        base.a
    };
}
