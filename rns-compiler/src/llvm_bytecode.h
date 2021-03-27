#pragma once
#include <RNS/types.h>
#include "compiler_types.h"

namespace LLVM
{
    using namespace AST;
    using namespace RNS;
    void encode(Compiler& compiler, NodeBuffer& node_buffer, FunctionTypeBuffer& function_type_declarations, TypeBuffer& type_declarations, FunctionDeclarationBuffer& function_declarations);
}