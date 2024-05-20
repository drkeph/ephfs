#	`EPHERFS` ver. `KTSN` (20 may 2024)
##	This document describes the capabilities, structures and ways of implementing the functions of the `EPHERFS`
##	Capabilities
+	####	Subdirectories
+	####	Quick access to files and directories
+	####	Easy to implement
##	Limitations
+	####	Maximum file name length: 255
+	####	Does not support `.` and `..`
+	####	No shortcuts
+	####	A small set of available file attributes
+	####	Partially supports 64-bit (64-bit support is not fully implemented in the 'ephfs' utility)
+	####	No protection against bad sectors
##	Structures
###	`EPHFS_HDR` (`EPHERFS` header)
####	Describes the file system
```c
#define EPHFS_HDR_SIGNATURE	"*EPHRFS*"
#define EPHFS_HDR_VERSION1	"KTSN"

#pragma pack(push, 1)
typedef struct _EPHFS_HDR {
	/*0x00*/	char	Signature[8];			// "*EPHRFS*" (EPHFS_HDR_SIGNATURE)
	/*0x08*/	char	Reserved1[4];			// "VER:"
	/*0x0C*/	char	Version[4];			// "KTSN"
	/*0x10*/	u16	NumberOfReservedSectors;	// (located on the second sector)
	/*0x12*/	u16	Reserved2[3];
	/*0x18*/	u64	NumberOfPartitions;
	/*0x20*/	u64	FirstPartitonOffset;		// offset to the first partition in sectors
	/*0x28*/	u64	Reserved3[59];
} EPHFS_HDR;
#pragma pack(pop)
```
### `EPHFS_PARTITION_HDR` (`EPHERFS` partition header)
####	Describes the partition
```c
#pragma pack(push, 1)
typedef struct _EPHFS_PARTITION_HDR {
	/*0x00*/	char	Name[32];
	/*0x20*/	u64	NumberOfSectors;		// maximum number of sectors in a partition
	/*0x28*/	u64	NumberOfFreeSectors;
	/*0x30*/	u64	NumberOfFiles;
	/*0x38*/	u64	FileTableOffset;		// offset to the first file header in sectors
	/*0x40*/	u64	NextPartitionOffset;		// offset to the next partition in sectors
	/*0x48*/	u64	Reserved[55];
} EPHFS_PARTITION_HDR;
#pragma pack(pop)
```
###	`EPHFS_FILE_HDR` (`EPHERFS` file header)
####	Describes the file
```c
#define EPHFS_FILE_ID_HASH_SEED		0x4E53544B
#define EPHFS_ROOT_ID			0			// root directory hash

#define EPHFS_FILE_ATTRIBUTE_DIRECTORY	0x01
#define EPHFS_FILE_ATTRIBUTE_SYSTEM	0x02
#define EPHFS_FILE_ATTRIBUTE_HIDDEN	0x04
#define EPHFS_FILE_ATTRIBUTE_READONLY	0x08

/*
	https://en.wikipedia.org/wiki/MurmurHash
	ID - Murmur v3 hash of the file path without partition name
	ParentDirectoryID - Murmur v3 hash of the path of the parent directory of the file without the partition namee
*/
#pragma pack(push, 1)
typedef struct _EPHFS_FILE_HDR {
	/*0x000*/	char	Name[256];
	/*0x100*/	u32	ID;				// Murmur v3 x86 (https://en.wikipedia.org/wiki/MurmurHash)
	/*0x104*/	u32	ParentDirectoryID;		// Murmur v3 x86
	/*0x108*/	u64	Length;				// number of data sectors
	/*0x110*/	u64	OriginalSize;
	/*0x118*/	u64	Attributes;
	/*0x120*/	u64	FirstDataBlockOffset;		// offset to the first block of file data in sectors
	/*0x128*/	u64	Reserved[27];
} EPHFS_FILE_HDR;
#pragma pack(pop)
```
###	`EPHFS_DATA_BLOCK` (`EPHERFS` data block)
####	Contains part of the file data (504 bytes)
```c
#pragma pack(push, 1)
typedef struct _EPHFS_DATA_BLOCK {
	/*0x000*/	u8	Data[504];
	/*0x1F8*/	u64	NextDataBlockOffset;		// offset to the next block of file data in sectors (if last, then is 0)
} EPHFS_DATA_BLOCK;
#pragma pack(pop)
```
##	Implementation
*	###	How to find a partition:
	+	###	Go to the `EPHFS_HDR` structure
	+	###	Go `EPHFS_HDR::FirstPartitionOffset` sectors forward
	+	###	For each `EPHFS_PARTITION_HDR`:
		+	###	Compare the name of the partition you are looking for with `EPHFS_PARTITION_HDR::Name`
		+	###	If the names match, then you have found the partition. Otherwise, go `EPHFS_PARTITION_HDR::NextPartitionOffset` sectors forward (*if this field is not 0!*)
*	###	How to find the file (file path format: `/PARTITION_NAME/PATH/TO/FILE.EXT`):
	+	###	Find the partition that contains the file
	+	###	Remove `/PARTITION_NAME/` from the file path
	+	###	Calculate the Murmur3 hash of the resulting file path 
	+	###	Go to file table (go `EPHFS_PARTITION_HDR::FileTableOffset` sectors forward)
	+	###	For each `EPHFS_FILE_HDR`:
		+	###	If `EPHFS_FILE_HDR::ID` matches the hash calculated by Murmur3, then you have found the file.
		+	###	Otherwise, go 1 sector forward (*if it was not the last file!*)
*	###	How to read a file:
	+	###	Find the corresponding file `EPHFS_FILE_HDR`
	+	###	Go `EPHFS_FILE_HDR::FirstDataBlockOffset` sectors forward
	+	###	For each `EPHFS_DATA_BLOCK`:
		+	###	Copy `EPHFS_DATA_BLOCK::Data` to buffer
		+	###	If `EPHFS_DATA_BLOCK::NextDataBlockOffset` != 0, then move `EPHFS_DATA_BLOCK::NextDataBlockOffset` sectors forward. Otherwise you have reached the end of the file.
*	###	How to create a new file and write data to it: (`N/A`)
*	###	How to overwrite existing file: (`N/A`)
*	###	How to delete file: (`N/A`)