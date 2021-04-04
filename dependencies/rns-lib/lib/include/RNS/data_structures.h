#pragma once

#include <RNS/types.h>
#include <string.h>
#include <immintrin.h>

#define VIRTUALIZE 1


#define AllocatorFnType(_name_) Allocation _name_(Allocator* allocator, AllocatorSizeType size)
#define FreeFnType(_name_) void _name_(Allocator* allocator, Allocation* allocation)
namespace RNS { struct Allocator; }
void* operator new(usize size, RNS::Allocator* allocator) noexcept;
void* operator new[](usize size, RNS::Allocator* allocator) noexcept;

namespace RNS
{
    using AllocatorSizeType = s64;
    struct AllocatorPool
    {
        u8* ptr;
        AllocatorSizeType used;
        AllocatorSizeType cap;
    };

    struct Allocation
    {
        u8* ptr;
        AllocatorSizeType size;
    };

    typedef AllocatorFnType(AllocateFn);
    typedef FreeFnType(FreeFn);

    struct Allocator
    {
        AllocatorPool pool;
        AllocateFn* allocate;
        FreeFn* free;
    };
    Allocation linear_allocate(Allocator* allocator, s64 size);
    Allocator create_suballocator(Allocator* allocator, s64 new_allocator_capacity);
    Allocator default_create_allocator(s64 size);
    Allocator default_create_execution_allocator(s64 size);
    void default_free_allocator(Allocator* allocator);

    struct CString
    {
        union
        {
            struct
            {
                char* ptr;
                s64 len;
                s64 cap;
            };
            char static_buffer[23];
        };
        bool static_allocated;
    };

    typedef CString CSB;

    struct StringBuffer
    {
        char* ptr;
        s64 len;
        s64 cap;

        static StringBuffer create(Allocator* allocator, s64 size)
        {
            StringBuffer sb =
            {
                .ptr = new(allocator) char[size],
                .len = 0,
                .cap = size,
            };

            return sb;
        }

        inline char* append(const char* str, s64 size)
        {
            assert(len + size < cap);
            char* result = ptr + len;
            memcpy(result, str, size);
            len += size;
            ptr[len++] = 0;
            return result;
        }

        inline char* append(char* str)
        {
            auto string_len = strlen(str);
            return append(str, string_len);
        }
    };

    struct SmallStringSlice
    {
        u64 ptr : 48;
        u64 len : 16;
    };

    inline u64 align(u64 n, usize align)
    {
        const usize mask = align - 1;
        assert((align & mask) == 0);
        return (n + mask) & ~mask;
    }

    inline u64 next_power_of_two(u64 n)
    {
        u64 leading_zero_count = _lzcnt_u64(n);
        constexpr auto data_bit_count = (sizeof(n) * 8);
        u64 result = (u64)1 << (data_bit_count - leading_zero_count);
        return result;
    }

    template<typename T>
    struct StaticCompactBuffer
    {
        u64 address : 48;
        u8 len;
        u8 cap;
        T* get_ptr()
        {
            return reinterpret_cast<T*>(address);
        }

        T& operator[](usize index)
        {
            return get_ptr()[index];
        }
    };

    // @Info: this struct occupies 16 bytes (128 bits) and it's meant to be used as a buffer with just an allocation
    template <typename T>
    struct OneTimeAllocationBuffer
    {
        using SizeType = u32;
        T* ptr;
        SizeType len, cap;

        static OneTimeAllocationBuffer<T> create(Allocator* allocator, SizeType count)
        {
            assert(allocator);
            assert(count > 0);

            OneTimeAllocationBuffer<T> buffer = {
                .ptr = new(allocator) T[count],
                .len = 0,
                .cap = count,
            };
            assert(buffer.ptr);

            return buffer;
        }
    };
    
    template <typename T>
    struct Buffer
    {
        T* ptr;
        s64 len; s64 cap;
        Allocator* allocator;

        static Buffer<T> create(Allocator* allocator, s64 count)
        {
            assert(allocator);
            assert(count > 0);

            Buffer<T> buffer = {
                .ptr = new(allocator) T[count],
                .len = 0,
                .cap = count,
                .allocator = allocator,
            };
            assert(buffer.ptr);

            return buffer;
        }

        void ensure_init()
        {
            assert(this);
            assert(cap);
        }

        T* allocate(s64 count)
        {
            ensure_init();
            assert(len + count <= cap);
            auto result = &ptr[len];
            len += count;
            return result;
        }

        T* allocate()
        {
            return allocate(1);
        }

        T* append(T element)
        {
            ensure_init();
            assert(len + 1 <= cap);
            auto index = len;
            ptr[index] = element;
            len++;
            return &ptr[index];
        }
        
        T* get(s64 id)
        {
            ensure_init();
            assert(id < len);
            return &ptr[id];
        }

        s64 get_id_if_ref_buffer(T value)
        {
            for (s64 i = 0; i < len; i++)
            {
                if (ptr[i] == value)
                {
                    return i;
                }
            }

            assert(false);
            return 0;
        }

