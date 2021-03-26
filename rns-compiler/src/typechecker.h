#pragma once
#include <RNS/types.h>
#include "compiler_types.h"

namespace Typing
{
    extern Type void_type;
    extern Type s8_type;
    extern Type s16_type;
    extern Type s32_type;
    extern Type s64_type;
    extern Type u8_type;
    extern Type u16_type;
    extern Type u32_type;
    extern Type u64_type;

    bool typecheck(Type* type1, Type* type2);
    void register_native_types(TypeBuffer& type_buffer);
    Type* get_native_type(TypeBuffer& type_declarations, const char* type_name, u32 len);
}
