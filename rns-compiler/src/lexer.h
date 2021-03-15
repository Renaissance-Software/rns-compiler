#pragma once
#include <RNS/types.h>
#include "compiler_types.h"
TokenBuffer lex(RNS::Allocator* token_allocator, RNS::Allocator* name_allocator, const char* file, u32 file_size);