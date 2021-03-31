#include <RNS/types.h>
#include <RNS/arch.h>
#include <RNS/compiler.h>
#include <RNS/os.h>
#include <RNS/os_internal.h>
#include <RNS/data_structures.h>
#include <RNS/profiler.h>


#define USE_LLVM 0
#include <stdio.h>
#include <stdlib.h>

#include "compiler_types.h"
#include "typechecker.h"
#include "lexer.h"
#include "parser.h"
#include "llvm_bytecode.h"
#if USE_LLVM
#include "llvm_backend.h"
#endif
#include "x86_64.h"

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
    TypeBuffer type_declarations = TypeBuffer::create(&type_allocator, 1024);
    register_native_types(type_declarations);

    LexerResult lexer_result = lex(compiler, type_declarations, file);
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

#if USE_LLVM
    CompilerIR compiler_ir = CompilerIR::LLVM;
    LLVM::encode(&bc_allocator, &parser_result);
#else
    CompilerIR compiler_ir = CompilerIR::LLVM_CUSTOM;
    switch (compiler_ir)
    {
        case CompilerIR::WASM:
        {
            //auto wasm_result = WASMBC::encode(&parser_result, &bc_allocator);
            //DebugAllocator mc_allocator = create_allocator(RNS_MEGABYTE(100));
            //jit_wasm(&wasm_result.ib, wasm_result.stack_pointer_id);
        } break;
        case CompilerIR::LLVM_CUSTOM:
        {
            LLVM::encode(compiler, parser_result.node_buffer, parser_result.function_type_declarations, type_declarations, parser_result.function_declarations);
        } break;
        default:
            RNS_UNREACHABLE;
            break;
    }
#endif
    if (compiler.errors_reported)
    {
        printf("IR generation failed\n");
        return false;
    }

    default_free_allocator(&compiler.page_allocator);
    return true;
}

#include "test_files.h"

s32 rns_main(s32 argc, char* argv[])
{
#if SL_INSTR
    PerformanceAPI_BeginEvent("Main function", nullptr, PERFORMANCEAPI_DEFAULT_COLOR);
#endif

#define TEST_FILES 0
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
        NEW_TEST(main :: () -> s32
    {
    sum: s32 = 0;
    for i : 4
    {
        if i > 1
        {
            if i == 2
            {
                sum = sum * sum;
                break;
            }
            sum = sum + 1;
        }
        else
        {
            sum = sum + 12;
            if i == 3
            {
                sum = sum + 1231;
                if sum == 1231
                {
                    for j : 10
                    {
                        sum = sum + j;
                    }
                }
                sum = sum + sum;
            }
            else if i == sum
            {
                sum = sum - 100;
            }
            else
            {
                sum = sum - 3;
                break;
            }
            sum = sum + 2;
        }
        sum = sum + i;
    }
    return sum;
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
