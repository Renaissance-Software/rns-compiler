#include <RNS/types.h>
#include <RNS/arch.h>
#include <RNS/compiler.h>
#include <RNS/os.h>
#include <RNS/os_internal.h>
#include <RNS/data_structures.h>
#include <RNS/profiler.h>

#include <stdio.h>
#include <stdlib.h>

#define USE_IMGUI 0
#define USE_LLVM 0
#define TEST_FILES 0
#include "test_files.h"
#if TEST_FILES
#undef USE_IMGUI
#define USE_IMGUI 0
#endif

#include "compiler_types.h"
#include "lexer.h"
#include "parser.h"
#include "llvm_bytecode.h"

using namespace RNS;

enum class CompilerIR
{
    WASM,
    LLVM_CUSTOM,
#if USE_LLVM
    LLVM,
#endif
};

bool compiler_workflow(RNS::String file)
{
    Compiler compiler = {
        .page_allocator = default_create_allocator(RNS_GIGABYTE(1)),
        .common_allocator = create_suballocator(&compiler.page_allocator, RNS_MEGABYTE(100)),
        .errors_reported = false,
    };

    Allocator type_allocator = create_suballocator(&compiler.page_allocator, RNS_MEGABYTE(5));
    TypeBuffer type_declarations = Type::init_type_system(&type_allocator);

    LexerResult lexer_result = lex(compiler, file, type_declarations);
    if (compiler.errors_reported)
    {
        printf("Lexer failed!\n");
        return false;
    }

    auto parser_result = parse(compiler, lexer_result, type_declarations);
    if (compiler.errors_reported)
    {
        printf("Parsing failed.\n");
        return false;
    }

    CompilerIR compiler_ir = CompilerIR::LLVM_CUSTOM;
    switch (compiler_ir)
    {
        case CompilerIR::LLVM_CUSTOM:
        {
            RNS::encode(compiler, parser_result.node_buffer, parser_result.function_type_declarations, parser_result.function_declarations);
        } break;
        default:
            RNS_UNREACHABLE;
            break;
    }

    if (compiler.errors_reported)
    {
        printf("IR generation failed\n");
        return false;
    }

    default_free_allocator(&compiler.page_allocator);
    return true;
}


s32 rns_main(s32 argc, char* argv[])
{
#if SL_INSTR
    PerformanceAPI_BeginEvent("Main function", nullptr, PERFORMANCEAPI_DEFAULT_COLOR);
#endif

#if TEST_FILES
    for (auto i = 0; i < rns_array_length(test_files); i++)
    {
        printf("Test %d: %s\n", i + 1, test_files[i].ptr);
        bool result = compiler_workflow(test_files[i]);
        if (result)
        {
            printf("Test %d passed\n", i + 1);
        }
        else
        {
            printf("Test %d failed\n", i + 1);
            return -1;
        }
    }
#endif
    RNS::String working_test_case =
        NEW_TEST(
            main :: () -> s32
    {
        arr: [1]s32 = [3];
        return arr[0];
    }
    );
    bool result = compiler_workflow(working_test_case);
    if (result)
    {
        printf("Working test case passed\n");
    }
    else
    {
        printf("Working test case failed\n");
        return -1;
    }
#if SL_INSTR
    PerformanceAPI_EndEvent();
#endif
    return 0;
}
