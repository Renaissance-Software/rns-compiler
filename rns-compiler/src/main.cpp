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

s32 rns_main(s32 argc, char* argv[])
{
#ifdef SL_INSTR
#endif
    if (argc < 2)
    {
        printf("Not enough arguments\n");
        return -1;
    }

#if SL_INSTR
    PerformanceAPI_BeginEvent("Main function", nullptr, PERFORMANCEAPI_DEFAULT_COLOR);
#endif

    char print_error[1024]{};

    RNS::String file;

#define READ_FROM_FILE 0

#if READ_FROM_FILE
    const char* filename = argv[1];
    assert(filename);


    DebugAllocator file_allocator = create_allocator(RNS_MEGABYTE(1));
    auto file_buffer = RNS::file_load(filename, &file_allocator);
    if (!file_buffer.ptr)
    {
        printf("File %s not found\n", filename); 
        return -1;
    }

    file.ptr = file_buffer.ptr;
    file.len = static_cast<u32>(file_buffer.len);
#else
    const char file_content[] = "main :: () -> s32 { a: s32 = 5; if a == 0 { return 1; } else { return 0; } }";

    file.ptr = (char*)file_content;
    file.len = rns_array_length(file_content);
#endif

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
        return -1;
    }

    auto parser_result = parse(compiler, lexer_result);
    if (compiler.errors_reported)
    {
        printf("Parsing failed.\n");
        return -1;
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

#if SL_INSTR
    PerformanceAPI_EndEvent();
#endif
    return 0;
}