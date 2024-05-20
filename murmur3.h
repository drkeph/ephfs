#pragma once
#ifndef MURMUR3_H
#define MURMUR3_H

#include <string.h>
#include "typedefs.h"

static inline u32 murmur_32_scramble(u32 k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

u32 murmur3_32(const u8 *s, u32 l, u32 key);
#endif