#pragma once

#include <RNS/types.h>
#include <RNS/data_structures.h>
#include "compiler_types.h"

RNS::DebugAllocator create_allocator(s64 size);
s32 x86_64_test_main(s32 argc, char* argv[]);

namespace WASMBC
{
    struct InstructionStruct;
    struct FunctionEncoder;
}

void jit_bytecode(Bytecode::IR* ir);
void jit_wasm(RNS::Buffer<WASMBC::InstructionStruct>* buffer, WASMBC::FunctionEncoder* encoder);
