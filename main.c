#include <stdio.h>
#include <Windows.h>

#include "murmur3.h"
#include "fileop.h"

#include "hdr.h"
#include "partition-hdr.h"
#include "file-hdr.h"
#include "data-block.h"

typedef struct _FS_OBJ {
	wchar_t	*Path;
	char	*FSPath;
	u8		*Data;
	u8		IsDirectory;
	size_t	Size;
} FS_OBJ;

size_t GetRecursiveNumberOfFilesInDirectory(const wchar_t* dirpath, size_t numprev) {
	WIN32_FIND_DATAW finddata;
	wchar_t* subdirpath;
	wchar_t* expdirpath;
	size_t filenamelen;
	size_t dirpathlen;
	wchar_t* filepath;
	HANDLE handle;

	dirpathlen = wcslen(dirpath);
	expdirpath = (wchar_t*)calloc(dirpathlen + 4, sizeof(wchar_t));
	if (!expdirpath) THROW_ERROR(L"failed to allocate memory!\n");

	memcpy(expdirpath, dirpath, sizeof(wchar_t) * dirpathlen);
	memcpy(&expdirpath[dirpathlen], L"\\*", sizeof(wchar_t) * 3);

	handle = FindFirstFileW(expdirpath, &finddata);
	if (handle == INVALID_HANDLE_VALUE) THROW_ERROR(L"failed to open directory `%s`\n", dirpath);

	do {
		if (!wcscmp(finddata.cFileName, L".") || !wcscmp(finddata.cFileName, L"..")) continue;

		filenamelen = wcslen(finddata.cFileName);
		filepath = (wchar_t*)calloc(dirpathlen + 1 + filenamelen + 1, sizeof(wchar_t));
		if (!filepath) THROW_ERROR(L"failed to allocate memory!\n");

		memcpy(filepath, dirpath, dirpathlen * sizeof(wchar_t));
		memcpy(&filepath[dirpathlen], L"\\", sizeof(wchar_t));
		memcpy(&filepath[dirpathlen + 1], finddata.cFileName, (filenamelen + 1) * sizeof(wchar_t));

		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// DBGWPRINTF(L"D: `%s`\n", filepath);

			subdirpath = (wchar_t*)calloc(dirpathlen + 1 + filenamelen + 1, sizeof(wchar_t));
			if (!subdirpath) THROW_ERROR(L"failed to allocate memory!\n");

			memcpy(subdirpath, dirpath, dirpathlen * sizeof(wchar_t));
			memcpy(&subdirpath[dirpathlen], L"\\", sizeof(wchar_t) * 1);
			memcpy(&subdirpath[dirpathlen + 1], finddata.cFileName, (filenamelen + 1) * sizeof(wchar_t));

			numprev = GetRecursiveNumberOfFilesInDirectory(subdirpath, numprev);
			free(subdirpath);
		}
		// else DBGWPRINTF(L"F: `%s`\n", filepath);

		numprev += 1;
		free(filepath);
	} while (FindNextFileW(handle, &finddata));

	FindClose(handle);
	free(expdirpath);
	return numprev;
}

