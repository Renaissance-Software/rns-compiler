#pragma once

#include <RNS/types.h>
#include <RNS/data_structures.h>

static inline RNS::DebugAllocator create_allocator(s64 size)
{
    void* address = RNS::virtual_alloc(nullptr, size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 });
    assert(address);
    RNS::DebugAllocator allocator = RNS::DebugAllocator::create(address, size);
    return allocator;
}

void write_executable();