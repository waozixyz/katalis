/**
 * Serialization Utilities Implementation
 *
 * Binary read/write helpers for network packet serialization
 * Uses little-endian byte order for cross-platform compatibility
 */

#include "voxel/network/serialization.h"
#include <string.h>

// ============================================================================
// WRITE FUNCTIONS
// ============================================================================

void ser_write_u8(uint8_t** buf, uint8_t val) {
    **buf = val;
    (*buf)++;
}

void ser_write_u16(uint8_t** buf, uint16_t val) {
    (*buf)[0] = val & 0xFF;
    (*buf)[1] = (val >> 8) & 0xFF;
    (*buf) += 2;
}

void ser_write_u32(uint8_t** buf, uint32_t val) {
    (*buf)[0] = val & 0xFF;
    (*buf)[1] = (val >> 8) & 0xFF;
    (*buf)[2] = (val >> 16) & 0xFF;
    (*buf)[3] = (val >> 24) & 0xFF;
    (*buf) += 4;
}

void ser_write_i32(uint8_t** buf, int32_t val) {
    ser_write_u32(buf, (uint32_t)val);
}

void ser_write_f32(uint8_t** buf, float val) {
    union { float f; uint32_t u; } conv;
    conv.f = val;
    ser_write_u32(buf, conv.u);
}

void ser_write_string(uint8_t** buf, const char* str, size_t max_len) {
    size_t len = strlen(str);
    if (len > max_len - 1) len = max_len - 1;
    memcpy(*buf, str, len);
    memset(*buf + len, 0, max_len - len);
    (*buf) += max_len;
}

// ============================================================================
// READ FUNCTIONS
// ============================================================================

uint8_t ser_read_u8(const uint8_t** buf) {
    uint8_t val = **buf;
    (*buf)++;
    return val;
}

uint16_t ser_read_u16(const uint8_t** buf) {
    uint16_t val = (*buf)[0] | ((*buf)[1] << 8);
    (*buf) += 2;
    return val;
}

uint32_t ser_read_u32(const uint8_t** buf) {
    uint32_t val = (*buf)[0] | ((*buf)[1] << 8) | ((*buf)[2] << 16) | ((*buf)[3] << 24);
    (*buf) += 4;
    return val;
}

int32_t ser_read_i32(const uint8_t** buf) {
    return (int32_t)ser_read_u32(buf);
}

float ser_read_f32(const uint8_t** buf) {
    union { float f; uint32_t u; } conv;
    conv.u = ser_read_u32(buf);
    return conv.f;
}

void ser_read_string(const uint8_t** buf, char* out, size_t max_len) {
    memcpy(out, *buf, max_len);
    out[max_len - 1] = '\0';
    (*buf) += max_len;
}
