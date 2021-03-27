#include "llvm_bytecode.h"

#include <llvm/Bitcode/LLVMBitCodes.h>
#include "typechecker.h"

namespace LLVM
{
    using namespace RNS;
    using LLVMID = u32;

    enum class Instruction : u8
    {
        // Terminator
        Ret = 1,
        Br = 2,
        Switch_ = 3,
        Indirectbr = 4,
        Invoke = 5,
        Resume = 6,
        Unreachable = 7,
        Cleanup_ret = 8,
        Catch_ret = 9,
        Catch_switch = 10,
        Call_br = 11,

        // Unary
        Fneg = 12,

        // Binary
        Add = 13,
        Fadd = 14,
        Sub = 15,
        Fsub = 16,
        Mul = 17,
        Fmul = 18,
        Udiv = 19,
        Sdiv = 20,
        Fdiv = 21,
        Urem = 22,
        Srem = 23,
        Frem = 24,

        // Logical
        Shl = 25,
        Lshr = 26,
        Ashr = 27,
        And = 28,
        Or = 29,
        Xor = 30,

        // Memory
        Alloca = 31,
        Load = 32,
        Store = 33,
        GetElementPtr = 34,
        Fence = 35,
        AtomicCmpXchg = 36,
        AtomicRMW = 37,

        // Cast
        Trunc = 38,
        ZExt = 39,
        SExt = 40,
        FPToUI = 41,
        FPToSI = 42,
        UIToFP = 43,
        SIToFP = 44,
        FPTrunc = 45,
        FPExt = 46,
        PtrToInt = 47,
        IntToPtr = 48,
        BitCast = 49,
        AddrSpaceCast = 50,

        // FuncLetPad
        CleanupPad = 51,
        CatchPad = 52,

        // Other
        ICmp = 53,
        FCmp = 54,
        Phi = 55,
        Call = 56,
        Select = 57,
        UserOp1 = 58,
        UserOp2 = 59,
        VAArg = 60,
        ExtractElement = 61,
        InsertElement = 62,
        ShuffleVector = 63,
        ExtractValue = 64,
        InsertValue = 65,
        LandingPad = 66,
        Freeze = 67,
    };


    enum class GlobalValue : u8
    {
        Function,
        GlobalAlias,
        GlobalIFunc,
        GlobalVariable,
    };

    enum class ConstantID : u8
    {
        BlockAddress,
        Expression,
        Array,
        Struct,
        Vector,
        Undefined,
        AggregateZero,
        DataArray,
        DataVector,
        Int,
        FP,
        PointerNull,
        TokenNone,
    };

    struct Constant
    {
        ConstantID id;
        union
        {
            ConstantInt integer;
        };
    };

    enum class MemoryID
    {
        Use,
        Def,
        Phi,
    };

    struct Value;

    struct Ret
    {
        Type* type;
        Value* value;
    };

    // @Info: these do not form part of the instruction buffer since they need to be allocated at the top of the function
    struct Alloca
    {
        Type* type;
        LLVMID index;
        u32 count;
        u32 alignment;
    };

    struct InstructionInfo;
    struct Store
    {
        Type* type;
        Value* value_to_be_stored;
        InstructionInfo* alloca_i;
        u32 alignment;
    };

    enum class ValueID : u8
    {
        Memory,
        Global,
        Constant,
        Instruction,
        Metadata,
        InlineASM,
        Argument,
        BasicBlock,
    };

    struct InstructionInfo;
    const char* instruction_to_string_for_value(InstructionInfo* instruction);

    struct Value
    {
        ValueID id;
        union
        {
            InstructionInfo* instruction;
            Constant constant;
        };

        const char* to_string()
        {
            switch (id)
            {
                case ValueID::Constant:
                {
                    switch (constant.id)
                    {
                        case ConstantID::Int:
                        {
                            return "int_lit";
                        } break;
                        default:
                        {
                            RNS_NOT_IMPLEMENTED;
                        } break;
                    }
                } break;
                case ValueID::Instruction:
                {
                    return instruction_to_string_for_value(instruction);
                }
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            return nullptr;
        }
    };

