#pragma once
#ifndef FILE_HDR_H
#define FILE_HDR_H

#include "typedefs.h"

#define EPHFS_FILE_ID_HASH_SEED			0x4E53544B
#define EPHFS_ROOT_ID					0

#define EPHFS_FILE_ATTRIBUTE_DIRECTORY	0x01
#define EPHFS_FILE_ATTRIBUTE_SYSTEM		0x02
#define EPHFS_FILE_ATTRIBUTE_HIDDEN		0x04

#pragma pack(push, 1)
typedef struct _EPHFS_FILE_HDR {
	char	Name[256];
	u32		ID;							// Murmur v3 x86 (https://en.wikipedia.org/wiki/MurmurHash)
	u32		ParentDirectoryID;			// Murmur v3 x86
	u64		Length;						// number of data sectors
	u64		OriginalSize;
	u64		Attributes;
	u64		FirstDataBlockOffset;		// offset to the first block of file data in sectors
	u64		Reserved[27];
} EPHFS_FILE_HDR;
#pragma pack(pop)

static inline void EPHFSInitFileHeader(
	EPHFS_FILE_HDR* dst,
	const char* name,
	u32 id,
	u32 parentdirid,
	u64 length,
	u64 originalsize,
	u64 attributes,
	u64 firstdatablockoffset
) {
	size_t namelen = strlen(name);

	if (namelen >= SIZE_OF(EPHFS_FILE_HDR, Name)) THROW_ERROR(L"ptn name is too long (%lu/%lu bytes)", (unsigned long)namelen, (unsigned long)SIZE_OF(EPHFS_FILE_HDR, Name));

	ZEROMEM(dst, sizeof(EPHFS_FILE_HDR));
	COPYMEM(dst->Name, name, namelen);
	dst->ID = id;
	dst->ParentDirectoryID = parentdirid;
	dst->Length = length;
	dst->OriginalSize = originalsize;
	dst->Attributes = attributes;
	dst->FirstDataBlockOffset = firstdatablockoffset;
}

#endif