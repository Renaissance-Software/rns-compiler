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
    errors_reported = true;
    const char* subsystem_name = subsystem_names[static_cast<u32>(subsystem)];

    printf("[Error in the %s] %s:%u col:%u %s\t", subsystem_name, context.filename, context.line_number, context.column_number, context.function_name);

    va_list va_args;
    va_start(va_args, message_format);
    vfprintf(stdout, message_format, va_args);
    va_end(va_args);

    fputc('\n', stdout);
}