size_t GetFSObjectsFromDirectory(FS_OBJ* dst, const wchar_t* dirpath, size_t numprev) {
	WIN32_FIND_DATAW finddata;
	wchar_t* subdirpath;
	wchar_t* expdirpath;
	size_t filenamelen;
	size_t dirpathlen;
	wchar_t* filepath;
	HANDLE handle;

	dirpathlen = wcslen(dirpath);
	expdirpath = (wchar_t*)calloc(dirpathlen + 4, sizeof(wchar_t));
	if (!expdirpath) THROW_ERROR(L"failed to allocate memory!\n");

	memcpy(expdirpath, dirpath, sizeof(wchar_t) * dirpathlen);
	memcpy(&expdirpath[dirpathlen], L"\\*", sizeof(wchar_t) * 3);

	handle = FindFirstFileW(expdirpath, &finddata);
	if (handle == INVALID_HANDLE_VALUE) THROW_ERROR(L"failed to open directory `%s`\n", dirpath);

	do {
		if (!wcscmp(finddata.cFileName, L".") || !wcscmp(finddata.cFileName, L"..")) continue;

		filenamelen = wcslen(finddata.cFileName);
		filepath = (wchar_t*)calloc(dirpathlen + 1 + filenamelen + 1, sizeof(wchar_t));
		if (!filepath) THROW_ERROR(L"failed to allocate memory!\n");

		memcpy(filepath, dirpath, dirpathlen * sizeof(wchar_t));
		memcpy(&filepath[dirpathlen], L"\\", sizeof(wchar_t));
		memcpy(&filepath[dirpathlen + 1], finddata.cFileName, (filenamelen + 1) * sizeof(wchar_t));

		dst[numprev].Path = (wchar_t*)calloc(wcslen(filepath) + 1, sizeof(wchar_t));
		if (!dst[numprev].Path) THROW_ERROR(L"failed to allocate memory!\n");

		memcpy(dst[numprev].Path, filepath, (wcslen(filepath) + 1) * sizeof(wchar_t));
		dst[numprev].IsDirectory = finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		dst[numprev].Size = dst[numprev].IsDirectory ? 0 : finddata.nFileSizeLow + ((sizeof(size_t) == 8 ? finddata.nFileSizeHigh : 0));

		numprev += 1;
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// DBGWPRINTF(L"D: `%s`\n", filepath);

			subdirpath = (wchar_t*)calloc(dirpathlen + 1 + filenamelen + 1, sizeof(wchar_t));
			if (!subdirpath) THROW_ERROR(L"failed to allocate memory!\n");

			memcpy(subdirpath, dirpath, dirpathlen * sizeof(wchar_t));
			memcpy(&subdirpath[dirpathlen], L"\\", sizeof(wchar_t) * 1);
			memcpy(&subdirpath[dirpathlen + 1], finddata.cFileName, (filenamelen + 1) * sizeof(wchar_t));

			numprev = GetFSObjectsFromDirectory(dst, subdirpath, numprev);
			free(subdirpath);
		}
		// else DBGWPRINTF(L"F: `%s`\n", filepath);

		free(filepath);
	} while (FindNextFileW(handle, &finddata));

	FindClose(handle);
	return numprev;
}

