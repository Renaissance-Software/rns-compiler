#pragma once

#if _MSC_VER
#define RNS_COMPILER_MSVC
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#else
#define RNS_COMPILER_GNU_COMPATIBLE
#endif


#ifdef __cplusplus
template<typename T>
inline T max(T a, T b)
{
    if (a >= b)
    {
        return a;
    }
    return b;
}
template<typename T>
inline T min(T a, T b)
{
    if (a <= b)
    {
        return a;
    }
    return b;
}
#else
#ifdef RNS_COMPILER_MSVC
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#else // GNU compatible
#ifndef MAX
#define MAX(a, b)\
    ({__typeof__ (a) _a = (a);\
      __typeof__ (b) _b = (b);\
      _a > _b ? _a : _b; })
#endif
#ifndef MIN
#define MIN(a, b)\
    ({__typeof__ (a) _a = (a);\
      __typeof__ (b) _b = (b);\
      _a < _b ? _a : _b; })
#endif
#endif
#endif

// List of compiler attributes
#ifdef RNS_COMPILER_MSVC
#define RNS_ALIGN(_size_) __declspec(align((_size_)))
#define RNS_FORCE_INLINE __forceinline
#define RNS_NOINLINE __declspec(noinline)
#define RNS_NORETURN __declspec(noreturn)
#define RNS_RESTRICT_FN __declspec(restrict)
#define RNS_RESTRICT_VAR __restrict
#define RNS_SHARED_LIBRARY_IMPORT __declspec(dllimport)
#define RNS_SHARED_LIBRARY_EXPORT __declspec(dllexport)
#define RNS_NAKED __declspec(naked)
#define RNS_CDECL __cdecl
#define RNS_FASTCALL __fastcall
#define RNS_STDCALL __stdcall
#else
#error
#endif
