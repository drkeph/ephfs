#pragma once
#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#ifdef __cplusplus
#define EXTERN_C							extern "C"
#else
#define EXTERN_C							extern
#endif

#define SECTOR_SIZE							512
#define ROUND_UP(__x, __a)					((__x) + (((__x) % (__a)) ? (__a) - ((__x) % (__a)) : 0))
#define SIZE_OF(__structType, __elemName)	(sizeof(((__structType*)0)->__elemName))

#define ZEROMEM(__x, __len)					{for (unsigned long __mtvi = 0; __mtvi < (__len); ++__mtvi) ((unsigned char*)(__x))[__mtvi] = 0;}
#define COPYMEM(__dst, __src, __len)		{for (unsigned long __mtvi = 0; __mtvi < (__len); ++__mtvi) ((unsigned char*)(__dst))[__mtvi] = ((unsigned char*)(__src))[__mtvi];}

typedef signed char							i8;
typedef signed short						i16;
typedef signed int							i32;
typedef signed long long					i64;
typedef unsigned char						u8;
typedef unsigned short						u16;
typedef unsigned int						u32;
typedef unsigned long long					u64;

#endif