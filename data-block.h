#pragma once
#ifndef DATA_BLOCK_H
#define DATA_BLOCK_H

#include "typedefs.h"

#pragma pack(push, 1)
typedef struct _EPHFS_DATA_BLOCK {
	u8	Data[504];
	u64	NextDataBlockOffset;		// offset to the next block of file data in sectors (if last, then is 0)
} EPHFS_DATA_BLOCK;
#pragma pack(pop)

static inline void EPHFSInitDataBlock(EPHFS_DATA_BLOCK* dst, u8* data, size_t size, u64 nextdatablockoffset) {
	if (size > SIZE_OF(EPHFS_DATA_BLOCK, Data)) THROW_ERROR(L"data block is too big (%lu/%lu bytes)!", (unsigned long)size, (unsigned long)SIZE_OF(EPHFS_DATA_BLOCK, Data));

	ZEROMEM(dst, sizeof(EPHFS_DATA_BLOCK));
	COPYMEM(dst->Data, data, size);
	dst->NextDataBlockOffset = nextdatablockoffset;
}

#endif