#pragma once
#ifndef FILE_OP_H
#define FILE_OP_H

#include <Windows.h>
#include "err.h"

void FastWriteFile(const wchar_t* path, const void* data, size_t size);
void FastAppendFile(const wchar_t* path, const void* data, size_t size);
void* FastReadFile(const wchar_t* path, size_t* ressize);
void FastCreateFileRecursivePath(const wchar_t* path);
void FastCreateDirectoryRecursivePath(const wchar_t* path);

#endif