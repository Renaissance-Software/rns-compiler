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

    char error_message[1024]{};
#define READ_FROM_FILE 0
    const char* file_ptr;
    u32 file_len;
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

    file_ptr = file_buffer.ptr;
    file_len = static_cast<u32>(file_buffer.len);
#else
    const char file[] = "main :: () -> s32 { a : s32 = 5; b: s32 = 4; return a + b + 5; }";

    file_ptr = file;
    file_len = rns_array_length(file);
#endif

    DebugAllocator lexer_allocator = create_allocator(RNS_MEGABYTE(100));
    DebugAllocator name_allocator = create_allocator(RNS_MEGABYTE(20));

    TokenBuffer tb = lex(&lexer_allocator, &name_allocator, file_ptr, file_len);

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

#if SL_INSTR
    PerformanceAPI_EndEvent();
#endif
    return 0;
}