#include "bytecode.h"

#include <RNS/os.h>

#include <stdio.h>

//using namespace Bytecode;
//
//constexpr const static u32 sizes[static_cast<u32>(NativeTypeID::Count)] = {
//    0,
//    sizeof(u8), sizeof(u16), sizeof(u32), sizeof(u64),
//    sizeof(s8), sizeof(s16), sizeof(s32), sizeof(s64),
//    sizeof(f32), sizeof(f64),
//};
//
//static inline Instruction instr(InstructionID id, s64 operand1 = INT64_MAX, s64 operand2 = INT64_MAX, s64 result = INT64_MAX)
//{
//    Instruction i = {
//        .operands = { operand1, operand2 },
//        .result = result,
//        .id = id,
//    };
//
//    return i;
//}
//
//struct ScopedStorage
//{
//    s64 ir_local_variables[100];
//    u32 ast_local_variables[100];
//    s64 local_variable_count;
//};
//
//using AST::Result;
//using AST::Node;
//using AST::NodeType;
//
//s64 transform_node_to_ir(AST::Result* parser_result, IR* ir, u32 node_id, ScopedStorage* scoped_storage)
//{
//    assert(node_id != no_value);
//    Node* node = parser_result->node_buffer.get(node_id);
//    assert(node);
//
//    auto type = node->type;
//
//    switch (type)
//    {
//        case NodeType::IntLit:
//        {
//            // @TODO: change hardcoded value
//            Value int_lit = {};
//            int_lit.type = NativeTypeID::S32;
//            int_lit.op_type = OperandType::IntLit;
//            int_lit.int_lit = node->int_lit;
//            return ir->vb.append(int_lit);
//        } break;
//        //case NodeType::SymName:
//        //{
//        //    const char* var_name = node->sym_name.ptr;
//        //    assert(var_name);
//
//        //    for (s64 i = 0; i < scoped_storage->local_variable_count; i++)
//        //    {
//        //        u32 ast_local = scoped_storage->ast_local_variables[i];
//        //        Node* ast_local_node = m->nb.get(ast_local);
//        //        assert(ast_local_node);
//        //        if (ast_local_node->type == NodeType::VarDecl)
//        //        {
//        //            u32 ast_local_name_id = ast_local_node->var_decl.name;
//        //            Node* ast_local_name_node = m->nb.get(ast_local_name_id);
//        //            assert(ast_local_name_node);
//        //            assert(ast_local_name_node->type == NodeType::SymName);
//        //            const char* local_name = ast_local_name_node->sym_name.ptr;
//        //            assert(local_name);
//
//        //            if (strcmp(local_name, var_name) == 0)
//        //            {
//        //                return scoped_storage->ir_local_variables[i];
//        //            }
//        //        }
//        //    }
//        //    assert(0);
//        //    return {};
//        //} break;
//        case NodeType::BinOp:
//        {
//            Value op_result;
//            op_result.type = NativeTypeID::S32;
//            op_result.op_type = OperandType::New;
//            s64 op_result_id = ir->vb.append(op_result);
//            s64 index = scoped_storage->local_variable_count++;
//            scoped_storage->ir_local_variables[index] = op_result_id;
//            scoped_storage->ast_local_variables[index] = node_id;
//            ir->ib.append(instr(InstructionID::Decl, op_result_id, no_value, op_result_id));
//
//            u32 left_node = node->bin_op.left;
//            u32 right_node = node->bin_op.right;
//            s64 left_value_id = transform_node_to_ir(m, ir, left_node, scoped_storage);
//            s64 right_value_id = transform_node_to_ir(m, ir, right_node, scoped_storage);
//
//            auto op = node->bin_op.op;
//
//            switch (op)
//            {
//                case BinOp::Plus:
//                    ir->ib.append(instr(InstructionID::Add, left_value_id, right_value_id, op_result_id));
//                    break;
//                default:
//                    RNS_NOT_IMPLEMENTED;
//                    break;
//            }
//
//            return op_result_id;
//        } break;
//        case NodeType::VarDecl:
//        {
//            auto type_node_id = node->var_decl.type;
//            Node* type_node = m->nb.get(type_node_id);
//            assert(type_node);
//            assert(type_node->type == NodeType::Type);
//            assert(type_node->type_expr.is_native);
//
//            Value var_value;
//            var_value.type = type_node->type_expr.native;
//            var_value.op_type = OperandType::New;
//            s64 var_operand_id = ir->vb.append(var_value);
//            s64 index = scoped_storage->local_variable_count++;
//            scoped_storage->ir_local_variables[index] = var_operand_id;
//            scoped_storage->ast_local_variables[index] = node_id;
//            ir->ib.append(instr(InstructionID::Decl, var_operand_id, no_value, var_operand_id));
//
//            u32 value_id = node->var_decl.value;
//            s64 expression_id;
//
//            if (value_id != Parser::no_value)
//            {
//                expression_id = transform_node_to_ir(m, ir, value_id, scoped_storage);
//            }
//            else
//            {
//                Value zero_lit = { .type = NativeTypeID::S32, .op_type = OperandType::IntLit };
//                expression_id = ir->vb.append(zero_lit);
//            }
//            ir->ib.append(instr(InstructionID::Assign, var_operand_id, expression_id, var_operand_id));
//            return var_operand_id;
//        } break;
//        case NodeType::Ret:
//        {
//            u32 ret_node_id = node->ret_expr.ret_value;
//            s64 ret_value_id = transform_node_to_ir(m, ir, ret_node_id, scoped_storage);
//            ir->ib.append(instr(InstructionID::Ret, ret_value_id));
//            return ret_value_id;
//        } break;
//        default:
//            RNS_NOT_IMPLEMENTED;
//            return {};
//    }
//}
//
//
//Bytecode::IR generate_ir(AST::Result* parser_result, RNS::Allocator* instruction_allocator, RNS::Allocator* value_allocator)
//{
//    IR ir = {
//        .ib = InstructionBuffer::create(instruction_allocator),
//        .vb = ValueBuffer::create(value_allocator),
//    };
//
//    for (auto i = 0; i < m->tldb.len; i++)
//    {
//        ScopedStorage scoped_storage = {};
//        u32 tld_id = m->tldb.ptr[i];
//        Node* tld_node = m->nb.get(tld_id);
//        assert(tld_node);
//        assert(tld_node->type == NodeType::Function);
//
//        for (auto i = 0; i < tld_node->function_decl.statements.len; i++)
//        {
//            auto st_id = tld_node->function_decl.statements.ptr[i];
//            transform_node_to_ir(m, &ir, st_id, &scoped_storage);
//        }
//    }
//
//    return ir;
//}
//
//Bytecode::PrintBuffer Bytecode::Value::to_string(s64 id)
//{
//    PrintBuffer bf;
//    auto len = 0;
//    len += sprintf(bf.ptr, "ID: %lld %s %s ", id, type_to_string(type), operand_type_to_string(op_type));
//
//    switch (op_type)
//    {
//        case OperandType::Invalid:
//            sprintf(bf.ptr + len, "Invalid");
//            break;
//        case OperandType::IntLit:
//            sprintf(bf.ptr + len, "Int literal %llu", int_lit.lit);
//            break;
//        case OperandType::StackAddress:
//            sprintf(bf.ptr + len, "Variable addressing: %lld", stack.id);
//            break;
//        case OperandType::New:
//            sprintf(bf.ptr + len, "Variable declaration");
//            break;
//        default:
//            RNS_NOT_IMPLEMENTED;
//            break;
//    }
//
//    return bf;
//}
//
//void Bytecode::Instruction::print(ValueBuffer* vb)
//{
//    const char* op1_string = value_id_to_string(vb, operands[0]).ptr;
//    const char* op2_string = value_id_to_string(vb, operands[1]).ptr;
//    const char* result_string = value_id_to_string(vb, result).ptr;
//    printf("Instruction: %s\n\tOperand 1: %s\n\tOperand 2: %s\n\tResult: %s\n\n", id_to_string(), op1_string, op2_string, result_string);
//}
//
//
//
//
//
//