    const char* type_to_string(Type* type)
    {
        switch (type->id)
        {
            case TypeID::IntegerType:
            {
                switch (type->integer_t.bits)
                {
                    case 32:
                    {
                        return "i32";
                    }
                    default:
                    {
                        RNS_NOT_IMPLEMENTED;
                    } break;
                }
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
        return nullptr;
    }

    struct InstructionInfo
    {
        Instruction id;
        union
        {
            Ret ret;
            Alloca alloca_i;
            Store store;
        };

        void print()
        {
            switch (id)
            {
                case Instruction::Store:
                {
                    //printf("store %s %s, %s* %%%u, align %u\n", type_to_string(store.type), store.value_to_be_stored->to_string(), type_to_string(0), store.alloca_i.index, store.alignment);
                } break;
                case Instruction::Alloca:
                {
                    printf("%%%u = alloca %s, align %u\n", alloca_i.index, type_to_string(alloca_i.type), alloca_i.alignment);
                } break;
                case Instruction::Ret:
                {
                    printf("ret %s ", type_to_string(ret.type));
                    switch (ret.value->id)
                    {
                        case ValueID::Instruction:
                        {
                            if (ret.value->instruction->id == Instruction::Alloca)
                            {
                                printf("%%%u\n", ret.value->instruction->alloca_i.index);
                            }
                            else
                            {
                                RNS_NOT_IMPLEMENTED;
                            }
                        } break;
                        default:
                            RNS_NOT_IMPLEMENTED;
                            break;
                    }
                } break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }
        }


        const char* id_to_string()
        {
            switch (id)
            {
#define rns_instr_id_to_string(_instr_) case Instruction:: ## _instr_: return #_instr_
                rns_instr_id_to_string(Ret);
                rns_instr_id_to_string(Br);
                rns_instr_id_to_string(Switch_);
                rns_instr_id_to_string(Indirectbr);
                rns_instr_id_to_string(Invoke);
                rns_instr_id_to_string(Resume);
                rns_instr_id_to_string(Unreachable);
                rns_instr_id_to_string(Cleanup_ret);
                rns_instr_id_to_string(Catch_ret);
                rns_instr_id_to_string(Catch_switch);
                rns_instr_id_to_string(Call_br);
                rns_instr_id_to_string(Fneg);
                rns_instr_id_to_string(Add);
                rns_instr_id_to_string(Fadd);
                rns_instr_id_to_string(Sub);
                rns_instr_id_to_string(Fsub);
                rns_instr_id_to_string(Mul);
                rns_instr_id_to_string(Fmul);
                rns_instr_id_to_string(Udiv);
                rns_instr_id_to_string(Sdiv);
                rns_instr_id_to_string(Fdiv);
                rns_instr_id_to_string(Urem);
                rns_instr_id_to_string(Srem);
                rns_instr_id_to_string(Frem);
                rns_instr_id_to_string(Shl);
                rns_instr_id_to_string(Lshr);
                rns_instr_id_to_string(Ashr);
                rns_instr_id_to_string(And);
                rns_instr_id_to_string(Or);
                rns_instr_id_to_string(Xor);
                rns_instr_id_to_string(Alloca);
                rns_instr_id_to_string(Load);
                rns_instr_id_to_string(Store);
                rns_instr_id_to_string(GetElementPtr);
                rns_instr_id_to_string(Fence);
                rns_instr_id_to_string(AtomicCmpXchg);
                rns_instr_id_to_string(AtomicRMW);
                rns_instr_id_to_string(Trunc);
                rns_instr_id_to_string(ZExt);
                rns_instr_id_to_string(SExt);
                rns_instr_id_to_string(FPToUI);
                rns_instr_id_to_string(FPToSI);
                rns_instr_id_to_string(UIToFP);
                rns_instr_id_to_string(SIToFP);
                rns_instr_id_to_string(FPTrunc);
                rns_instr_id_to_string(FPExt);
                rns_instr_id_to_string(PtrToInt);
                rns_instr_id_to_string(IntToPtr);
                rns_instr_id_to_string(BitCast);
                rns_instr_id_to_string(AddrSpaceCast);
                rns_instr_id_to_string(CleanupPad);
                rns_instr_id_to_string(CatchPad);
                rns_instr_id_to_string(ICmp);
                rns_instr_id_to_string(FCmp);
                rns_instr_id_to_string(Phi);
                rns_instr_id_to_string(Call);
                rns_instr_id_to_string(Select);
                rns_instr_id_to_string(UserOp1);
                rns_instr_id_to_string(UserOp2);
                rns_instr_id_to_string(VAArg);
                rns_instr_id_to_string(ExtractElement);
                rns_instr_id_to_string(InsertElement);
                rns_instr_id_to_string(ShuffleVector);
                rns_instr_id_to_string(ExtractValue);
                rns_instr_id_to_string(InsertValue);
                rns_instr_id_to_string(LandingPad);
                rns_instr_id_to_string(Freeze);
#undef rns_instr_id_to_string
            }

            return nullptr;
        }
    };

    const char* instruction_to_string_for_value(InstructionInfo* instruction)
    {
        if (instruction->id == Instruction::Alloca)
        {
            return "%alloca";
        }
        else
        {
            RNS_NOT_IMPLEMENTED;
        }
        return nullptr;
    }


    struct BasicBlock
    {
        Buffer<InstructionInfo> instructions;
    };

    struct Function
    {
        Buffer<BasicBlock> basic_blocks;
        u32 alloca_count;
        u32 next_alloca_index;
        u32 next_non_alloca_index;
    };

    struct IRBuilder
    {
        BasicBlock* current;
        Function* current_fn;
        Buffer<InstructionInfo*>* current_alloca_buffer;
        Buffer<InstructionInfo*>* current_instruction_ref;

        InstructionInfo* append(InstructionInfo instruction)
        {
            auto* instr = current->instructions.append(instruction);
            instr->print();
            return instr;
        }

        void create_alloca(Node* node)
        {
            assert(node->type == NodeType::VarDecl);
            assert(node->var_decl.type);
            Alloca alloca_i = {
                .type = node->var_decl.type,
                .index = current_fn->next_alloca_index++,
                .count = 1,
                .alignment = 0xcc,
            };
            auto* alloca_i_ptr = append({
                .id = Instruction::Alloca,
                .alloca_i = alloca_i,
                });
            current_alloca_buffer->append(alloca_i_ptr);
            current_fn->alloca_count++;
        }
    };

    bool typecheck(Type* type, LLVM::Value* value)
    {
        switch (value->id)
        {
            case ValueID::Constant:
            {
                switch (value->constant.id)
                {
                    case ConstantID::Int:
                    {
                        if (type->id != TypeID::IntegerType)
                        {
                            return false;
                        }
                        if (value->constant.integer.is_signed && !type->integer_t.is_signed)
                        {
                            return false;
                        }
                        // @TODO: we can check integer type limits here
                    } break;
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return true;
    }

    using TypecheckingFunction = bool(Type*, LLVM::Value*);
    Value* node_to_bytecode_value(Allocator* allocator, IRBuilder& ir_builder, TypeBuffer& type_declarations, FunctionDeclaration& current_function, Node* node, TypecheckingFunction* typechecking_fn = nullptr, Type* type_to_typecheck = nullptr)
    {
        switch (node->type)
        {
            case NodeType::IntLit:
            {
                Value* value = new(allocator) Value;
                assert(value);
                *value = {
                    .id = ValueID::Constant,
                    .constant = {
                        .id = ConstantID::Int,
                        .integer = node->int_lit,
                    }
                };
                if (typechecking_fn)
                { 
                    assert(type_to_typecheck);
                    assert(typechecking_fn(type_to_typecheck, value));
                }
                return value;
            } break;
            case NodeType::VarExpr:
            {
                auto* alloca_elem = (*ir_builder.current_alloca_buffer)[current_function.variables.get_id_if_ref_buffer(node->var_expr.mentioned)];
                assert(alloca_elem);
                Value* value = new(allocator) Value;
                assert(value);
                *value = {
                    .id = ValueID::Instruction,
                    .instruction = alloca_elem,
                };
                return value;
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return nullptr;
    }

    void do_statement_node(Allocator* allocator, IRBuilder& ir_builder, LLVM::Function& llvm_function, TypeBuffer& type_declarations, FunctionDeclaration& current_function, Node* node)
    {
        InstructionInfo instruction;

        switch (node->type)
        {
            case NodeType::VarDecl:
            {
                if (node->var_decl.value)
                {
                    auto* value = node_to_bytecode_value(allocator, ir_builder, type_declarations, current_function, node->var_decl.value, typecheck, node->var_decl.type);
                    assert(value);
                    InstructionInfo instruction;
                    instruction.id = Instruction::Store;
                    instruction.store.type = node->var_decl.type;
                    instruction.store.value_to_be_stored = value;
                    instruction.store.alloca_i = (*ir_builder.current_alloca_buffer)[current_function.variables.get_id_if_ref_buffer(node)];
                    assert(instruction.store.alloca_i);

                    instruction.store.alignment = 0xcc;


                    ir_builder.append(instruction);
                }
            } break;
            case NodeType::Ret:
            {
                auto* ret_expr = node->ret.expr;
                auto* fn_type = current_function.type;
                assert(fn_type);
                auto* ret_type = fn_type->ret_type;
                assert(ret_type);
                instruction.id = Instruction::Ret;
                if (ret_expr)
                {
                    auto* ret_value = node_to_bytecode_value(allocator, ir_builder, type_declarations, current_function, ret_expr, typecheck, current_function.type->ret_type);
                    assert(ret_value);
                    instruction.ret = {
                        .type = current_function.type->ret_type,
                        .value = ret_value,
                    };
                    ir_builder.append(instruction);
                }
                else
                {
                    assert(ret_type->id == TypeID::VoidType);
                }
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    void encode(Compiler& compiler, NodeBuffer& node_buffer, FunctionTypeBuffer& function_type_declarations, TypeBuffer& type_declarations, FunctionDeclarationBuffer& function_declarations)
    {
        compiler.subsystem = Compiler::Subsystem::IR;
        auto llvm_allocator = create_suballocator(&compiler.page_allocator, RNS_MEGABYTE(50));
        assert(function_declarations.len == 1);
        IRBuilder ir_builder = {};

        for (auto& function : function_declarations)
        {
            Function llvm_function = {
                .basic_blocks = Buffer<BasicBlock>::create(&llvm_allocator, 8),
            };
            Buffer<InstructionInfo*> alloca_buffer = Buffer<InstructionInfo*>::create(&llvm_allocator, function.variables.len);

            auto arg_count = 0;
            for (auto* var_node : function.variables)
            {
                if (var_node->var_decl.is_fn_arg)
                {
                    arg_count++;
                    continue;
                }
                break;
            }

            llvm_function.alloca_count = arg_count;
            llvm_function.next_alloca_index = arg_count + 1;

            ir_builder.current = llvm_function.basic_blocks.allocate();
            ir_builder.current->instructions = ir_builder.current->instructions.create(&llvm_allocator, 1024);
            ir_builder.current_fn = &llvm_function;
            ir_builder.current_alloca_buffer = &alloca_buffer;

            for (auto* var_node : function.variables)
            {
                assert(var_node->type == NodeType::VarDecl);

                ir_builder.create_alloca(var_node);
            }

            for (auto* st_node : function.scope_blocks[0].statements)
            {
                do_statement_node(&llvm_allocator, ir_builder, llvm_function, type_declarations, function, st_node);
            }
        }
    }
}