        s64 get_id(T* value)
        {
            ensure_init();
            assert(value - ptr < len);
            assert(value >= ptr);
            return static_cast<s64>(value - ptr);
        }

        T* begin()
        {
            return ptr;
        }

        T* end()
        {
            return ptr + len;
        }

        void push(T e)
        {
            ensure_init();
            assert(len + 1 <= cap);
            ptr[len++] = e;
        }

        T pop()
        {
            ensure_init();
            assert(len);
            return ptr[len-- - 1];
        }

        T* insert_after(T element, T* after_this_one)
        {
            assert(after_this_one >= ptr && after_this_one < ptr + len);
            return insert_at(element, s64(after_this_one - ptr) + 1);
        }

        T* insert_at(T element, s64 index)
        {
            assert(len + 1 <= cap);
            assert(index <= len);

            if (index < len)
            {
                memmove(&ptr[index + 1], &ptr[index], sizeof(T) * (len - index));
            }

            ptr[index] = element;
            len++;
            return &ptr[index];
        }

        T& operator[](s64 index)
        {
            assert(index < len);
            return ptr[index];
        }

        // @Warning: use this carefully
        void put_at(T element, usize index)
        {
            assert(index < cap);
            if (len <= index)
            {
                len = index + 1;
            }
            ptr[index] = element;
        }
    };

    template <typename T>
    struct Slice
    {
        T* ptr;
        s64 len;

        T* begin()
        {
            return ptr;
        }

        T* end()
        {
            return ptr + len;
        }
        T& operator[](s64 index)
        {
            assert(index < len);
            return ptr[index];
        }
    };

    struct String
    {
        const char* ptr;
        s64 len;

        bool equal(const char* str)
        {
            bool equal_portion = strncmp(ptr, str, len) == 0;
            if (!equal_portion)
            {
                return false;
            }
            if (str[len])
            {
                return false;
            }
            return true;
        }

        const char* begin()
        {
            return ptr;
        }

        const char* end()
        {
            return ptr + len;
        }

        bool equal(String& str)
        {
            if (len != str.len)
            {
                return false;
            }

            bool equal = memcmp(ptr, str.ptr, len) == 0;
            return equal;
        }

        const char& operator[](usize index)
        {
            return ptr[index];
        }
    };

    // @Info: in theory, array length gets null-terminated character (that's why the -1)
#define StringFromCStringLiteral(_cstr_) { _cstr_, rns_array_length(_cstr_) - 1 }

    template <typename Type1, typename Type2>
    struct Pair
    {
        Type1 first;
        Type2 second;
    };

    template<typename Type1, typename Type2>
    struct PairBuffer
    {
        Type1* first;
        Type2* second;
        // @Here we need to store the allocator too and we need to shrink the buffer size to fit it all in 8 * 4 bytes
        u32 len;
        u32 cap;
        Allocator* allocator;

        static PairBuffer<Type1, Type2> create(Allocator* allocator, u32 count)
        {
            assert(allocator);
            assert(count > 0);

            PairBuffer<Type1, Type2> pair_buffer;

            pair_buffer = {
                .first = new(allocator) Type1[count],
                .second = new(allocator) Type2[count],
                .len = 0,
                .cap = count,
                .allocator = allocator,
            };
            assert(pair_buffer.first);
            assert(pair_buffer.second);

            return pair_buffer;
        }

        void ensure_init()
        {
            assert(this);
            assert(cap > 0);
        }

        Pair<Type1*, Type2*> allocate(u32 count)
        {
            ensure_init();
            assert(len + count <= cap);
            auto result = { &first[len], &second[len] };
            len += count;
            return result;
        }

        Pair<Type1*, Type2*> allocate()
        {
            return allocate(1);
        }

        Pair<Type1*, Type2*> append(Type1 element1, Type2 element2)
        {
            ensure_init();
            assert(len + 1 <= cap);
            auto index = len;
            first[index] = element1;
            second[index] = element2;
            len++;
            return { &first[index], &second[index] };
        }
        
        Pair<Type1*, Type2*> get(u32 id)
        {
            ensure_init();
            assert(id < len);
            return { &first[id], &second[id] };
        }

        u32 get_id(Pair<Type1*, Type2*> pair)
        {
            ensure_init();
            assert(pair->first - first < len);
            assert(pair->second - second < len);
            assert(pair->first - first == pair->second - second);
            return pair->first - first;
        }

        void push(Type1 e1, Type2 e2)
        {
            ensure_init();
            assert(len + 1 <= cap);
            first[len] = e1;
            second[len] = e2;
            len++;
        }

        Pair<Type1, Type2> pop()
        {
            ensure_init();
            assert(len > 0);
            auto index = len - 1;
            Pair<Type1, Type2> popped_value = { first[index], second[index] };
            len = index;
            return popped_value;
        }

        Pair<Type1&, Type2&> operator[](usize index)
        {
            return { first[index], second[index] };
        }
    };
}
