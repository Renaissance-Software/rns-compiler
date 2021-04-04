#pragma once

#include <RNS/types.h>
#include <RNS/compiler.h>
#include <RNS/data_structures.h>

#if _WIN64
#define RNS_OS_WINDOWS
#elif __linux__
#define RNS_OS_LINUX
#elif __APPLE__
#define RNS_OS_APPLE
#endif

#ifdef RNS_OS_WINDOWS
#define RNS_DEBUG_BREAK() __debugbreak()
#else
#error
#endif

#ifdef RNS_DEBUG
#define rns_assert(_expr_) if (!(_expr_)) { RNS::panic(__FILE__, __LINE__, __func__, "Assert failed: expression " #_expr_ " is false\n"); RNS_DEBUG_BREAK(); }
#else
#define rns_assert(_expr_)
#endif
#define Assert(x) rns_assert(x)


#ifdef RNS_DEBUG
#define RNS_os_check(_result_) if (_result_) { rns_panic(__FILE__, __LINE__, __func__, "OS operation failed: error " #_result_ "\n"); }
#else
#define OSCheck(_result_) _expr_
#endif


#define RNS_NOT_IMPLEMENTED { assert(!"Not implemented"); }
#define RNS_UNREACHABLE { assert(!"Unreachable"); }
#define RNS_PANIC { }

namespace RNS
{
#ifdef RNS_OS_WINDOWS
    typedef void* PID;
    typedef s64 PerformanceCounter;
#else
    typedef pid_t PID;
    typedef u64 PerformanceCounter;
#endif
    typedef void* (AllocationCallback)(void* allocator, s64 size);
    typedef struct VirtualAllocOptions
    {
        u8 large_pages : 1;
        u8 commit : 1;
        u8 reserve : 1;
        u8 execute : 1;
        u8 read : 1;
        u8 write : 1;
        u8 no_access : 1;
    } VirtualAllocOptions;
    enum class OSResult 
    {
        Success = 0,
        Error_Buffer_not_long_enough,
    };
    void                        print(const char* format, ...);
    void                        println(const char* format, ...);
    void                        panic(const char* filename, s32 line, const char* function_name, const char* error_message);
    void                        os_init(void);
    OSResult                    get_cwd(char* cwd_buffer, s32 cwd_buffer_size);
    /*
    * Virtual allocation API is still sloppy since it's experimental
    */
    void*                       virtual_alloc(void* address, s64 size, VirtualAllocOptions options);
    void                        virtual_free(void* address);
    void*                       heap_alloc(s64 size);
    usize                       get_page_size(void);
    void                        exit(s32 code);
    void                        exit_with_message(s32 code, const char* message, ...);
    PerformanceCounter          get_performance_counter(void);
    CSB                         file_load(const char* filename, Allocator* allocator);
    const char*                 error_string(OSResult error);
    Allocation                  default_allocate(Allocator* allocator, s64 size);
    /* @TODO: @ToBeImplemented*/
    //void        os_spawn_process(const char* exe_path);
    //s32        os_load_shared_library(...);

}
