#if defined(RNS_OS_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#include <Windows.h>
#endif

#ifdef RNS_OS_LINUX
#define RNS_POSIX
#include <unistd.h>
#include <linux/limits.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <time.h>
#endif