/* Pseudo-WASM bytecode */

namespace WASMBC
{
    using RNS::Allocator;
    using AST::Node;
    using AST::NodeBuffer;
    using AST::FunctionDeclaration;

    const WASM_ID no_value = 0xcccccccc;
    const WASM_ID __stack_pointer = 66560;



    struct ScopeStorage
    {
        AST::NodeRefBuffer node_ref_buffer;
        IDBuffer bc_id_buffer;
        IDBuffer stack_offset;
    };
    // @TODO: encapsulate IR classes into an static context
    struct FunctionEncoder
    {
        NodeBuffer* nb;
        FunctionDeclaration* fn_decl;
        InstructionBuffer* instructions;
        WASM_ID next_id;
        u32 stack_size;
        u32 stack_occupied;
        WASM_ID stack_pointer_id;
        ScopeStorage scope_storage;

        void stack_allocate(u32 size);
        void instr(Instruction instruction, u32 value = no_value);
        WASM_ID process_statement(Node* node, AST::Type* type = nullptr);
        NativeTypeID get_type(u32 type_node_id);
        void scan_through_function();
        void print_instructions(s64 start = 0);
        u32 find_size(NativeTypeID native_type);
        u32 find_size(AST::Type* type);
        void local_get(u32 index);
        u32 local_set();
        void local_set(u32 index);
        void encode(RNS::Allocator* allocator, NodeBuffer* node_buffer, InstructionBuffer* instructions, FunctionDeclaration* fn_decl);
    };

#define Literal(_lit_value_) { .type = OperandType::Literal, .lit = static_cast<u64>(_lit_value_) }
#define StackPointer() { .type = OperandType::StackPointer }

