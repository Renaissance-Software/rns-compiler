#include "bytecode.h"

#include <RNS/os.h>

#include <stdio.h>

using namespace Bytecode;

constexpr const static u32 sizes[static_cast<u32>(NativeTypeID::Count)] = {
    0,
    sizeof(u8), sizeof(u16), sizeof(u32), sizeof(u64),
    sizeof(s8), sizeof(s16), sizeof(s32), sizeof(s64),
    sizeof(f32), sizeof(f64),
};

struct IRFunction
{

};

static inline Instruction instr(InstructionID id, s64 operand1 = INT64_MAX, s64 operand2 = INT64_MAX, s64 result = INT64_MAX)
{
    Instruction i = {
        .operands = { operand1, operand2 },
        .result = result,
        .id = id,
    };

    return i;
}

struct ScopedStorage
{
    s64 ir_local_variables[100];
    u32 ast_local_variables[100];
    s64 local_variable_count;
};

using Parser::ModuleParser;
using Parser::Node;
using Parser::NodeType;

s64 transform_node_to_ir(Parser::ModuleParser* m, IR* ir, u32 node_id, ScopedStorage* scoped_storage)
{
    assert(node_id != no_value);
    Node* node = m->nb.get(node_id);
    assert(node);

    auto type = node->type;

    switch (type)
    {
        case NodeType::IntLit:
        {
            // @TODO: change hardcoded value
            Value int_lit = {};
            int_lit.type = NativeTypeID::S32;
            int_lit.op_type = OperandType::IntLit;
            int_lit.int_lit = node->int_lit;
            return ir->vb.append(int_lit);
        } break;
        case NodeType::SymName:
        {
            const char* var_name = node->sym_name.ptr;
            assert(var_name);

            for (s64 i = 0; i < scoped_storage->local_variable_count; i++)
            {
                u32 ast_local = scoped_storage->ast_local_variables[i];
                Node* ast_local_node = m->nb.get(ast_local);
                assert(ast_local_node);
                if (ast_local_node->type == NodeType::VarDecl)
                {
                    u32 ast_local_name_id = ast_local_node->var_decl.name;
                    Node* ast_local_name_node = m->nb.get(ast_local_name_id);
                    assert(ast_local_name_node);
                    assert(ast_local_name_node->type == NodeType::SymName);
                    const char* local_name = ast_local_name_node->sym_name.ptr;
                    assert(local_name);

                    if (strcmp(local_name, var_name) == 0)
                    {
                        return scoped_storage->ir_local_variables[i];
                    }
                }
            }
            assert(0);
            return {};
        } break;
        case NodeType::BinOp:
        {
            Value op_result;
            op_result.type = NativeTypeID::S32;
            op_result.op_type = OperandType::New;
            s64 op_result_id = ir->vb.append(op_result);
            s64 index = scoped_storage->local_variable_count++;
            scoped_storage->ir_local_variables[index] = op_result_id;
            scoped_storage->ast_local_variables[index] = node_id;
            ir->ib.append(instr(InstructionID::Decl, op_result_id, no_value, op_result_id));

            u32 left_node_id = node->bin_op.left;
            u32 right_node_id = node->bin_op.right;
            s64 left_value_id = transform_node_to_ir(m, ir, left_node_id, scoped_storage);
            s64 right_value_id = transform_node_to_ir(m, ir, right_node_id, scoped_storage);

            auto op = node->bin_op.op;

            switch (op)
            {
                case BinOp::Plus:
                    ir->ib.append(instr(InstructionID::Add, left_value_id, right_value_id, op_result_id));
                    break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            return op_result_id;
        } break;
        case NodeType::VarDecl:
        {
            auto type_node_id = node->var_decl.type;
            Node* type_node = m->nb.get(type_node_id);
            assert(type_node);
            assert(type_node->type == NodeType::Type);
            assert(type_node->type_expr.is_native);

            Value var_value;
            var_value.type = type_node->type_expr.native;
            var_value.op_type = OperandType::New;
            s64 var_operand_id = ir->vb.append(var_value);
            s64 index = scoped_storage->local_variable_count++;
            scoped_storage->ir_local_variables[index] = var_operand_id;
            scoped_storage->ast_local_variables[index] = node_id;
            ir->ib.append(instr(InstructionID::Decl, var_operand_id, no_value, var_operand_id));

            u32 value_id = node->var_decl.value;
            s64 expression_id;

            if (value_id != Parser::no_value)
            {
                expression_id = transform_node_to_ir(m, ir, value_id, scoped_storage);
            }
            else
            {
                Value zero_lit = { .type = NativeTypeID::S32, .op_type = OperandType::IntLit };
                expression_id = ir->vb.append(zero_lit);
            }
            ir->ib.append(instr(InstructionID::Assign, var_operand_id, expression_id, var_operand_id));
            return var_operand_id;
        } break;
        case NodeType::Ret:
        {
            u32 ret_node_id = node->ret_expr.ret_value;
            s64 ret_value_id = transform_node_to_ir(m, ir, ret_node_id, scoped_storage);
            ir->ib.append(instr(InstructionID::Ret, ret_value_id));
            return ret_value_id;
        } break;
        default:
            RNS_NOT_IMPLEMENTED;
            return {};
    }
}


IR generate_ir(ModuleParser* m, RNS::Allocator* instruction_allocator, RNS::Allocator* value_allocator)
{
    IR ir = {
        .ib = InstructionBuffer::create(instruction_allocator),
        .vb = ValueBuffer::create(value_allocator),
    };

    for (auto i = 0; i < m->tldb.len; i++)
    {
        ScopedStorage scoped_storage = {};
        u32 tld_id = m->tldb.ptr[i];
        Node* tld_node = m->nb.get(tld_id);
        assert(tld_node);
        assert(tld_node->type == NodeType::Function);

        for (auto i = 0; i < tld_node->function_decl.statements.len; i++)
        {
            auto st_id = tld_node->function_decl.statements.ptr[i];
            transform_node_to_ir(m, &ir, st_id, &scoped_storage);
        }
    }

    return ir;
}

Bytecode::PrintBuffer Bytecode::Value::to_string(s64 id)
{
    PrintBuffer bf;
    auto len = 0;
    len += sprintf(bf.ptr, "ID: %lld %s %s ", id, type_to_string(type), operand_type_to_string(op_type));

    switch (op_type)
    {
        case OperandType::Invalid:
            sprintf(bf.ptr + len, "Invalid");
            break;
        case OperandType::IntLit:
            sprintf(bf.ptr + len, "Int literal %llu", int_lit.lit);
            break;
        case OperandType::StackAddress:
            sprintf(bf.ptr + len, "Variable addressing: %lld", stack.id);
            break;
        case OperandType::New:
            sprintf(bf.ptr + len, "Variable declaration");
            break;
        default:
            RNS_NOT_IMPLEMENTED;
            break;
    }

    return bf;
}

void Bytecode::Instruction::print(ValueBuffer* vb)
{
    const char* op1_string = value_id_to_string(vb, operands[0]).ptr;
    const char* op2_string = value_id_to_string(vb, operands[1]).ptr;
    const char* result_string = value_id_to_string(vb, result).ptr;
    printf("Instruction: %s\n\tOperand 1: %s\n\tOperand 2: %s\n\tResult: %s\n\n", id_to_string(), op1_string, op2_string, result_string);
}