u8* CreateFileTableFromDirectory(const wchar_t* dirpath, size_t *resfiles, size_t *ressize) {
	EPHFS_DATA_BLOCK* datablock;
	EPHFS_FILE_HDR* filehdr;
	size_t extrapathlen;
	size_t curoffset;
	size_t totalsize;
	size_t numfiles;
	size_t pathlen;
	FS_OBJ* objs;
	char* chpos;
	size_t i;
	size_t j;
	u8* data;

	if (ressize) *ressize = 0;
	numfiles = GetRecursiveNumberOfFilesInDirectory(dirpath, 0);
	DBGWPRINTF(L"Number of files in `%s`: %lu\n", dirpath, (unsigned long)numfiles);

	objs = (FS_OBJ*)calloc(numfiles, sizeof(FS_OBJ));
	if (!objs) THROW_ERROR(L"failed to allocate memory!\n");

	GetFSObjectsFromDirectory(objs, dirpath, 0);

	totalsize = 0;
	for (i = 0; i < numfiles; ++i) {
		pathlen = wcslen(objs[i].Path);
		extrapathlen = wcslen(dirpath) + 1;

		if (pathlen >= SIZE_OF(EPHFS_FILE_HDR, Name)) THROW_ERROR(L"file (`%s`) path length (%lu) > %lu!\n", objs[i].Path, (unsigned long)pathlen, (unsigned long)SIZE_OF(EPHFS_FILE_HDR, Name));

		totalsize += sizeof(EPHFS_FILE_HDR);

		objs[i].FSPath = (char*)calloc(pathlen + 1 - extrapathlen, sizeof(char));
		if (!objs[i].FSPath) THROW_ERROR(L"failed to allocate memory!\n");

		wcstombs_s(&extrapathlen, objs[i].FSPath, pathlen + 1 - extrapathlen, &objs[i].Path[extrapathlen], pathlen - extrapathlen);
		for (j = 0; objs[i].FSPath[j]; ++j) if (objs[i].FSPath[j] == '\\') objs[i].FSPath[j] = '/';

		if (!objs[i].IsDirectory && objs[i].Size) {
			objs[i].Data = (u8*)FastReadFile(objs[i].Path, NULL);
			totalsize += SECTOR_SIZE * (ROUND_UP(objs[i].Size, SIZE_OF(EPHFS_DATA_BLOCK, Data)) / SIZE_OF(EPHFS_DATA_BLOCK, Data));
		}

		DBGPRINTF("`%s", objs[i].FSPath);
		DBGCWPRINTF(L"` - %s (%lu)\n", objs[i].IsDirectory ? L"DIR" : L"FILE", (unsigned long)objs[i].Size);
	}

	data = (u8*)malloc(totalsize);
	if (!data) THROW_ERROR(L"failed to allocate memory!\n");

	memset(data, 0, totalsize);

	if (resfiles) *resfiles = 0;
	filehdr = (EPHFS_FILE_HDR*)data;
	datablock = (EPHFS_DATA_BLOCK*)((size_t)data + numfiles * sizeof(EPHFS_FILE_HDR));
	for (i = 0; i < numfiles; ++i) {
		memset(filehdr, 0, sizeof(EPHFS_FILE_HDR));
		chpos = strrchr(objs[i].FSPath, '/');
		if (chpos) {
			memcpy(filehdr->Name, chpos + 1, strlen(chpos));
			filehdr->ParentDirectoryID = murmur3_32(objs[i].FSPath, (u32)((size_t)chpos - (size_t)objs[i].FSPath), EPHFS_FILE_ID_HASH_SEED);
		}
		else {
			memcpy(filehdr->Name, objs[i].FSPath, strlen(objs[i].FSPath) + 1);
			filehdr->ParentDirectoryID = EPHFS_ROOT_ID;
		}

		filehdr->ID = murmur3_32(objs[i].FSPath, (u32)strlen(objs[i].FSPath), EPHFS_FILE_ID_HASH_SEED);
		filehdr->Length = ROUND_UP(objs[i].Size, SIZE_OF(EPHFS_DATA_BLOCK, Data)) / SIZE_OF(EPHFS_DATA_BLOCK, Data);
		filehdr->OriginalSize = objs[i].Size;
		filehdr->Attributes = (objs[i].IsDirectory ? EPHFS_FILE_ATTRIBUTE_DIRECTORY : 0) | EPHFS_FILE_ATTRIBUTE_SYSTEM;
		filehdr->FirstDataBlockOffset = (u64)(((size_t)datablock - (size_t)filehdr) / SECTOR_SIZE);

		if (!objs[i].IsDirectory && filehdr->Length) {
			curoffset = 0;
			for (j = 0; j < filehdr->Length - 1; ++j) {
				EPHFSInitDataBlock(datablock, &objs[i].Data[curoffset], SIZE_OF(EPHFS_DATA_BLOCK, Data), 1);
				curoffset += SIZE_OF(EPHFS_DATA_BLOCK, Data);
				objs[i].Size -= SIZE_OF(EPHFS_DATA_BLOCK, Data);
				++datablock;
			}

			EPHFSInitDataBlock(datablock, &objs[i].Data[curoffset], objs[i].Size, 0);
			++datablock;
		}

		++filehdr;
		if (resfiles) *resfiles += 1;
	}

#pragma warning(push)
#pragma warning(disable: 6001)
	for (i = 0; i < numfiles; ++i) {
		if (objs[i].Path) free(objs[i].Path);
		if (objs[i].FSPath) free(objs[i].FSPath);
		if (objs[i].Data) free(objs[i].Data);
	}
#pragma warning(pop)
	if (objs) free(objs);
	if (ressize) *ressize = totalsize;
	return data;
}

