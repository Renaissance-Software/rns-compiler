#pragma once
#include <RNS/types.h>
#include <RNS/data_structures.h>
#include "compiler_types.h"
LexerResult lex(Compiler& compiler, RNS::String file_content, TypeBuffer& type_declarations);