#pragma once
#include <RNS/types.h>
#include "compiler_types.h"

namespace LLVM
{
    void encode(Parser::Parser* module_parser, RNS::Allocator* allocator);
}
