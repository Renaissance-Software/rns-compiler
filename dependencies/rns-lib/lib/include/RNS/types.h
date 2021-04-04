#pragma once

/*
* This file contains basic types and macro definitions to help in general programming
*/
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef NDEBUG
#define RNS_RELEASE
#else
#define RNS_DEBUG
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uptr;
typedef size_t usize;

typedef float f32;
typedef double f64;

#ifndef __cplusplus
#include <stdbool.h>
#define null NULL
#define nullptr NULL
#endif

#define rns_array_length(_arr) ((sizeof(_arr)) / sizeof(_arr[0]))
#define rns_case_to_str(x) case (x): return #x
#define rns_unused_elem(x) (void)x
#define rns_single_digit_decimal_char_to_int(_c_) (((s32)_c_) - 48)

#define RNS_A_BYTE      (1ULL)
#define RNS_BYTE(x)     (x * RNS_A_BYTE)
#define RNS_KILOBYTE(x) (x * (RNS_BYTE(1024)))
#define RNS_MEGABYTE(x) (x * (RNS_KILOBYTE(1024)))
#define RNS_GIGABYTE(x) (x * (RNS_MEGABYTE(1024)))
#define CONCAT_HELPER(_a_, _b_) a ## b
#define CONCAT(_a_, _b_) CONCAT_HELPER(a, b)

#ifdef __cplusplus

template <typename T>
struct __defer_helper__
{
    T lambda;
    __defer_helper__(T lambda) : lambda(lambda) {}
    ~__defer_helper__() { lambda(); }
    __defer_helper__(const __defer_helper__&);
private:
    __defer_helper__& operator=(const __defer_helper__&);
};

struct __defer_helper2__
{
    template<typename T>
    __defer_helper__<T> operator+(T t) { return t; }
};

#define defer const auto& CONCAT(defer__, __LINE__) = __defer_helper2__ + [&]()
#endif
