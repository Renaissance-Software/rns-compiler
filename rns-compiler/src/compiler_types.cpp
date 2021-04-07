#include "compiler_types.h"

#include <stdio.h>

const char* subsystem_names[] = {
    "lexer",
    "parser",
    "intermediate representation generation",
    "machine code generation",
};

static_assert(static_cast<u32>(Compiler::Subsystem::Count) == rns_array_length(subsystem_names));

void Compiler::print_error(MetaContext context, const char* message_format, ...)
{
    errors_reported++;
    const char* subsystem_name = subsystem_names[static_cast<u32>(subsystem)];

    printf("[Error in the %s] %s:%u col:%u %s\t", subsystem_name, context.filename, context.line_number, context.column_number, context.function_name);

    va_list va_args;
    va_start(va_args, message_format);
    vfprintf(stdout, message_format, va_args);
    va_end(va_args);

    fputc('\n', stdout);
}

Type* Type::get_pointer_type(Type* type, TypeBuffer& type_declarations)
{
    RNS_NOT_IMPLEMENTED;
    return nullptr;
}
Type* Type::get_label_type(TypeBuffer& type_declarations)
{
    RNS_NOT_IMPLEMENTED;
    return nullptr;
}

Type* Type::get_void_type(TypeBuffer& type_declarations)
{
    RNS_NOT_IMPLEMENTED;
    return nullptr;
}

TypeBuffer Type::init_type_system(Allocator* allocator)
{
    TypeBuffer type_declarations = type_declarations.create(allocator, 1024);
    auto create_base_type = [&](const char* name, TypeID id)
    {
        Type type = {
            .id = id,
            .name = RNS::StringView::create(name, strlen(name)),
        };
        return type;
    };
    auto create_int_type = [&](const char* name, u16 bits, bool is_signed)
    {
        auto type = create_base_type(name, TypeID::IntegerType);
        type.integer_t = {
            .bits = bits,
            .is_signed = is_signed,
        };

        return type;
    };

    auto create_float_type = [&](const char* name, u16 bits)
    {
        auto type = create_base_type(name, TypeID::IntegerType);
        RNS_NOT_IMPLEMENTED;
    };

    type_declarations.append(create_int_type("bool", 8, false));
    type_declarations.append(create_int_type("u8",  8, false));
    type_declarations.append(create_int_type("u16", 16, false));
    type_declarations.append(create_int_type("u32", 32, false));
    type_declarations.append(create_int_type("u64", 64, false));
    type_declarations.append(create_int_type("s8",  8,  true));
    type_declarations.append(create_int_type("s16", 16, true));
    type_declarations.append(create_int_type("s32", 32, true));
    type_declarations.append(create_int_type("s64", 64, true));
    // @TODO: add fp types

    return type_declarations;
}

Type* Type::get_array_type(Type* elem_type, s64 elem_count, TypeBuffer& type_declarations)
{
    for (auto& type : type_declarations)
    {
        if (type.id == TypeID::ArrayType && (u64)type.array_t.type == (u64)elem_type && type.array_t.count == elem_count)
        {
            return &type;
        }
    }

    Type type = {
        .id = TypeID::ArrayType,
        .array_t = {
            .type = elem_type,
            .count = elem_count,
        },
    };

    auto* result = type_declarations.append(type);
    return result;
}

Type* Type::get_integer_type(u16 bits, bool signedness, TypeBuffer& type_declarations)
{
    for (auto& type : type_declarations)
    {
        if (type.id == TypeID::IntegerType && type.integer_t.bits == bits && type.integer_t.is_signed == signedness)
        {
            return &type;
        }
    }

    RNS_NOT_IMPLEMENTED;
    return nullptr;
}
