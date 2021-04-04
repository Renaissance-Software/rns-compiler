#include <RNS/os.h>
#include <RNS/os_internal.h>
#include <RNS/arch.h>
#include <RNS/c_containers.h>

#ifndef RNS_OS_PANIC_ON_ERROR
#define RNS_OS_PANIC_ON_ERROR (0)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

bool system_initialized = false;

static usize page_size;
static u16 logical_thread_count;
static u16 shared_library_count;

/* Static cached structures */
#ifdef RNS_OS_WINDOWS
static SYSTEM_INFO system_info;
static LARGE_INTEGER performance_frequency;
static HINSTANCE loaded_dlls[1000];
static HANDLE process_heap;
#endif

void RNS::os_init(void)
{
#ifdef RNS_OS_WINDOWS

    GetSystemInfo(&system_info);
    page_size = system_info.dwPageSize > system_info.dwAllocationGranularity ? system_info.dwPageSize : system_info.dwAllocationGranularity;
    logical_thread_count = (u16)system_info.dwNumberOfProcessors;
    QueryPerformanceFrequency(&performance_frequency);
    process_heap = GetProcessHeap();

#elif defined(RNS_OS_LINUX)
#error
#else
#error
#endif
}

void RNS::panic(const char* filename, s32 line, const char* function_name, const char* error_message)
{
    printf("[Panic] [%s:%d](%s) %s", filename, line, function_name, error_message);
}

RNS::OSResult RNS::get_cwd(char* cwd_buffer, s32 cwd_buffer_size)
{
#ifdef RNS_OS_WINDOWS

    rns_assert(cwd_buffer_size >= MAX_PATH);
    DWORD result = GetCurrentDirectoryA(cwd_buffer_size, cwd_buffer);
    if (result == 0)
    {
        return RNS::OSResult::Error_Buffer_not_long_enough;
    }
    return RNS::OSResult::Success;

#elif defined(RNS_OS_POSIX)

    Assert(cwd_buffer_size >= PATH_MAX);
    char* result = getcwd(cwd_buffer, cwd_buffer_size);
    if (!result)
    {
        return OSResult_Error_Buffer_not_long_enough;
    }
    return OSResult_Success;

#else
#error
#endif
}

typedef struct HeapAllocOptions
{
    u8 zero_memory : 1;
    u8 no_serialize : 1;
} HeapAllocOptions;

void* os_heap_alloc(s64 size, HeapAllocOptions options)
{
    void* address = NULL;
    if (size > 0)
    {
#ifdef RNS_OS_WINDOWS
        DWORD option_flags = 0;
        if (options.no_serialize)
        {
            option_flags |= HEAP_NO_SERIALIZE;
        }
        if (options.zero_memory)
        {
            option_flags |= HEAP_ZERO_MEMORY;
        }
        address = HeapAlloc(process_heap, option_flags, (usize)size);
#else
#error
#endif
    }
    return address;
}

void* RNS::virtual_alloc(void* target_address, s64 size, VirtualAllocOptions options)
{
    void* address = NULL;
    if (size > 0)
    {
#ifdef RNS_OS_WINDOWS
        DWORD allocation_type = 0;
        DWORD protect_flags = 0;
        if (target_address)
        {
            Assert(options.commit);
        }
        if (options.reserve)
        {
            allocation_type |= MEM_RESERVE;
        }
        if (options.commit)
        {
            allocation_type |= MEM_COMMIT;
        }
        if (options.write && options.read && options.execute)
        {
            protect_flags |= PAGE_EXECUTE_READWRITE;
        }
        else if (options.write && options.read && !options.execute)
        {
            protect_flags |= PAGE_READWRITE;
        }
        else if (options.execute && options.read && !options.write)
        {
            protect_flags |= PAGE_EXECUTE_READ;
        }
        else if (options.execute && !options.read && !options.write)
        {
            protect_flags |= PAGE_EXECUTE;
        }
        else if (options.read && !options.write && !options.execute)
        {
            protect_flags |= PAGE_READONLY;
        }
        else
        {
            RNS_NOT_IMPLEMENTED;
        }
        address = VirtualAlloc(target_address, (usize)size, allocation_type, protect_flags);
#elif defined(RNS_OS_POSIX)
#error
#else
#error
#endif
    }
    return address;
}

void RNS::virtual_free(void* address)
{
#if defined(RNS_OS_WINDOWS)
    auto result = VirtualFree(address, 0, MEM_RELEASE);
    assert(result != 0);
#else
#error
#endif
}

usize RNS::get_page_size(void)
{
    return page_size;
}

const char* os_error_string(RNS::OSResult error)
{
    switch (error)
    {
        rns_case_to_str(RNS::OSResult::Error_Buffer_not_long_enough);
        default:
            RNS_NOT_IMPLEMENTED;
            return NULL;
    }
}

void RNS::exit(s32 code)
{
#ifdef RNS_OS_WINDOWS
    ExitProcess((u32)code);
#else
    exit(code);
#endif

}

void RNS::exit_with_message(s32 code, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    vfprintf(stdout, message, args);
    va_end(args);
    RNS::exit(code);
}

RNS::CSB RNS::file_load(const char* filename, Allocator* allocator)
{
    FILE* file_handle = fopen(filename, "rb");
    if (!file_handle)
    {
        return {};
    }

    s32 rc = fseek(file_handle, 0, SEEK_END);
    if (rc)
    {
        return {};
    }

    usize length = ftell(file_handle);
    if (length == 0)
    {
        return {};
    }

    auto allocation = allocator->allocate(allocator, (s64)length);
    if (allocation.size < (s64)length)
    {
        return {};
    }
    if (!allocation.ptr)
    {
        return {};
    }

    RNS::CSB csb = {
        .ptr = (char*)allocation.ptr,
        .len = 0,
        .cap = (s64)allocation.size,
    };

    rc = fseek(file_handle, 0, SEEK_SET);
    if (rc)
    {
        return {};
    }

    usize read = fread(csb.ptr, 1, length, file_handle);
    if (read != length)
    {
        return {};
    }

    csb.len = (s64)read;

    rc = fclose(file_handle);
    if (rc)
    {
        return {};
    }

    return csb;
}

// @TODO: remove, this is just for rapid development
RNS::Allocator RNS::default_create_allocator(s64 size)
{
    auto* ptr = reinterpret_cast<u8*>(RNS::virtual_alloc(nullptr, size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 }));
    assert(ptr);
    RNS::Allocator allocator = {
        .pool = {
            .ptr = ptr,
            .used = 0,
            .cap = size,
        },
        .allocate = RNS::linear_allocate,
    };

    return allocator;
}

RNS::Allocator RNS::default_create_execution_allocator(s64 size)
{
    auto* ptr = reinterpret_cast<u8*>(RNS::virtual_alloc(nullptr, size, { .commit = 1, .reserve = 1, .execute = 1, .read = 1, .write = 1 }));
    assert(ptr);
    RNS::Allocator allocator = {
        .pool = {
            .ptr = ptr,
            .used = 0,
            .cap = size,
        },
        .allocate = RNS::linear_allocate,
    };

    return allocator;
}

void RNS::default_free_allocator(Allocator* allocator)
{
    virtual_free(allocator->pool.ptr);
    memset(allocator, 0, sizeof(*allocator));
}

void print(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}
void println(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    puts("");
}

extern s32 rns_main(s32 argc, char* argv[]);

s32 main(s32 argc, char* argv[])
{
    RNS::os_init();
    return rns_main(argc, argv);
}