    void FunctionEncoder::stack_allocate(u32 size)
    {
        assert(size == sizeof(s32));
        stack_occupied += size;
        scope_storage.stack_offset.append(stack_occupied);
        instr(Instruction::i32_store, stack_size - stack_occupied);
    }

    void FunctionEncoder::instr(Instruction instruction, u32 value)
    {
        auto instruction_checkpoint = instructions->len;
        auto* instr = instructions->append({ instruction, value });
        //instr->print();
        //print_instructions(instruction_checkpoint);
    }

    u32 FunctionEncoder::find_size(AST::Type* type)
    {
        auto type_kind = type->kind;
        switch (type_kind)
        {
            case AST::TypeKind::Native:
                return find_size(type->native_t);
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
        return 0;
    }

    WASM_ID FunctionEncoder::process_statement(Node* node, AST::Type* type)
    {
        assert(node);

        auto node_type = node->type;

        switch (node_type)
        {
            case AST::NodeType::VarDecl:
            {
                auto instruction_checkpoint = instructions->len;
                auto* var_type = node->var_decl.type;
                assert(var_type);
                assert(var_type->kind == AST::TypeKind::Native);
                assert(var_type->native_t == NativeTypeID::S32);
                auto var_size = find_size(var_type);

                auto var_value_node = node->var_decl.value;
                assert(var_value_node);
                auto var_id = process_statement(var_value_node, var_type);
                local_get(stack_pointer_id);
                local_get(var_id);
                scope_storage.bc_id_buffer.append(var_id);
                scope_storage.node_ref_buffer.append(node);
                stack_allocate(var_size);
            } break;
            case AST::NodeType::BinOp:
            {
                auto binop = node->bin_op.op;
                auto left_node = node->bin_op.left;
                auto right_node = node->bin_op.right;

                auto left_id = process_statement(left_node, type);
                auto right_id = process_statement(right_node, type);

                instr(Instruction::local_get, left_id);
                instr(Instruction::local_get, right_id);

                switch (binop)
                {
                    case Lexer::BinOp::Plus:
                    {
                        instr(Instruction::i32_add);
                        auto sum_id = local_set();
                        return sum_id;
                    }
                    case Lexer::BinOp::Minus:
                    {
                        instr(Instruction::i32_sub);
                        auto sub_id = local_set();
                        return sub_id;
                    }
                    case Lexer::BinOp::None:
                        RNS_UNREACHABLE;
                        break;
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }
            } break;
            case AST::NodeType::IntLit:
            {
                auto lit = node->int_lit.lit;
                assert(type->kind == AST::TypeKind::Native);

                switch (type->native_t)
                {
                    case Lexer::NativeTypeID::S32:
                    {
                        assert((s64)lit >= INT32_MIN);
                        assert(lit <= INT32_MAX);
                        instr(Instruction::i32_const, static_cast<u32>(lit));
                        auto int_lit_id = local_set();
                        return int_lit_id;
                    }
                    case Lexer::NativeTypeID::Count:
                    case Lexer::NativeTypeID::None:
                        RNS_UNREACHABLE;
                        break;
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }
            } break;
            case AST::NodeType::Ret:
            {
                auto* ret_expr_node = node->ret.expr;
                auto ret_expr_id = process_statement(ret_expr_node);
                local_get(ret_expr_id);
                instr(Instruction::ret);
            } break;
            case AST::NodeType::VarExpr:
            {
                auto* var_decl_node = node->var_expr.mentioned;
                assert(var_decl_node);

                u32 i = 0;

                for (auto* node : scope_storage.node_ref_buffer)
                {
                    if (node == var_decl_node)
                    {
                        local_get(stack_pointer_id);
                        instr(Instruction::i32_load, stack_size - scope_storage.stack_offset.ptr[i]);
                        auto load = local_set();
                        return load;
                    }

                    i++;
                }
                RNS_UNREACHABLE;
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return no_value;
    }

    u32 FunctionEncoder::find_size(NativeTypeID native_type)
    {
        switch (native_type)
        {
            case Lexer::NativeTypeID::None:
                RNS_UNREACHABLE;
            case Lexer::NativeTypeID::U8:
                return sizeof(u8);
            case Lexer::NativeTypeID::U16:
                return sizeof(u16);
            case Lexer::NativeTypeID::U32:
                return sizeof(u32);
            case Lexer::NativeTypeID::U64:
                return sizeof(u64);
            case Lexer::NativeTypeID::S8:
                return sizeof(s8);
            case Lexer::NativeTypeID::S16:
                return sizeof(s16);
            case Lexer::NativeTypeID::S32:
                return sizeof(s32);
            case Lexer::NativeTypeID::S64:
                return sizeof(s64);
            case Lexer::NativeTypeID::F32:
                return sizeof(f32);
            case Lexer::NativeTypeID::F64:
                return sizeof(f64);
            case Lexer::NativeTypeID::Count:
                RNS_UNREACHABLE;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return 0;
    }

    // @Info: we need to scan in advance the function to see how many stack allocations there are
    void FunctionEncoder::scan_through_function()
    {
        assert(fn_decl);

        auto arg_count = static_cast<u32>(fn_decl->arg_count);
        next_id += arg_count;
        for (u32 i = 0; i < arg_count; i++)
        {
            RNS_NOT_IMPLEMENTED;
        }

        for (auto* st_node : fn_decl->scope.statements)
        {
            if (st_node->type == AST::NodeType::VarDecl)
            {
                auto* type = st_node->var_decl.type;
                auto var_size = find_size(type);
                stack_size += var_size;
            }
        }
    }

    void FunctionEncoder::local_get(u32 index)
    {
        instr(Instruction::local_get, index);
    }
    void FunctionEncoder::local_set(u32 index)
    {
        instr(Instruction::local_set, index);
    }

    u32 FunctionEncoder::local_set()
    {
        auto id = next_id;
        local_set(id);
        next_id++;
        return id;
    }

    void FunctionEncoder::encode(RNS::Allocator* allocator, NodeBuffer* node_buffer, InstructionBuffer* instructions, FunctionDeclaration* fn_decl)
    {
        this->nb = node_buffer;
        this->fn_decl = fn_decl;
        this->instructions = instructions;
        this->next_id = 0;
        this->stack_size = 0;
        this->stack_occupied = 0;
        this->scope_storage.node_ref_buffer = AST::NodeRefBuffer::create(allocator, 1024),
            this->scope_storage.bc_id_buffer = IDBuffer::create(allocator, 1024);
        this->scope_storage.stack_offset = IDBuffer::create(allocator, 1024);

        scan_through_function();

        bool stack_operations = stack_size > 0;
        if (stack_operations)
        {
            stack_size = static_cast<u32>(RNS::align(stack_size, 16));
            instr(Instruction::global_get, __stack_pointer);
            local_set();
            instr(Instruction::i32_const, stack_size);
            local_set();
            local_get(next_id - 2);
            local_get(next_id - 1);
            instr(Instruction::i32_sub);
            stack_pointer_id = local_set();
        }

        for (auto* st_node : fn_decl->scope.statements)
        {
            process_statement(st_node);
        }
    }

    void FunctionEncoder::print_instructions(s64 start)
    {
        for (auto i = start; i < instructions->len; i++)
        {
            auto& instr = instructions->ptr[i];
            //instr.print();
        }
    }

    Result encode(AST::Result* parser_result, RNS::Allocator* allocator)
    {
        auto& fn_declarations = parser_result->function_declarations;
        InstructionBuffer instructions = InstructionBuffer::create(allocator, 1024);
        FunctionEncoder encoder;

        for (auto& fn_decl : parser_result->function_declarations)
        {
            encoder.encode(allocator, &parser_result->node_buffer, &instructions, &fn_decl);
        }

        return { instructions, encoder.stack_pointer_id };
    }

    void InstructionStruct::print()
    {
        printf("%s", id_to_string());
        if (value != no_value)
        {
            if (value == __stack_pointer)
            {
                printf(" __stack_pointer");
            }
            else
            {
                printf(" %u", value);
            }
        }
        printf("\n");
    }

    const char* InstructionStruct::id_to_string()
    {
        switch (id)
        {
#define rns_instr_case_to_str(_instr_) case WASMBC::Instruction:: ## _instr_ : return #_instr_;
            rns_instr_case_to_str(unreachable);
            rns_instr_case_to_str(nop);
            rns_instr_case_to_str(ret);
            rns_instr_case_to_str(call);
            rns_instr_case_to_str(call_indirect);
            rns_instr_case_to_str(local_get);
            rns_instr_case_to_str(local_set);
            rns_instr_case_to_str(local_tee);
            rns_instr_case_to_str(global_get);
            rns_instr_case_to_str(global_set);
            rns_instr_case_to_str(i32_load);
            rns_instr_case_to_str(i32_store);
            rns_instr_case_to_str(i32_const);
            rns_instr_case_to_str(i32_eqz);
            rns_instr_case_to_str(i32_eq);
            rns_instr_case_to_str(i32_ne);
            rns_instr_case_to_str(i32_lt_s);
            rns_instr_case_to_str(i32_lt_u);
            rns_instr_case_to_str(i32_gt_s);
            rns_instr_case_to_str(i32_gt_u);
            rns_instr_case_to_str(i32_le_s);
            rns_instr_case_to_str(i32_le_u);
            rns_instr_case_to_str(i32_ge_s);
            rns_instr_case_to_str(i32_ge_u);
            rns_instr_case_to_str(i32_clz);
            rns_instr_case_to_str(i32_ctz);
            rns_instr_case_to_str(i32_popcnt);
            rns_instr_case_to_str(i32_add);
            rns_instr_case_to_str(i32_sub);
            rns_instr_case_to_str(i32_mul);
            rns_instr_case_to_str(i32_div_s);
            rns_instr_case_to_str(i32_div_u);
            rns_instr_case_to_str(i32_rem_s);
            rns_instr_case_to_str(i32_rem_u);
            rns_instr_case_to_str(i32_and);
            rns_instr_case_to_str(i32_or);
            rns_instr_case_to_str(i32_xor);
            rns_instr_case_to_str(i32_shl);
            rns_instr_case_to_str(i32_shr_s);
            rns_instr_case_to_str(i32_shr_u);
            rns_instr_case_to_str(i32_rotl);
            rns_instr_case_to_str(i32_rotr);
            rns_instr_case_to_str(i32_wrap_i64);
            default:
                RNS_NOT_IMPLEMENTED;
                return nullptr;
#undef rns_instr_case_to_str
        }
    }
}

#if 0
namespace LLVMIR
{
    using ID = u32;
    const u32 no_value = 0xcccccccc;
    enum class Instruction
    {

    };
    enum class Type
    {

    };
    
    enum class OperandType
    {
        Literal,
    };
    struct Operand
    {
        OperandType operand_type;
        union
        {
            u32 lit;
        };
    };

    struct InstructionStruct
    {
        ID index;
        Instruction id;
        Type type;
        // @TODO: we can omit alignment for now
    };

    using namespace RNS;
    using InstructionBuffer = Buffer<InstructionStruct>;

    struct Encoder
    {
        Parser* parser;
        InstructionBuffer* instructions;
        u32 alloca_count;

        void process_node_id(u32 node_id)
        {
            auto* node = parser->nb.get(node_id);
            auto node_type = node->type;

            switch (node_type)
            {
                case NodeType::VarDecl:
                {

                }
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }
        }

        void encode(Allocator* allocator, Parser* parser, InstructionBuffer* instructions, Node* node)
        {
            assert(node);
            assert(node->type == NodeType::Function);

            this->parser = parser;
            this->instructions = instructions;

            alloca_count = 0;

            for (auto node_id : node->function_decl.statements)
            {
                auto* node = parser->nb.get(node_id);
                auto node_type = node->type;

                if (node_type == NodeType::VarDecl)
                {
                    auto var_type_node_id = node->var_decl.type;
                    auto* var_type = parser->nb.get(var_type_node_id);
                }
            }

            for (auto node_id : node->function_decl.statements)
            {
                process_node_id(node_id);
            }
        }
    };

    void encode(Parser* module_parser, Allocator* allocator)
    {
        assert(module_parser->tldb.len == 1);
        InstructionBuffer instructions = InstructionBuffer::create(allocator, 1024);
        Encoder encoder;
        for (auto i = 0; i < module_parser->tldb.len; i++)
        {
            u32 tld_id = module_parser->tldb.ptr[i];
            Node* tld_node = module_parser->nb.get(tld_id);
            assert(tld_node);
            assert(tld_node->type == NodeType::Function);
            encoder.encode(allocator, module_parser, &instructions, tld_node);
        }

        //return { instructions, encoder };
    }
}
#endif
