#include "bitfield.h"
#include "memory.h"

#define CONVERT_CAPACITY(capacity) (((capacity) + 7) / 8)

void initBitfield(Bitfield* bitfield, size_t capacity) {
    bitfield->data = ALLOCATE(uint8_t, CONVERT_CAPACITY(capacity), true);
    if (bitfield->data == NULL) {
        // TODO out of memory
        return;
    }
    bitfield->capacity = capacity;
}

void freeBitfield(Bitfield* bitfield) {
    FREE_ARRAY(uint8_t, bitfield->data, CONVERT_CAPACITY(bitfield->capacity));
    bitfield->data = NULL;
    bitfield->capacity = 0;
}

void resizeBitfield(Bitfield* bitfield, size_t capacity) {
    bitfield->data = GROW_ARRAY_ZERO(
        bitfield->data,
        uint8_t,
        CONVERT_CAPACITY(bitfield->capacity),
        CONVERT_CAPACITY(capacity));
    if (bitfield->data == NULL) {
        // TODO out of memory
        return;
    }
    bitfield->capacity = capacity;
}

void bitfieldSet(Bitfield* bitfield, size_t address) {
    // No bounds checking
    bitfield->data[address >> 3] |= (uint8_t) (0x1 << (address & 0x7));
}
void bitfieldClear(Bitfield* bitfield, size_t address) {
    // No bounds checking
    bitfield->data[address >> 3] &= (uint8_t) ~((0x1 << (address & 0x7)));
}
bool bitfieldGet(Bitfield* bitfield, size_t address) {
    return (bitfield->data[address >> 3] & (uint8_t) (0x1 << (address & 0x7))) != 0;
}