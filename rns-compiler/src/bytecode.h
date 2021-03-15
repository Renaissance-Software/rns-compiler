#pragma once
#include <RNS/types.h>
#include "compiler_types.h"
Bytecode::IR generate_ir(Parser::ModuleParser* m, RNS::Allocator* instruction_allocator, RNS::Allocator* value_allocator);