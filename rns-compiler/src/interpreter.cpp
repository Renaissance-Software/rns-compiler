#include "interpreter.h"

#include <stdio.h>

namespace Interp
{
    struct Variable
    {
        s32 value;
    };

    struct Runtime
    {
        Interp::Variable variables[100];
        s64 ir_ids_reserved[30];
        s64 var_count = 0;
    };
}

Interp::Variable* get_local_variable(Interp::Runtime* runtime, s64 id)
{
    for (auto i = 0; i < runtime->var_count; i++)
    {
        if (id == runtime->ir_ids_reserved[i])
        {
            return &runtime->variables[i];
        }
    }

    return nullptr;
}
// @TODO: for now, we are only dealing with s32 values
s32 resolve_index(Bytecode::IR* ir, Interp::Runtime* runtime, s64 index)
{
    Interp::Variable* variable = get_local_variable(runtime, index);
    if (variable)
    {
        return variable->value;
    }

    auto value = ir->vb.get(index);

    switch (value.type)
    {
        case NativeTypeID::S32:
            return (s32)value.int_lit.lit;
        default:
            RNS_NOT_IMPLEMENTED;
            return 0;
    }
}

using Bytecode::InstructionID;

void interpret(Bytecode::IR* ir)
{
    s32 result = 0;
    auto instruction_count = ir->ib.len;

    Interp::Runtime runtime = {};
    memset(&runtime.ir_ids_reserved, 0xcccccccc, sizeof(runtime.ir_ids_reserved));

    for (auto i = 0; i < instruction_count; i++)
    {
        auto instruction = ir->ib.ptr[i];

        switch (instruction.id)
        {
            case InstructionID::Decl:
            {
                // This should be the equivalent of stack reserving
                auto id = instruction.operands[0];
                runtime.ir_ids_reserved[runtime.var_count++] = id;
            } break;
            case InstructionID::Add:
            {
                auto op1_id = instruction.operands[0];
                auto op2_id = instruction.operands[1];
                auto result_id = instruction.result;
                s32 op1_value = resolve_index(ir, &runtime, op1_id);
                s32 op2_value = resolve_index(ir, &runtime, op2_id);
                auto* result_variable = get_local_variable(&runtime, result_id);
                assert(result_variable);
                result_variable->value = op1_value + op2_value;
            } break;
            case InstructionID::Assign:
            {
                auto op1_id = instruction.operands[0];
                auto op2_id = instruction.operands[1];
                auto result_id = instruction.result;
                s32 op1_value = resolve_index(ir, &runtime, op1_id);
                s32 op2_value = resolve_index(ir, &runtime, op2_id);
                auto* result_variable = get_local_variable(&runtime, result_id);
                assert(result_variable);
                result_variable->value = op2_value;
            } break;
            case InstructionID::Ret:
            {
                auto op1_id = instruction.operands[0];
                s32 op1_value = resolve_index(ir, &runtime, op1_id);
                printf("Function returned %d\n", op1_value);
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }
}