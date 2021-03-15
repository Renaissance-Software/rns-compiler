#pragma once
#include <RNS/types.h>
#include "compiler_types.h"

Parser::ModuleParser parse(TokenBuffer* tb, RNS::Allocator* node_allocator, RNS::Allocator* node_id_allocator, RNS::Allocator* tld_allocator);
