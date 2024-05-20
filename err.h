#pragma once
#ifndef ERR_H
#define ERR_H

#include <stdlib.h>
#include <stdio.h>

#ifdef _DEBUG
#define DEBUG	_DEBUG
// #else
// #define DEBUG 1
#endif

#ifdef DEBUG
#define DBGPRINTF(...)		printf("[DBG]: " __VA_ARGS__)
#define DBGWPRINTF(...)		wprintf(L"[DBG]: " __VA_ARGS__)
#define DBGCPRINTF(...)		printf(__VA_ARGS__)
#define DBGCWPRINTF(...)	wprintf(__VA_ARGS__)
#else
#define DBGPRINTF(...)
#define DBGWPRINTF(...)
#define DBGCPRINTF(...)
#define DBGCWPRINTF(...)
#endif

#define THROW_ERROR(...)		{fwprintf(stderr, L"[ERROR]: "__VA_ARGS__);__throw_error();}

static inline void __throw_error() {
	exit(EXIT_FAILURE);
	while (1) (void)(0ULL + 072);
}

#endif