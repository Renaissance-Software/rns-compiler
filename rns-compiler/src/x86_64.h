#pragma once

#include <RNS/types.h>
#include <RNS/data_structures.h>
#include <RNS/os.h>

RNS::DebugAllocator create_allocator(s64 size);
s32 wna_main(s32 argc, char* argv[]);
