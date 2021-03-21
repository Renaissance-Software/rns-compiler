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
#include "interpreter.h"
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
    char buffer[1024];
    RNS::get_cwd(buffer, 1024);
    if (argc < 2)
    {
        printf("Not enough arguments\n");
        return -1;
    }
    const char* filename = argv[1];
    assert(filename);

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
    DebugAllocator node_id_allocator = create_allocator(RNS_MEGABYTE(100));
    DebugAllocator tld_allocator = create_allocator(RNS_MEGABYTE(100));

    Parser::ModuleParser module_parser = parse(&tb, &node_allocator, &node_id_allocator, &tld_allocator);
    if (module_parser.failed())
    {
        printf("Parsing failed! Error: %s\n", module_parser.error_message);
        return -1;
    }

    DebugAllocator ir_allocator = create_allocator(RNS_MEGABYTE(300));

#if USE_LLVM
    CompilerIR compiler_ir = CompilerIR::LLVM;
    LLVM::encode(&module_parser, &ir_allocator);
#else
    CompilerIR compiler_ir = CompilerIR::LLVM_CUSTOM;
    switch (compiler_ir)
    {
        case CompilerIR::WASM:
        {
            auto wasm_result = WASMBC::encode(&module_parser, &ir_allocator);
            jit_wasm(&wasm_result.ib, &wasm_result.encoder);
        } break;
        case CompilerIR::LLVM_CUSTOM:
        {
            LLVMIR::encode(&module_parser, &ir_allocator);
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