wchar_t* ExtractPathFromFileTable(EPHFS_FILE_HDR* filetable, u64 numfiles, size_t fileid) {
	size_t numpathparts = 0;
	size_t fullpathlen = 0;
	wchar_t** pathparts;
	u32 prevparentid;
	size_t namelen;
	wchar_t* res;
	u64 i;
	u64 j;
	u64 k;

	for (i = 0; i < numfiles; ++i) {
		if (filetable[i].ID == fileid) {
			prevparentid = filetable[i].ParentDirectoryID;
			numpathparts = 1;
			j = 0;

			while (j < numfiles && prevparentid != EPHFS_ROOT_ID) {
				if (filetable[j].ID == prevparentid) {
					prevparentid = filetable[j].ParentDirectoryID;
					fullpathlen += strlen(filetable[j].Name) + 1;
					numpathparts += 1;
					j = 0;
				}
				else ++j;
			}

			break;
		}
	}

	pathparts = (wchar_t**)calloc(numpathparts, sizeof(wchar_t*));
	if (!pathparts) THROW_ERROR(L"failed to allocate %lu bytes!\n", (unsigned long)(numpathparts * sizeof(wchar_t*)));

	namelen = strlen(filetable[i].Name);
	pathparts[0] = (wchar_t*)calloc(namelen + 1, sizeof(wchar_t));
	if (!pathparts[0]) THROW_ERROR(L"failed to allocate %lu bytes!\n", (unsigned long)((namelen + 1) * sizeof(wchar_t)));

	mbstowcs_s(NULL, pathparts[0], namelen + 1, filetable[i].Name, namelen);
	fullpathlen += namelen;
	k = 1;
	
	prevparentid = filetable[i].ParentDirectoryID;
	j = 0;

	while (j < numfiles && prevparentid != EPHFS_ROOT_ID) {
		if (filetable[j].ID == prevparentid) {
			prevparentid = filetable[j].ParentDirectoryID;
			
			namelen = strlen(filetable[j].Name);
			if (k >= numpathparts) THROW_ERROR(L"unexpected internal error!\n");
			pathparts[k] = (wchar_t*)calloc(namelen + 1, sizeof(wchar_t));
			if (!pathparts[k]) THROW_ERROR(L"failed to allocate %lu bytes!\n", (unsigned long)((namelen + 1) * sizeof(wchar_t)));

			mbstowcs_s(NULL, pathparts[k], namelen + 1, filetable[j].Name, namelen);
			pathparts[k][namelen] = 0;
			k += 1;
			j = 0;
		}
		else ++j;
	}

	res = (wchar_t*)calloc(fullpathlen + 1, sizeof(wchar_t));
	if (!res) THROW_ERROR(L"failed to allocate %lu bytes!\n", (unsigned long)((fullpathlen + 1) * sizeof(wchar_t)));

	k = 0;
	i = (u64)(numpathparts - 1);
	do {
		j = wcslen(pathparts[i]);
		memcpy(&res[k], pathparts[i], (size_t)(j * sizeof(wchar_t)));
		res[(size_t)(k + j)] = L'\\';
		k += j + 1;
		--i;
	} while (i < numpathparts);

	res[fullpathlen] = 0;

#pragma warning(push)
#pragma warning(disable: 6001)
	if (pathparts) {
		for (i = 0; i < numpathparts; ++i) if (pathparts[i]) free(pathparts[i]);
		free(pathparts);
	}
#pragma warning(pop)

	return res;
}

