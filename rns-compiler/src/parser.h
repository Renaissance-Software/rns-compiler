#pragma once
#include <RNS/types.h>
#include "compiler_types.h"

AST::Result parse(Compiler& compiler, LexerResult& lexer_result, TypeBuffer& type_declarations);
