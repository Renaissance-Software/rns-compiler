#pragma once
#include <RNS/types.h>
#include <RNS/data_structures.h>
#include "compiler_types.h"
LexerResult lex(Compiler& compiler, TypeBuffer& type_declarations, RNS::String file_content);