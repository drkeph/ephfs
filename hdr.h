#pragma once
#ifndef HDR_H
#define HDR_H

#include "typedefs.h"

#define EPHFS_HDR_SIGNATURE		"*EPHRFS*"
#define EPHFS_HDR_VERSION1		"KTSN"

#pragma pack(push, 1)
typedef struct _EPHFS_HDR {
	/*0x00*/	char	Signature[8];				// "*EPHRFS*" (EPHFS_HDR_SIGNATURE)
	/*0x08*/	char	Reserved1[4];				// "VER:"
	/*0x0C*/	char	Version[4];					// "KTSN"
	/*0x10*/	u16		NumberOfReservedSectors;	// (located on the second sector)
	/*0x12*/	u16		Reserved2[3];
	/*0x18*/	u64		NumberOfPartitions;
	/*0x20*/	u64		FirstPartitonOffset;		// offset to the first partition in sectors
	/*0x28*/	u64		Reserved3[59];
} EPHFS_HDR;
#pragma pack(pop)

static inline void EPHFSInitHeader(
	EPHFS_HDR *dst,
	const char *version,
	u16 numofrsvdsectors,
	u64 numofpartitions,
	u64 firstptnoffset
) {
	if (strlen(version) != SIZE_OF(EPHFS_HDR, Version)) THROW_ERROR(L"invalid version name (must be %lu bytes long)!", (unsigned long)SIZE_OF(EPHFS_HDR, Version));

	ZEROMEM(dst, sizeof(EPHFS_HDR));
	COPYMEM(dst->Signature, EPHFS_HDR_SIGNATURE, SIZE_OF(EPHFS_HDR, Signature));
	COPYMEM(dst->Reserved1, "VER:", SIZE_OF(EPHFS_HDR, Reserved1));
	COPYMEM(dst->Version, version, SIZE_OF(EPHFS_HDR, Version));
	dst->NumberOfReservedSectors = numofrsvdsectors;
	dst->NumberOfPartitions = numofpartitions;
	dst->FirstPartitonOffset = firstptnoffset;
}

#endif