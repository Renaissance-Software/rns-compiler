#pragma once
#include <RNS/types.h>
#include "compiler_types.h"

struct RNS::Allocator;

namespace LLVM
{
    void encode(Compiler& compiler, AST::Result* parser_result);
}
