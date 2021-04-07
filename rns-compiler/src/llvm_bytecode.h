#pragma once
#include <RNS/types.h>
#include "compiler_types.h"


namespace RNS
{
    using namespace AST;
    void encode(Compiler& compiler, NodeBuffer& node_buffer, FunctionTypeBuffer& function_type_declarations, FunctionDeclarationBuffer& function_declarations);
}