#pragma once
#ifndef PARTITION_HDR_H
#define PARTITION_HDR_H

#include "typedefs.h"
#include "err.h"

#pragma pack(push, 1)
typedef struct _EPHFS_PARTITION_HDR {
	/*0x00*/	char	Name[32];
	/*0x20*/	u64		NumberOfSectors;		// maximum number of sectors in a partition
	/*0x28*/	u64		NumberOfFreeSectors;
	/*0x30*/	u64		NumberOfFiles;
	/*0x38*/	u64		FileTableOffset;		// offset to the first file header in sectors
	/*0x40*/	u64		NextPartitionOffset;	// offset to the next partition in sectors
	/*0x48*/	u64		Reserved[55];
} EPHFS_PARTITION_HDR;
#pragma pack(pop)

static inline void EPHFSInitPartitionHeader(
	EPHFS_PARTITION_HDR* dst,
	const char* name,
	u64 numsectors,
	u64 numfreesectors,
	u64 numfiles,
	u64 filetableoffset,
	u64 nextptnoffset
) {
	size_t namelen = strlen(name);

	if (namelen >= SIZE_OF(EPHFS_PARTITION_HDR, Name)) THROW_ERROR(L"ptn name is too long (%lu/%lu bytes)", (unsigned long)namelen, (unsigned long)SIZE_OF(EPHFS_PARTITION_HDR, Name));

	ZEROMEM(dst, sizeof(EPHFS_PARTITION_HDR));
	COPYMEM(dst->Name, name, namelen);
	dst->NumberOfSectors = numsectors;
	dst->NumberOfFreeSectors = numfreesectors;
	dst->NumberOfFiles = numfiles;
	dst->FileTableOffset = filetableoffset;
	dst->NextPartitionOffset = nextptnoffset;
}

#endif