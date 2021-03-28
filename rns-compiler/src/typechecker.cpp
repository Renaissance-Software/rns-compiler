#include "typechecker.h"

bool Typing::typecheck(Type* type1, Type* type2)
{
    return false;
}

#define type_append(_id_)

const char* native_type_names[] = {
    "none",
    "u1",
    "u8", 
    "u16",
    "u32",
    "u64",
    "s8", 
    "s16",
    "s32",
    "s64",
};
const u32 native_type_count = rns_array_length(native_type_names);

void Typing::register_native_types(TypeBuffer& type_buffer)
{
    u16 integer_bits[] = { 8, 16, 32, 64 };

    type_buffer.append({ .id = TypeID::VoidType });
    type_buffer.append({
        .id = TypeID::IntegerType,
        .integer_t = {
            .bits = 1,
            .is_signed = false,
        },
        });

    for (auto bit_count : integer_bits)
    {
        type_buffer.append({
            .id = TypeID::IntegerType,
            .integer_t = {
                .bits = bit_count,
                .is_signed = false,
            } });
    }

    for (auto bit_count : integer_bits)
    {
        type_buffer.append({
            .id = TypeID::IntegerType,
            .integer_t = {
                .bits = bit_count,
                .is_signed = true,
            } });
    }

    assert(native_type_count == type_buffer.len);
}

Type* Typing::get_native_type(TypeBuffer& type_declarations, const char* type_name, u32 len)
{
    for (auto i = 0; i < native_type_count; i++)
    {
        auto native_typename = native_type_names[i];
        if (strlen(native_typename) == len && strncmp(native_typename, type_name, len) == 0)
        {
            return &type_declarations[i];
        }
    }

    return nullptr;
}




