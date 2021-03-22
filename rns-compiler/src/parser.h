#pragma once
#include <RNS/types.h>
#include "compiler_types.h"

AST::Result parse(TokenBuffer* tb, RNS::Allocator* node_allocator, RNS::Allocator* node_id_allocator, RNS::Allocator* tld_allocator, char* error_message);
