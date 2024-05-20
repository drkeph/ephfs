#include "murmur3.h"

u32 murmur3_32(const u8 *s, u32 l, u32 key) {
	u32 h = key;
	u32 k;
	u32 i;

	for (i = l >> 2; i; i--) {
		memcpy(&k, s, sizeof(u32));
		s += sizeof(u32);
		h ^= murmur_32_scramble(k);
		h = (h << 13) | (h >> 19);
		h = h * 5 + 0xe6546b64;
	}

	k = 0;
	for (i = l & 3; i; i--) {
		k <<= 8;
		k |= s[i - 1];
	}

	h ^= murmur_32_scramble(k);

	h ^= l;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}
