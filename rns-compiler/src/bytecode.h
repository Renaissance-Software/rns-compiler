#pragma once
#include <RNS/types.h>
#include "compiler_types.h"

Bytecode::IR generate_ir(Parser::ModuleParser* m, RNS::Allocator* instruction_allocator, RNS::Allocator* value_allocator);
namespace WASMBC {
    extern const u32 no_value;
    extern const u32 __stack_pointer;
    enum class Instruction : u8
    {
        unreachable = 0x00,
        nop = 0x01,
        ret = 0x0F,
        call = 0x10,
        call_indirect = 0x11,

        local_get = 0x20,
        local_set = 0x21,
        local_tee = 0x22,
        global_get = 0x23,
        global_set = 0x24,

        i32_load = 0x28,
        i32_store = 0x36,

        i32_const = 0x41,

        i32_eqz = 0x45,
        i32_eq = 0x46,
        i32_ne = 0x47,
        i32_lt_s = 0x48,
        i32_lt_u = 0x49,
        i32_gt_s = 0x4A,
        i32_gt_u = 0x4B,
        i32_le_s = 0x4C,
        i32_le_u = 0x4D,
        i32_ge_s = 0x4E,
        i32_ge_u = 0x4F,

        i32_clz = 0x67,
        i32_ctz = 0x68,
        i32_popcnt = 0x69,
        i32_add = 0x6A,
        i32_sub = 0x6B,
        i32_mul = 0x6C,
        i32_div_s = 0x6D,
        i32_div_u = 0x6E,
        i32_rem_s = 0x6F,
        i32_rem_u = 0x70,
        i32_and = 0x71,
        i32_or = 0x72,
        i32_xor = 0x73,
        i32_shl = 0x74,
        i32_shr_s = 0x75,
        i32_shr_u = 0x76,
        i32_rotl = 0x77,
        i32_rotr = 0x78,

        i32_wrap_i64 = 0xA7,
        // ...
    };

    struct InstructionStruct
    {
        Instruction id;
        u32 value;

        void print();
        const char* id_to_string();
    };

    using InstructionBuffer = RNS::Buffer<InstructionStruct>;
    using IDBuffer = RNS::Buffer<u32>;

    struct ScopeStorage
    {
        IDBuffer node_id_buffer;
        IDBuffer bc_id_buffer;
        IDBuffer stack_offset;
    };
    struct FunctionEncoder
    {
        Parser::ModuleParser* parser;
        InstructionBuffer* instructions;
        u32 next_id;
        u32 stack_size;
        u32 stack_occupied;
        u32 stack_pointer_id;
        ScopeStorage scope_storage;

        void stack_allocate(u32 size);
        void instr(Instruction instruction, u32 value = no_value);
        u32 process_statement(u32 node_id, NativeTypeID type = NativeTypeID::None);
        NativeTypeID get_type(u32 type_node_id);
        void scan_through_function(Parser::Node* node);
        void print_instructions(s64 start = 0);
        u32 find_size(NativeTypeID native_type);
        void local_get(u32 index);
        u32 local_set();
        void local_set(u32 index);
        void encode(RNS::Allocator* allocator, Parser::ModuleParser* parser, InstructionBuffer* instructions, Parser::Node* node);
    };

    struct Result
    {
        InstructionBuffer ib;
        FunctionEncoder encoder;
    };
    Result encode(Parser::ModuleParser* module_parser, RNS::Allocator* allocator);
}
namespace LLVMIR
{
    void encode(Parser::ModuleParser* module_parser, RNS::Allocator* allocator);
}