/**
 * Serialization Utilities
 *
 * Binary read/write helpers for network packet serialization
 */

#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdint.h>
#include <stddef.h>

// Write functions - advance buffer pointer after writing
void ser_write_u8(uint8_t** buf, uint8_t val);
void ser_write_u16(uint8_t** buf, uint16_t val);
void ser_write_u32(uint8_t** buf, uint32_t val);
void ser_write_i32(uint8_t** buf, int32_t val);
void ser_write_f32(uint8_t** buf, float val);
void ser_write_string(uint8_t** buf, const char* str, size_t max_len);

// Read functions - advance buffer pointer after reading
uint8_t ser_read_u8(const uint8_t** buf);
uint16_t ser_read_u16(const uint8_t** buf);
uint32_t ser_read_u32(const uint8_t** buf);
int32_t ser_read_i32(const uint8_t** buf);
float ser_read_f32(const uint8_t** buf);
void ser_read_string(const uint8_t** buf, char* out, size_t max_len);

#endif // SERIALIZATION_H
