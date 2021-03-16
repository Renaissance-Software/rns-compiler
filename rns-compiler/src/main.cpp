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

#include "file.h"

using namespace RNS;

s32 rns_main(s32 argc, char* argv[])
{
    DebugAllocator lexer_allocator = create_allocator(RNS_MEGABYTE(100));
    DebugAllocator name_allocator = create_allocator(RNS_MEGABYTE(20));

    TokenBuffer tb = lex(&lexer_allocator, &name_allocator, file, rns_array_length(file));

    DebugAllocator node_allocator = create_allocator(RNS_MEGABYTE(200));
    DebugAllocator node_id_allocator = create_allocator(RNS_MEGABYTE(100));
    DebugAllocator tld_allocator = create_allocator(RNS_MEGABYTE(100));

    Parser::ModuleParser module_parser = parse(&tb, &node_allocator, &node_id_allocator, &tld_allocator);
    if (module_parser.failed())
    {
        printf("Parsing failed! Error: %s\n", module_parser.error_message);
        return -1;
    }
    else
    {
        printf("Parsing finished successfully!\n");
    }

    DebugAllocator instruction_allocator = create_allocator(RNS_MEGABYTE(300));
    DebugAllocator value_allocator = create_allocator(RNS_MEGABYTE(300));
    Bytecode::IR ir = generate_ir(&module_parser, &instruction_allocator, &value_allocator);
    //for (auto i = 0; i < ir.ib.len; i++)
    //{
    //    auto instruction = ir.ib.ptr[i];
    //    instruction.print(&ir.vb);
    //}

    interpret(&ir);

    return wna_main(argc, argv);
}