#include "fileop.h"

void FastWriteFile(const wchar_t* path, const void* data, size_t size) {
	FILE* fout;
	_wfopen_s(&fout, path, L"wb");
	if (!fout) THROW_ERROR(L"failed to open `%s`!", path);
	if (fwrite(data, 1, size, fout) != size) THROW_ERROR(L"failed to write `%s`!", path);
	fclose(fout);
}

void FastAppendFile(const wchar_t* path, const void* data, size_t size) {
	FILE* fout;
	_wfopen_s(&fout, path, L"ab");
	if (!fout) THROW_ERROR(L"failed to open `%s`!", path);
	if (fwrite(data, 1, size, fout) != size) THROW_ERROR(L"failed to append `%s`!", path);
	fclose(fout);
}

void* FastReadFile(const wchar_t* path, size_t* ressize) {
	size_t size;
	void* data;
	FILE* fin;
	_wfopen_s(&fin, path, L"rb");
	if (!fin) THROW_ERROR(L"failed to open `%s`!", path);

	fseek(fin, 0, SEEK_END);
	size = (size_t)ftell(fin);
	fseek(fin, 0, SEEK_SET);

	if (ressize) *ressize = size;
	data = malloc(size);
	if (!data) {
		fclose(fin);
		THROW_ERROR(L"failed to allocate %lu bytes!", (unsigned long)size);
	}

	if (fread(data, 1, size, fin) != size) THROW_ERROR(L"failed to read `%s`!", path);
	fclose(fin);
	return data;
}

void FastCreateFileRecursivePath(const wchar_t* path) {
	size_t l = wcslen(path);
	size_t i = (size_t)(-1);
	wchar_t* next;
	FILE* file;

	wchar_t* tmp = (wchar_t*)calloc(l + 1, sizeof(wchar_t));
	if (!tmp) THROW_ERROR(L"failed to allocate %lu bytes!", (unsigned long)(l + 1));

	while (1) {
		next = wcschr(&path[i + 1], L'\\');
		if (!next) break;

		i = ((size_t)next - (size_t)path) / sizeof(wchar_t);
		memcpy(tmp, path, i * sizeof(wchar_t));
		tmp[i] = 0;

		CreateDirectoryW(tmp, NULL);
	}

	_wfopen_s(&file, path, L"wb");
	if (!file) THROW_ERROR(L"failed to create `%s`!", path);
	fclose(file);
	
	free(tmp);
}

void FastCreateDirectoryRecursivePath(const wchar_t* path) {
	size_t l = wcslen(path);
	size_t i = (size_t)(-1);
	wchar_t* next;

	wchar_t* tmp = (wchar_t*)calloc(l + 1, sizeof(wchar_t));
	if (!tmp) THROW_ERROR(L"failed to allocate %lu bytes!", (unsigned long)(l + 1));

	while (1) {
		next = wcschr(&path[i + 1], L'\\');
		if (!next) break;

		i = ((size_t)next - (size_t)path) / sizeof(wchar_t);
		memcpy(tmp, path, i * sizeof(wchar_t));
		tmp[i] = 0;

		CreateDirectoryW(tmp, NULL);
	}

	CreateDirectoryW(path, NULL);
	free(tmp);
}