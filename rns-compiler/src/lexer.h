#pragma once
#include <RNS/types.h>
#include <RNS/data_structures.h>
#include "compiler_types.h"
TokenBuffer lex(Compiler& compiler, TypeBuffer& type_declarations, RNS::String file_content);