#include "typechecker.h"

bool Typing::typecheck(Type* type1, Type* type2)
{
    return false;
}

void Typing::Type::init(Allocator* allocator, s64 element_count)
{
    type_buffer = type_buffer.create(allocator, element_count);

    const char* unsigned_names[] = { "u8", "u16", "u32", "u64" };
    const char* signed_names[] = { "s8", "s16", "s32", "s64" };
    const u16 integer_bits[] = { 8, 16, 32, 64 };
    const int integer_name_lengths[] = { 2, 3, 3, 3 };

    for (auto i = 0; i < rns_array_length(integer_bits); i++)
    {
        Type unsigned_type = {
            .id = TypeID::IntegerType,
            .name = { unsigned_names[i], integer_name_lengths[i] },
            .integer_t = {
                .bits = integer_bits[i],
                .is_signed = false,
            }
        };
        type_buffer.append(unsigned_type);
        Type signed_type = {
            .id = TypeID::IntegerType,
            .name = { signed_names[i], integer_name_lengths[i] },
            .integer_t = {
                .bits = integer_bits[i],
                .is_signed = true,
            }
        };
        type_buffer.append(signed_type);
    }

    Type void_type = {
        .id = TypeID::VoidType,
        .name = StringFromCStringLiteral("none"),
    };
    type_buffer.append(void_type);

    Type bool_type = {
        .id = TypeID::IntegerType,
        .name = StringFromCStringLiteral("bool"),
        .integer_t = {
            .bits = 1,
            .is_signed = false
        }
    };

    type_buffer.append(bool_type);
    Type label_type = {
        .id = TypeID::LabelType,
        .name = "label",
    };
    type_buffer.append(label_type);
}

Type* Typing::Type::get_integer_type(u16 bits, bool is_signed)
{
    switch (bits)
    {
        case 8:
        case 16:
        case 32:
        case 64:
        {
            return &type_buffer[2 * (bits / 8) + is_signed];
        } break;
        default:
            break;
    }

    return nullptr;
}

Type* Typing::Type::get(RNS::String name)
{
    for (auto& type : type_buffer)
    {
        if (type.name.equal(name))
        {
            return &type;
        }
    }

    return nullptr;
}

Type* Typing::Type::get_label()
{
    return Type::get(StringFromCStringLiteral("label"));
}
Type* Typing::Type::get_bool_type()
{
    return Type::get(StringFromCStringLiteral("bool"));
}

Type* Typing::Type::get_void_type()
{
    return Type::get(StringFromCStringLiteral("none"));
}

TypeBuffer Typing::Type::type_buffer;


