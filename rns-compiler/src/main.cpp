#include <RNS/types.h>
#include <RNS/arch.h>
#include <RNS/compiler.h>
#include <RNS/os.h>
#include <RNS/os_internal.h>
#include <RNS/data_structures.h>

#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "bytecode.h"
#include "interpreter.h"
#include "x86_64.h"

using namespace RNS;

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

    //DebugAllocator instruction_allocator = create_allocator(RNS_MEGABYTE(300));
    //DebugAllocator value_allocator = create_allocator(RNS_MEGABYTE(300));
    //Bytecode::IR ir = generate_ir(&module_parser, &instruction_allocator, &value_allocator);
    ////for (auto i = 0; i < ir.ib.len; i++)
    ////{
    ////    auto instruction = ir.ib.ptr[i];
    ////    instruction.print(&ir.vb);
    ////}

    //interpret(&ir);

    //jit_bytecode(&ir);

    DebugAllocator ir_allocator = create_allocator(RNS_MEGABYTE(300));
    auto wasm_result = WASMBC::encode(&module_parser, &ir_allocator);
    jit_wasm(&wasm_result.ib, &wasm_result.encoder);

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