int wmain(int argc, wchar_t** argv) {
	EPHFS_PARTITION_HDR *partitionhdr;
	EPHFS_DATA_BLOCK *datablock;
	EPHFS_FILE_HDR* filehdrs;
	wchar_t* wfullfilepath;
	wchar_t* wfilepath;
	size_t ptnnamelen;
	size_t totalsize;
	size_t filesize;
	size_t numfiles;
	EPHFS_HDR* phdr;
	size_t pathlen;
	EPHFS_HDR hdr;
	u8 *tdata;
	u8 *data;
	size_t j;
	size_t i;

	if (argc < 2) THROW_ERROR(L"invalid arguments (-help - get info)!\n");

	if (!wcscmp(argv[1], L"-help")) {
		puts("EPHERFS utility v1.0.0\nUsage:");
		puts("\tephfs <action> [...]");
		puts("\tActions:");
		puts("\t\t-help - get info");
		puts("\t\t-cft <dir> <out> - create file table [->number of sectors written]");
		puts("\t\t-cp-ft <name(31)> <num-of-sectors> <file-table-path> <num-of-files> <out> - create partition from file table [-> number of sectors written]");
		puts("\t\t-cp-d <name(31)> <num-of-sectors> <dir> <out> - create partition from file dir [-> number of sectors written]");
		puts("\t\t-c-bl <reserved-data-file> <out> <partitions...> - create fs image");
		puts("\t\t-e <fs-image> <out-dir> - unpack files & folders from fs image");
	}
	else if (!wcscmp(argv[1], L"-cft")) {
		if (argc != 4) THROW_ERROR(L"invalid arguments (-help - get info)!\n");

		data = CreateFileTableFromDirectory(argv[2], &numfiles, &totalsize);
		if (!data) return EXIT_FAILURE;

		FastWriteFile(argv[3], data, totalsize);
		free(data);

		printf("Done (%lu bytes written, %lu files processed)\n", (unsigned long)totalsize, (unsigned long)numfiles);
		return totalsize / SECTOR_SIZE;
	}
	else if (!wcscmp(argv[1], L"-cp-ft")) {
		if (argc != 7) THROW_ERROR(L"invalid arguments (-help - get info)!\n");

		data = (u8*)FastReadFile(argv[4], &filesize);
		totalsize = (size_t)(wcstoull(argv[3], NULL, 10) * SECTOR_SIZE);
		if (filesize > totalsize - 1) THROW_ERROR(L"file table size > partition size!\n");

		pathlen = wcslen(argv[2]);
		partitionhdr = (EPHFS_PARTITION_HDR*)data;
		if (pathlen > 31) THROW_ERROR(L"partition name (`%s`) length (%lu) > %lu!\n", argv[2], (unsigned long)pathlen, (unsigned long)SIZE_OF(EPHFS_PARTITION_HDR, Name));

		EPHFSInitPartitionHeader(
			partitionhdr,
			"???", (u64)wcstoull(argv[3], NULL, 10),
			(u64)(wcstoull(argv[3], NULL, 10) - filesize / SECTOR_SIZE),
			(u64)wcstoull(argv[5], NULL, 10), 1, 0
		);

		wcstombs_s(NULL, partitionhdr->Name, pathlen + 1, argv[2], pathlen);

		FastWriteFile(argv[6], data, totalsize);
		free(data);

		printf("Done (%lu bytes written)\n", (unsigned long)totalsize);
		return totalsize / SECTOR_SIZE;
	}
	else if (!wcscmp(argv[1], L"-cp-d")) {
		if (argc != 6) THROW_ERROR(L"invalid arguments (-help - get info)!\n");

		data = CreateFileTableFromDirectory(argv[4], &numfiles, &filesize);
		if (!data) return EXIT_FAILURE;

		totalsize = (size_t)(wcstoull(argv[3], NULL, 10) * SECTOR_SIZE);
		if (filesize + sizeof(EPHFS_PARTITION_HDR) > totalsize) THROW_ERROR(L"file table size (%lu) >= %lu!\n", (unsigned long)filesize, (unsigned long)totalsize);

		tdata = (u8*)malloc(totalsize);
		if (!tdata) THROW_ERROR(L"failed to allocate memory!\n");

		memset(tdata, 0, totalsize);
		pathlen = wcslen(argv[2]);
		if (pathlen > SIZE_OF(EPHFS_PARTITION_HDR, Name)) THROW_ERROR(L"partition name (`%s`) len (%lu) > %lu!\n", argv[2], (unsigned long)pathlen, (unsigned long)SIZE_OF(EPHFS_PARTITION_HDR, Name));

		partitionhdr = (EPHFS_PARTITION_HDR*)tdata;
		wcstombs_s(NULL, partitionhdr->Name, pathlen + 1, argv[2], pathlen);
		partitionhdr->NumberOfSectors = totalsize / SECTOR_SIZE;
		partitionhdr->NumberOfFreeSectors = partitionhdr->NumberOfSectors - filesize / SECTOR_SIZE;
		partitionhdr->NumberOfFiles = numfiles;
		partitionhdr->FileTableOffset = 1;

		EPHFSInitPartitionHeader(
			partitionhdr, "???", totalsize / SECTOR_SIZE,
			(u64)((totalsize - filesize) / SECTOR_SIZE), numfiles, 1, 0
		);

		wcstombs_s(NULL, partitionhdr->Name, pathlen + 1, argv[2], pathlen);
		memcpy(&tdata[sizeof(EPHFS_PARTITION_HDR)], data, filesize);

		FastWriteFile(argv[5], tdata, totalsize);
		free(tdata);
		free(data);

		printf("Done (%lu bytes written, %lu files processed)\n", (unsigned long)totalsize, (unsigned long)numfiles);
		return totalsize / SECTOR_SIZE;
	}
	else if (!wcscmp(argv[1], L"-c-bl")) {
		if (argc < 5) THROW_ERROR(L"invalid arguments (-help - get info)!\n");

		data = (u8*)FastReadFile(argv[2], &filesize);

		if (filesize > 0xFFFFF) THROW_ERROR(L"reserved data file is too big (%lu bytes > %lu)!\n", (unsigned long)filesize, (unsigned long)0xFFFFF);
		if (filesize % SECTOR_SIZE)
			THROW_ERROR(L"invalid reserved data file size (%lu %% %lu = %lu)!\n", (unsigned long)filesize, (unsigned long)SECTOR_SIZE, (unsigned long)(filesize % SECTOR_SIZE));

		EPHFSInitHeader(&hdr, EPHFS_HDR_VERSION1, (u16)(filesize / SECTOR_SIZE), (u64)(argc - 4), (u64)(1 + filesize / SECTOR_SIZE));

		FastWriteFile(argv[3], &hdr, sizeof(EPHFS_HDR));
		FastAppendFile(argv[3], data, filesize);
		free(data);

		totalsize = sizeof(EPHFS_HDR) + filesize;

		for (i = 4; i < (size_t)argc; ++i) {
			data = (u8*)FastReadFile(argv[i], &filesize);
			if (filesize % SECTOR_SIZE)
				THROW_ERROR(L"invalid file (`%s`) size (%lu %% %lu = %lu)!\n", argv[i], (unsigned long)filesize, (unsigned long)SECTOR_SIZE, (unsigned long)(filesize % SECTOR_SIZE));

			((EPHFS_PARTITION_HDR*)data)->NextPartitionOffset = (u64)((i < (size_t)(argc) - 1) ? filesize / SECTOR_SIZE : 0);

			FastAppendFile(argv[3], data, filesize);
			free(data);

			totalsize += filesize;
		}

		printf("Done (%lu bytes written)", (unsigned long)totalsize);
	}
	else if (!wcscmp(argv[1], L"-e")) {
		if (argc != 4) THROW_ERROR(L"invalid arguments (-help - get info)!\n");

		data = (u8*)FastReadFile(argv[2], &filesize);
		phdr = (EPHFS_HDR*)data;
		totalsize = 0;

		FastCreateDirectoryRecursivePath(argv[3]);
		
		pathlen = wcslen(argv[3]);
		wfilepath = (wchar_t*)calloc(pathlen + 1 + strlen("reserved-sectors.bin") + 1, sizeof(wchar_t));
		if (!wfilepath) THROW_ERROR(L"failed to allocate memory!\n");
		memcpy(wfilepath, argv[3], pathlen * sizeof(wchar_t));
		wfilepath[pathlen] = L'\\';
		memcpy(&wfilepath[pathlen + 1], L"reserved-sectors.bin", (strlen("reserved-sectors.bin") + 1) * sizeof(wchar_t));
		FastWriteFile(wfilepath, &data[sizeof(EPHFS_HDR)], phdr->NumberOfReservedSectors * SECTOR_SIZE);
		totalsize += phdr->NumberOfReservedSectors * SECTOR_SIZE;
		free(wfilepath);

		partitionhdr = (EPHFS_PARTITION_HDR*)((size_t)data + phdr->FirstPartitonOffset * SECTOR_SIZE);

		do {
			ptnnamelen = strlen(partitionhdr->Name);
			filehdrs = (EPHFS_FILE_HDR*)((size_t)partitionhdr + partitionhdr->FileTableOffset * SECTOR_SIZE);
			for (i = 0; i < partitionhdr->NumberOfFiles; ++i) {
				filesize = (size_t)filehdrs[i].OriginalSize;
				datablock = (EPHFS_DATA_BLOCK*)((size_t)&filehdrs[i] + filehdrs[i].FirstDataBlockOffset * SECTOR_SIZE);

				pathlen = wcslen(argv[3]);
				wfilepath = ExtractPathFromFileTable(filehdrs, partitionhdr->NumberOfFiles, filehdrs[i].ID);
				wfullfilepath = (wchar_t*)calloc(pathlen + 1 + ptnnamelen + 1 + wcslen(wfilepath) + 1, sizeof(wchar_t));
				if (!wfullfilepath) THROW_ERROR(L"failed to allocate memory!\n");

				memcpy(wfullfilepath, argv[3], pathlen * sizeof(wchar_t));
				wfullfilepath[pathlen] = L'\\';
				pathlen += 1;

				mbstowcs_s(NULL, &wfullfilepath[pathlen], ptnnamelen + 1, partitionhdr->Name, ptnnamelen);
				pathlen += ptnnamelen;
				wfullfilepath[pathlen] = L'\\';
				pathlen += 1;

				memcpy(&wfullfilepath[pathlen], wfilepath, (wcslen(wfilepath) + 1) * sizeof(wchar_t));
				free(wfilepath);

				if (filehdrs[i].Attributes & EPHFS_FILE_ATTRIBUTE_DIRECTORY) FastCreateDirectoryRecursivePath(wfullfilepath);
				else {
					FastCreateFileRecursivePath(wfullfilepath);

					if (filehdrs[i].Length) {
						for (j = 0; j < filehdrs[i].Length - 1; ++j) {
							FastAppendFile(wfullfilepath, datablock->Data, SIZE_OF(EPHFS_DATA_BLOCK, Data));
							filesize -= SIZE_OF(EPHFS_DATA_BLOCK, Data);
							totalsize += SIZE_OF(EPHFS_DATA_BLOCK, Data);

							if (!datablock->NextDataBlockOffset) break;
							datablock = (EPHFS_DATA_BLOCK*)((size_t)datablock + datablock->NextDataBlockOffset * SECTOR_SIZE);
						}
					}

					FastAppendFile(wfullfilepath, datablock->Data, filesize);
					totalsize += filesize;
				}

				free(wfullfilepath);
			}

			partitionhdr = (EPHFS_PARTITION_HDR*)((size_t)partitionhdr + partitionhdr->NextPartitionOffset * SECTOR_SIZE);
		} while (partitionhdr->NextPartitionOffset);

		free(data);
		printf("Done (%lu bytes written)", (unsigned long)totalsize);
	}
	else THROW_ERROR(L"unknown action (-help - get info)!\n");
	return ERROR_SUCCESS;
}