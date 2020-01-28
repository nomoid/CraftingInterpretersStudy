#ifndef clox_endian_h
#define clox_endian_h

#include "settings.h"

#ifdef CLOX_LITTLE_ENDIAN
// Little endian definition
#define BYTE_FROM_3WORD(word, byteNum) \
    (((size_t) (word)) >> (8 * (byteNum)) & ((uint8_t) 0xFF))
#define COMBINE_3WORD(part1, part2, part3) \
    ((size_t)(part1) | (size_t)((part2) << 8) | (size_t)((part3) << (8 * 2)))
#else
// Big endian definition
#define BYTE_FROM_3WORD(word, byteNum) \
    (((size_t) (word)) >> (8 * (2 - byteNum)) & ((uint8_t) 0xFF))
#define COMBINE_3WORD(part1, part2, part3) \
    ((size_t)(part3) | (size_t)((part2) << 8) | (size_t)((part1) << (8 * 2)))
#endif

#endif