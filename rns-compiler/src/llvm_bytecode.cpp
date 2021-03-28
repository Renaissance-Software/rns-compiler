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

    struct Symbol
    {
        enum class ID
        {
            Constant,
            Index,
            Label,
        };

        ID type;
        union
        {
            Constant constant;
            LLVMID index;
        } result;

        void to_string(char* buffer)
        {
            switch (type)
            {
                case Symbol::ID::Constant:
                {
                    if (result.constant.integer.is_signed)
                    {
                        sprintf(buffer, "-%llu", result.constant.integer.lit);
                    }
                    else
                    {
                        sprintf(buffer, "%llu", result.constant.integer.lit);
                    }
                } break;
                case Symbol::ID::Index:
                {
                    sprintf(buffer, "%%%u", result.index);
                } break;
                case Symbol::ID::Label:
                {
                    sprintf(buffer, "label %%%u", result.index);
                } break;
            }
        }
    };

    struct Ret
    {
        Type* type;
        Symbol symbol;
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
        Symbol symbol;
        InstructionInfo* alloca_i;
        u32 alignment;
    };

    struct Load
    {
        Type* type;
        InstructionInfo* alloca_i;
        LLVMID index;
        u32 alignment;
    };

    struct Add
    {
        Type* type;
        Symbol left, right;
        LLVMID index;
    };

    struct ICmp
    {
        enum class ID : u8
        {
            Equal,
            NotEqual,
        };
        ID id;
        Type* type;
        Symbol left, right;
        LLVMID index;

        const char* id_to_string()
        {
            switch (id)
            {
                case ID::Equal:
                {
                    return "eq";
                }
                case ID::NotEqual:
                {
                    return "ne";
                }
                default:
                    RNS_NOT_IMPLEMENTED;
                    return nullptr;
            }
        }
    };

    struct Br
    {
        Type* type;
        Symbol condition;
        Symbol if_label, else_label;
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
            Load load;
            Add add;
            ICmp icmp;
            Br br;
        };

        void print()
        {
            printf("\t");
            switch (id)
            {
                case Instruction::Store:
                {
                    char value_buffer[64];
                    store.symbol.to_string(value_buffer);
                    assert(value_buffer && *value_buffer);

                    printf("store %s %s, %s* %%%u, align %u\n", type_to_string(store.type), value_buffer, type_to_string(store.alloca_i->alloca_i.type), store.alloca_i->alloca_i.index, store.alignment);
                } break;
                case Instruction::Load:
                {
                    char value_buffer[64];
                    store.symbol.to_string(value_buffer);
                    assert(value_buffer && *value_buffer);

                    printf("%%%u = load %s, %s* %%%u, align %u\n", load.index, type_to_string(load.type), type_to_string(load.alloca_i->alloca_i.type), load.alloca_i->alloca_i.index, load.alignment);

                } break;
                case Instruction::Alloca:
                {
                    printf("%%%u = alloca %s, align %u\n", alloca_i.index, type_to_string(alloca_i.type), alloca_i.alignment);
                } break;
                case Instruction::Ret:
                {
                    char value_buffer[64];
                    ret.symbol.to_string(value_buffer);
                    printf("ret %s %s\n", type_to_string(ret.type), value_buffer);
                } break;
                case Instruction::Add:
                {
                    char left_buffer[64];
                    char right_buffer[64];
                    add.left.to_string(left_buffer);
                    add.right.to_string(right_buffer);
                    printf("%%%u = add %s %s, %s\n", add.index, type_to_string(add.type), left_buffer, right_buffer);
                } break;
                case Instruction::ICmp:
                {
                    char left_buffer[64];
                    char right_buffer[64];
                    icmp.left.to_string(left_buffer);
                    icmp.right.to_string(right_buffer);
                    printf("%%%u = icmp %s %s %s, %s\n", icmp.index, icmp.id_to_string(), type_to_string(icmp.type), left_buffer, right_buffer);
                } break;
                case Instruction::Br:
                {
                    // @Info: this means a conditional branch
                    if (br.type)
                    {
                        char condition_buffer[64];
                        char if_buffer[64];
                        char else_buffer[64];
                        br.condition.to_string(condition_buffer);
                        br.if_label.to_string(if_buffer);
                        br.else_label.to_string(else_buffer);
                        printf("br i1 %s, %s, %s\n", condition_buffer, if_buffer, else_buffer);
                    }
                    // @Info: this means a straight jump
                    else
                    {
                        char if_buffer[64];
                        br.if_label.to_string(if_buffer);
                        printf("br %s\n", if_buffer);

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

    void instruction_to_string_for_value(InstructionInfo* instruction, char* buffer)
    {
        switch (instruction->id)
        {
            case Instruction::Alloca:
            {
                sprintf(buffer, "%%%u", instruction->alloca_i.index);
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    struct BasicBlock
    {
        Buffer<InstructionInfo> instructions;
        BasicBlock* parent;
    };

    struct Function
    {
        BasicBlock entry_block;
        Buffer<LLVMID> basic_block_labels;
        Buffer<BasicBlock> basic_blocks;
        LLVMID next_index;
        u32 arg_count;
        bool conditional_alloca;
    };

    struct IRBuilder
    {
        BasicBlock* current;
        Function* current_fn;
        Buffer<InstructionInfo*>* current_alloca_buffer;
        Buffer<InstructionInfo*>* current_instruction_ref;
        Buffer<Symbol>* current_symbol_buffer;

        InstructionInfo* append(InstructionInfo instruction)
        {
            auto* instr = current->instructions.append(instruction);
            //instr->print();
            return instr;
        }

        void create_alloca(Node* node)
        {
            assert(node->type == NodeType::VarDecl);
            assert(node->var_decl.type);
            Alloca alloca_i = {
                .type = node->var_decl.type,
                .index = current_fn->next_index++,
                .count = 1,
                .alignment = 4,
            };
            auto* alloca_i_ptr = append({
                .id = Instruction::Alloca,
                .alloca_i = alloca_i,
                });
            current_alloca_buffer->append(alloca_i_ptr);
            append({
                .type = Symbol::ID::Index,
                .result = {.index = alloca_i.index }
                });
        }

        InstructionInfo* get_alloca(Node* node, FunctionDeclaration& ast_current_function)
        {
            return (*current_alloca_buffer)[ast_current_function.variables.get_id_if_ref_buffer(node) + current_fn->conditional_alloca];
        }

        InstructionInfo* get_conditional_alloca()
        {
            if (current_fn->conditional_alloca)
            {
                return (*current_alloca_buffer)[0];
            }
            return nullptr;
        }

        void append(Symbol symbol)
        {
            current_symbol_buffer->append(symbol);
        }

        void terminate(BasicBlock* basic_block, InstructionInfo instruction)
        {
            basic_block->instructions.append(instruction);
            for (auto& instr : basic_block->instructions)
            {
                instr.print();
            }
        }

        BasicBlock* append_basic_block(Allocator* allocator)
        {
            current_fn->basic_block_labels.append(current_fn->next_index++);
            auto* new_one = current_fn->basic_blocks.allocate();
            new_one->parent = current;
            current = new_one;
            current->instructions = current->instructions.create(allocator, 64);

            return new_one;
        }

        Symbol get_block_label(BasicBlock* basic_block)
        {
            auto index = current_fn->basic_blocks.get_id(basic_block);
            auto label_index = current_fn->basic_block_labels[index];

            Symbol block_label = {
                .type = Symbol::ID::Label,
                .result = {.index = label_index }
            };
            return block_label;
        }

        void print_block(BasicBlock* basic_block)
        {
            if (basic_block == &this->current_fn->entry_block)
            {
                printf("fn_entry:\n");
            }
            else
            {
                auto block_label = get_block_label(basic_block);
                printf("%u:\n", block_label.result.index);
            }

            for (auto& instr : basic_block->instructions)
            {
                instr.print();
            }
        }
    };

    bool introspect_for_conditional_allocas(ScopeBlock* block)
    {
        for (auto* st_node : block->statements)
        {
            if (st_node->type == NodeType::Ret)
            {
                return true;
            }
            else if (st_node->type == NodeType::Conditional)
            {
                if (introspect_for_conditional_allocas(st_node->conditional.if_block))
                {
                    return true;
                }
                if (introspect_for_conditional_allocas(st_node->conditional.else_block))
                {
                    return true;
                }
            }
        }

        return false;
    }

    // @TODO: Add typechecking again <Type, Symbol>
    Symbol node_to_bytecode_value(Allocator* allocator, IRBuilder& ir_builder, TypeBuffer& type_declarations, FunctionDeclaration& current_function, Node* node, Type* expected_type = nullptr)
    {
        switch (node->type)
        {
            case NodeType::IntLit:
            {
                Symbol result = {
                    .type = Symbol::ID::Constant,
                    .result = {
                        .constant = {
                            .id = ConstantID::Int,
                            .integer = node->int_lit,
                        }
                    }
                };
                return result;
            } break;
            // @TODO: this should generate a load
            case NodeType::VarExpr:
            {
                auto* alloca_elem = ir_builder.get_alloca(node->var_expr.mentioned, current_function);
                assert(alloca_elem);

                InstructionInfo load_instruction = {
                    .id = Instruction::Load,
                    .load = {
                        .type = alloca_elem->alloca_i.type,
                        .alloca_i = alloca_elem,
                        .index = ir_builder.current_fn->next_index++,
                        .alignment = 4,
                    }
                };
                ir_builder.append(load_instruction);

                Symbol symbol = {
                    .type = Symbol::ID::Index,
                    .result = { .index = load_instruction.load.index},
                };
                ir_builder.append(symbol);
                return symbol;
            } break;
            case NodeType::BinOp:
            {
                auto* left_node = node->bin_op.left;
                auto* right_node = node->bin_op.right;

                auto left_symbol = node_to_bytecode_value(allocator, ir_builder, type_declarations, current_function, left_node, expected_type);
                auto right_symbol = node_to_bytecode_value(allocator, ir_builder, type_declarations, current_function, right_node, expected_type);

                switch (node->bin_op.op)
                {
                    case BinOp::Plus:
                    {
                        InstructionInfo add_instruction = {
                            .id = Instruction::Add,
                            .add = {
                                .type = expected_type,
                                .left = left_symbol,
                                .right = right_symbol,
                                .index = ir_builder.current_fn->next_index++,
                            }
                        };
                        ir_builder.append(add_instruction);

                        Symbol result = {
                            .type = Symbol::ID::Index,
                            .result = {
                                .index = add_instruction.add.index,
                            }
                        };
                        ir_builder.append(result);
                        return result;
                    } break;
                    case BinOp::Cmp_Equal:
                    {
                        InstructionInfo icmp_instruction = {
                            .id = Instruction::ICmp,
                            .icmp = {
                                .id = ICmp::ID::Equal,
                                // @TODO: remove hardcoded value
                                .type = &type_declarations[static_cast<u32>(NativeTypeID::S32)],
                                .left = left_symbol,
                                .right = right_symbol,
                                .index = ir_builder.current_fn->next_index++,
                            },
                        };

                        ir_builder.append(icmp_instruction);

                        Symbol result = {
                            .type = Symbol::ID::Index,
                            .result = {.index = icmp_instruction.icmp.index},
                        };
                        ir_builder.append(result);

                        return result;
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
        return {};
    }

    void do_statement_node(Allocator* allocator, IRBuilder& ir_builder, TypeBuffer& type_declarations, FunctionDeclaration& current_function, Node* node);

    BasicBlock* append_basic_block(Allocator* allocator, IRBuilder& ir_builder, TypeBuffer& type_declarations, FunctionDeclaration& current_function, ScopeBlock* scope_block, bool* returned)
    {
        auto* new_one = ir_builder.append_basic_block(allocator);

        assert(scope_block->type == ScopeType::ConditionalBlock);

        for (auto* st_node : scope_block->statements)
        {
            switch (st_node->type)
            {
                case NodeType::Ret:
                {
                    auto* ret_type = current_function.type->ret_type;
                    assert(ret_type);

                    if (ret_type->id != TypeID::VoidType)
                    {
                        Node* ret_expr = st_node->ret.expr;
                        assert(ret_expr);

                        auto ret_symbol = node_to_bytecode_value(allocator, ir_builder, type_declarations, current_function, ret_expr, ret_type);

                        auto* conditional_alloca = ir_builder.get_conditional_alloca();
                        assert(conditional_alloca);

                        InstructionInfo store_conditional_alloca = {
                            .id = Instruction::Store,
                            .store = {
                                .type = ret_type,
                                .symbol = ret_symbol,
                                .alloca_i = conditional_alloca,
                                .alignment = 4,
                            }
                        };

                        ir_builder.append(store_conditional_alloca);
                    }

                    *returned = true;
                } break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }
        }

        ir_builder.current = ir_builder.current->parent;

        return new_one;
    }

    void do_statement_node(Allocator* allocator, IRBuilder& ir_builder, TypeBuffer& type_declarations, FunctionDeclaration& current_function, Node* node)
    {
        InstructionInfo instruction;

        switch (node->type)
        {
            case NodeType::VarDecl:
            {
                if (node->var_decl.value)
                {
                    auto symbol = node_to_bytecode_value(allocator, ir_builder, type_declarations, current_function, node->var_decl.value);
                    InstructionInfo instruction;
                    instruction.id = Instruction::Store;
                    instruction.store.type = node->var_decl.type;
                    instruction.store.symbol = symbol;
                    instruction.store.alloca_i = ir_builder.get_alloca(node, current_function);
                    assert(instruction.store.alloca_i);

                    instruction.store.alignment = 4;

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
                    auto symbol = node_to_bytecode_value(allocator, ir_builder, type_declarations, current_function, ret_expr, fn_type->ret_type);
                    instruction.ret = {
                        .type = current_function.type->ret_type,
                        .symbol = symbol,
                    };
                    ir_builder.append(instruction);
                }
                else
                {
                    assert(ret_type->id == TypeID::VoidType);
                }
            } break;
            case NodeType::Conditional:
            {
                auto* condition = node->conditional.condition;
                assert(condition);
                // @Info: boolean
                auto* br_type = &type_declarations[static_cast<u32>(NativeTypeID::U1)];
                auto condition_symbol = node_to_bytecode_value(allocator, ir_builder, type_declarations, current_function, condition, br_type);

                auto* if_block = node->conditional.if_block;
                assert(if_block);
                assert(if_block->type == ScopeType::ConditionalBlock);
                auto* else_block = node->conditional.else_block;
                assert(else_block);
                assert(else_block->type == ScopeType::ConditionalBlock);

                bool if_returned, else_returned;
                if_returned = else_returned = false;
                auto* if_bb = append_basic_block(allocator, ir_builder, type_declarations, current_function, if_block, &if_returned);
                auto* else_bb = append_basic_block(allocator, ir_builder, type_declarations, current_function, else_block, &else_returned);
                auto* branch_epilogue_bb = ir_builder.append_basic_block(allocator);
                Symbol if_label = ir_builder.get_block_label(if_bb);
                Symbol else_label = ir_builder.get_block_label(else_bb);
                Symbol epilogue_label = ir_builder.get_block_label(branch_epilogue_bb);

                // Return to parent scope
                auto* parent = branch_epilogue_bb->parent;
                ir_builder.current = parent;
                ir_builder.append({
                        .id = Instruction::Br,
                        .br = {
                            .type = br_type,
                            .condition = condition_symbol,
                            .if_label = if_label,
                            .else_label = else_label,
                        },
                    });
                ir_builder.print_block(ir_builder.current);
                ir_builder.current = if_bb;
                ir_builder.append({
                        .id = Instruction::Br,
                        .br = {
                            .if_label = epilogue_label,
                        },
                    });
                ir_builder.print_block(ir_builder.current);
                ir_builder.current = else_bb;
                ir_builder.append({
                        .id = Instruction::Br,
                        .br = {
                            .if_label = epilogue_label,
                        },
                    });
                ir_builder.print_block(ir_builder.current);

                ir_builder.current = branch_epilogue_bb;

                auto* ret_type = current_function.type->ret_type;
                if (ret_type->id != TypeID::VoidType)
                {
                    auto* conditional_alloca = ir_builder.get_conditional_alloca();

                    // @Info: we need to first load the conditional alloca
                    InstructionInfo load_instruction = {
                        .id = Instruction::Load,
                        .load = {
                            .type = ret_type,
                            .alloca_i = conditional_alloca,
                            .index = ir_builder.current_fn->next_index++,
                            .alignment = 4,
                        },
                    };
                    ir_builder.append(load_instruction);
                    auto load_index = load_instruction.load.index;


                    InstructionInfo ret_instruction = {
                        .id = Instruction::Ret,
                        .ret = {
                            .type = ret_type,
                            .symbol = {
                                .type = Symbol::ID::Index,
                                .result = { .index = load_index },
                            }
                        }
                    };
                    ir_builder.append(ret_instruction);
                    ir_builder.print_block(ir_builder.current);
                }
                else
                {

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
                .basic_block_labels = Buffer<LLVMID>::create(&llvm_allocator, 64),
                .basic_blocks = Buffer<BasicBlock>::create(&llvm_allocator, 64),
            };

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

            bool conditional_alloca = function.type->ret_type->id != TypeID::VoidType;

            auto& main_scope_statements = function.scope_blocks[0].statements;
            if (conditional_alloca)
            {
                for (auto* st_node : main_scope_statements)
                {
                    if (st_node->type == NodeType::Conditional)
                    {
                        if (introspect_for_conditional_allocas(st_node->conditional.if_block))
                        {
                            conditional_alloca = true;
                            break;
                        }
                        if (introspect_for_conditional_allocas(st_node->conditional.else_block))
                        {
                            conditional_alloca = true;
                            break;
                        }
                    }
                    // @TODO: here we need to comtemplate other cases which imply new blocks, like loops
                }
            }

            llvm_function.arg_count = arg_count;
            llvm_function.next_index = llvm_function.arg_count + 1;
            llvm_function.conditional_alloca = conditional_alloca;

            Buffer<InstructionInfo*> alloca_buffer = Buffer<InstructionInfo*>::create(&llvm_allocator, function.variables.len + conditional_alloca);
            Buffer<Symbol> symbol_buffer = Buffer<Symbol>::create(&llvm_allocator, 1024);

            ir_builder.current_fn = &llvm_function;
            //ir_builder.current = ir_builder.current_fn->basic_blocks.allocate();
            ir_builder.current = &ir_builder.current_fn->entry_block;
            ir_builder.current->instructions = ir_builder.current->instructions.create(&llvm_allocator, 1024);
            ir_builder.current_alloca_buffer = &alloca_buffer;
            ir_builder.current_symbol_buffer = &symbol_buffer;

            if (conditional_alloca)
            {
                Alloca alloca_i = {
                    .type = function.type->ret_type,
                    .index = ir_builder.current_fn->next_index++,
                    .count = 1,
                    .alignment = 4,
                };
                auto* alloca_i_ptr = ir_builder.append({
                    .id = Instruction::Alloca,
                    .alloca_i = alloca_i,
                    });
                ir_builder.current_alloca_buffer->append(alloca_i_ptr);
            }

            for (auto* var_node : function.variables)
            {
                assert(var_node->type == NodeType::VarDecl);

                ir_builder.create_alloca(var_node);
            }

            for (auto* st_node : main_scope_statements)
            {
                do_statement_node(&llvm_allocator, ir_builder, type_declarations, function, st_node);
            }
        }
    }
}