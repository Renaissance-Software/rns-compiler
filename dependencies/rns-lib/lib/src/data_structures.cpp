#include <RNS/arch.h>
#include <RNS/data_structures.h>
#include <stdio.h>

RNS::Allocation RNS::linear_allocate(Allocator* allocator, s64 size)
{
    s64 necessary_pool_size = allocator->pool.used + size;
    bool preconditions = size > 0;
    preconditions = preconditions && necessary_pool_size <= allocator->pool.cap;
    assert(preconditions);

    if (preconditions)
    {
        u8* new_address = allocator->pool.ptr + allocator->pool.used;
        allocator->pool.used += size;
        return { new_address, size };
    }
    else
    {
        printf("Allocation of %I64d bytes failed\n", size);
        return {};
    }

    RNS::Allocation allocation = {
        .ptr = allocator->pool.ptr + allocator->pool.used,
        .size = size,
    };

    allocator->pool.used += size;

    return allocation;
}

RNS::Allocator RNS::create_suballocator(Allocator* allocator, s64 new_allocator_capacity)
{
    Allocator new_allocator = {
        .pool = {
            .ptr = allocator->allocate(allocator, new_allocator_capacity).ptr,
            .used = 0,
            .cap = new_allocator_capacity,
        },
        .allocate = linear_allocate,
    };

    return new_allocator;
}


const u32 max_allocation_count = 1000000;
struct AllocationTrack
{
    RNS::Allocation allocations[max_allocation_count];
    u32 allocation_count;
};

static AllocationTrack debug = {};

void* operator new(usize size, RNS::Allocator* allocator) noexcept
{
    assert(size < INT64_MAX);
    assert(debug.allocation_count < max_allocation_count);

    RNS::Allocation* allocation = &debug.allocations[debug.allocation_count++];
    *allocation = allocator->allocate(allocator, (s64)size);

    return allocation->ptr;
}
void* operator new[](usize size, RNS::Allocator* allocator) noexcept
{
    assert(size < INT64_MAX);
    assert(debug.allocation_count < max_allocation_count);

    RNS::Allocation* allocation = &debug.allocations[debug.allocation_count++];
    *allocation = allocator->allocate(allocator, (s64)size);

    return allocation->ptr;
}

template<typename T, const u8 _count_ = (RNS_CACHE_LINE_SIZE - sizeof(u64)) / sizeof(T)> 
struct BucketArray
{
    T arr[_count_];
    BucketArray* next;
    static_assert(sizeof(BucketArray<T>) <= RNS_CACHE_LINE_SIZE);
};

struct HashBucketTable
{

};