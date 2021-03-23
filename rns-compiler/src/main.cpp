#include <RNS/types.h>
#include <RNS/arch.h>
#include <RNS/compiler.h>
#include <RNS/os.h>
#include <RNS/os_internal.h>
#include <RNS/data_structures.h>

#define USE_LLVM 0
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "bytecode.h"
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
#ifndef RNS_DEBUG
    s64 freq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    s64 start, end;
    QueryPerformanceCounter((LARGE_INTEGER*)&start);
#endif
    if (argc < 2)
    {
        printf("Not enough arguments\n");
        return -1;
    }

    const char* filename = argv[1];
    assert(filename);

    char error_message[1024]{};

    DebugAllocator file_allocator = create_allocator(RNS_MEGABYTE(1));
    auto file_buffer = RNS::file_load(filename, &file_allocator);
    if (!file_buffer.ptr)
    {
        printf("File %s not found\n", filename); 
        return -1;
    }

    DebugAllocator lexer_allocator = create_allocator(RNS_MEGABYTE(100));
    DebugAllocator name_allocator = create_allocator(RNS_MEGABYTE(20));

    TokenBuffer tb = lex(&lexer_allocator, &name_allocator, file_buffer.ptr, static_cast<u32>(file_buffer.len));

    DebugAllocator node_allocator = create_allocator(RNS_MEGABYTE(200));
    DebugAllocator function_allocator = create_allocator(RNS_MEGABYTE(100));
    DebugAllocator general_allocator = create_allocator(RNS_MEGABYTE(100));

    auto parser_result = parse(&tb, &node_allocator, &function_allocator, &general_allocator, error_message);
    if (parser_result.failed())
    {
        printf("Parsing failed. Error message: %s\n", parser_result.error_message);
        return -1;
    }

    DebugAllocator bc_allocator = create_allocator(RNS_MEGABYTE(100));
    auto wasm_result = WASMBC::encode(&parser_result, &bc_allocator);


#if USE_LLVM
    CompilerIR compiler_ir = CompilerIR::LLVM;
    LLVM::encode(&parser, &ir_allocator);
#else
    CompilerIR compiler_ir = CompilerIR::WASM;
    switch (compiler_ir)
    {
        case CompilerIR::WASM:
        {
            DebugAllocator mc_allocator = create_allocator(RNS_MEGABYTE(100));
            jit_wasm(&wasm_result.ib, wasm_result.stack_pointer_id);
        } break;
        case CompilerIR::LLVM_CUSTOM:
        {
            //LLVMIR::encode(&parser, &ir_allocator);
        } break;
        default:
            RNS_UNREACHABLE;
            break;
    }
#endif

#ifndef RNS_DEBUG
    QueryPerformanceCounter((LARGE_INTEGER*)&end);
    auto diff = end - start;
    auto us = 1000000;
    auto diff_us = diff * us;
    auto time_us = (double)diff_us / (double)freq;
    printf("Time: %Lf us\n", time_us);
#endif
    return 0;
}