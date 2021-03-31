#include "llvm_bytecode.h"

#include <RNS/profiler.h>
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

        RNS_PATCH = 0xFF,
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
            EntryBlockLabel,
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

    struct Sub
    {
        Type* type;
        Symbol left, right;
        LLVMID index;
    };

    struct Mul
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
            UnsignedGreaterThan,
            UnsignedGreaterOrEqual,
            UnsignedLessThan,
            UnsignedLessOrEqual,
            SignedGreaterThan,
            SignedGreaterOrEqual,
            SignedLessThan,
            SignedLessOrEqual,
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
                case ID::UnsignedGreaterThan:
                {
                    return "ugt";
                }
                case ID::UnsignedGreaterOrEqual:
                {
                    return "uge";
                }

                case ID::UnsignedLessThan:
                {
                    return "ult";
                }
                case ID::UnsignedLessOrEqual:
                {
                    return "ule";
                }
                case ID::SignedGreaterThan:
                {
                    return "sgt";
                }
                case ID::SignedGreaterOrEqual:
                {
                    return "sge";
                }
                case ID::SignedLessThan:
                {
                    return "slt";
                }
                case ID::SignedLessOrEqual:
                {
                    return "sle";
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

    // @TODO: this is no LLVM instruction, but more of a patch for us to do
    enum class JumpType
    {
        Direct,
        Conditional,
    };

    enum class HighLevelType
    {
        Break,
        NoElse,
    };

    struct BasicBlock;

    struct PatchBlock
    {
        JumpType type;
        HighLevelType keyword;
        Symbol condition; // @Info: This is just for the conditional jump
        BasicBlock* ir_jump_from;
        Node* ast_true_jump;
        Node* ast_false_jump;
        Node* ast_parent_node;
        BasicBlock* ir_true_block;
        BasicBlock* ir_false_block;
        BasicBlock* ir_parent;
        bool done;
    };

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
            Sub sub;
            Mul mul;
            ICmp icmp;
            Br br;
        };

        void print(Symbol block_label)
        {
            if (block_label.type == Symbol::ID::EntryBlockLabel)
            {
                printf("fn_entry:\t");
            }
            else
            {
                printf("%u:\t", block_label.result.index);
            }
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
                case Instruction::Sub:
                {
                    char left_buffer[64];
                    char right_buffer[64];
                    sub.left.to_string(left_buffer);
                    sub.right.to_string(right_buffer);
                    printf("%%%u = sub %s %s, %s\n", sub.index, type_to_string(sub.type), left_buffer, right_buffer);
                } break;
                case Instruction::Mul:
                {
                    char left_buffer[64];
                    char right_buffer[64];
                    mul.left.to_string(left_buffer);
                    mul.right.to_string(right_buffer);
                    printf("%%%u = mul %s %s, %s\n", mul.index, type_to_string(mul.type), left_buffer, right_buffer);
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
                case Instruction::RNS_PATCH:
                {
                    printf("RNS Patch\n");
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
        Node* ast_block;
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

    struct IRBuilder;
    Symbol node_to_bytecode_value(Allocator* allocator, IRBuilder& ir_builder, TypeBuffer& type_declarations, FunctionDeclaration& ast_current_function, Node* node, Type* expected_type = nullptr);
    struct IRBuilder
    {
        BasicBlock* current;
        Function* current_fn;
        Buffer<InstructionInfo*>* current_alloca_buffer;
        // @TODO: Consider using another kind of data structure to speed up compilation
        Buffer<PatchBlock>* current_patch_list;
        Buffer<Symbol>* current_symbol_buffer;

        InstructionInfo* append(InstructionInfo instruction)
        {
            auto* instr = current->instructions.append(instruction);
            //instr->print(get_block_label(current));
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
            auto* alloca_i_ptr = append({ .id = Instruction::Alloca, .alloca_i = alloca_i, });
            current_alloca_buffer->append(alloca_i_ptr);
            append({ .type = Symbol::ID::Index, .result = {.index = alloca_i.index } });
        }

        void create_store(Node* var_node, Symbol assignment_value, FunctionDeclaration& ast_current_function)
        {
            assert(var_node);
            assert(var_node->type == NodeType::VarDecl);
            assert(var_node->var_decl.type);
            InstructionInfo instruction;
            instruction.id = Instruction::Store;
            instruction.store.type = var_node->var_decl.type;
            instruction.store.symbol = assignment_value;
            instruction.store.alloca_i = get_alloca(var_node, ast_current_function);
            assert(instruction.store.alloca_i);

            instruction.store.alignment = 4;

            append(instruction);
        }

        void encode_assign(Allocator* allocator, TypeBuffer& type_declarations, FunctionDeclaration& ast_current_function, Node* node)
        {
            assert(node->type == NodeType::BinOp);
            assert(node->bin_op.op == BinOp::Assign);

            auto* ast_assignment_value = node->bin_op.right;
            auto* var_expr_node = node->bin_op.left;
            assert(var_expr_node);
            assert(var_expr_node->type == NodeType::VarExpr);
            auto* var_decl_node = var_expr_node->var_expr.mentioned;
            assert(var_decl_node);
            assert(var_decl_node->type == NodeType::VarDecl);
            auto* var_type = var_decl_node->var_decl.type;
            assert(var_type);
            auto assignment_value = node_to_bytecode_value(allocator, *this, type_declarations, ast_current_function, ast_assignment_value, var_type);

            create_store(var_decl_node, assignment_value, ast_current_function);
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

        BasicBlock* append_basic_block(Allocator* allocator, Node* ast_scope_block = nullptr)
        {
            auto label_index = current_fn->next_index++;
            if (label_index == 13)
            {
                int k = 123213;
            }
            current_fn->basic_block_labels.append(label_index);
            auto* new_one = current_fn->basic_blocks.allocate();
            new_one->parent = current;
            current = new_one;
            current->instructions = current->instructions.create(allocator, 64);
            current->ast_block = ast_scope_block;

            return new_one;
        }

        Symbol get_block_label(BasicBlock* basic_block)
        {
            if (basic_block == &this->current_fn->entry_block)
            {
                Symbol block_label = { .type = Symbol::ID::EntryBlockLabel };
                return block_label;
            }
            else
            {
                auto index = current_fn->basic_blocks.get_id(basic_block);
                auto label_index = current_fn->basic_block_labels[index];

                Symbol block_label = {
                    .type = Symbol::ID::Label,
                    .result = {.index = label_index }
                };
                return block_label;
            }
        }

        void append_branch(BasicBlock* origin, BasicBlock* destination)
        {
            auto* original_scope = current;

            current = origin;
            auto dest_label = get_block_label(destination);
            InstructionInfo br_instruction = {
                .id = Instruction::Br,
                .br = {
                    .if_label = dest_label,
                },
            };
            append(br_instruction);
            current = original_scope;
        }

        void append_conditional_branch(Symbol condition, BasicBlock* origin_block, BasicBlock* if_block, BasicBlock* else_block, TypeBuffer& type_declarations)
        {
            auto* original_scope = current;

            current = origin_block;
            auto if_label = get_block_label(if_block);
            auto else_label = get_block_label(else_block);
            InstructionInfo br_instruction = {
                .id = Instruction::Br,
                .br = {
                    .type = &type_declarations[(u32)NativeTypeID::U1],
                    .condition = condition,
                    .if_label = if_label,
                    .else_label = else_label,
                },
            };
            append(br_instruction);
            current = original_scope;
        }

        BasicBlock* change_block(BasicBlock* block)
        {
            assert(block != current);
            auto* current_block = current;
            current = block;
            return current_block;
        }

        void print_block(BasicBlock* basic_block)
        {
#if 1
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
                instr.print(get_block_label(basic_block));
            }
#endif
        }
    };

    bool introspect_for_conditional_allocas(Node* scope)
    {
        if (!scope)
        {
            return false;
        }

        for (auto* st_node : scope->block.statements)
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
            else if (st_node->type == NodeType::Loop)
            {
                if (introspect_for_conditional_allocas(st_node->loop.body))
                {
                    return true;
                }
            }
        }

        return false;
    }

    // @TODO: Add typechecking again <Type, Symbol>
    Symbol node_to_bytecode_value(Allocator* allocator, IRBuilder& ir_builder, TypeBuffer& type_declarations, FunctionDeclaration& current_function, Node* node, Type* expected_type)
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
                    case BinOp::Minus:
                    {
                        InstructionInfo sub_instruction = {
                            .id = Instruction::Sub,
                            .sub = {
                                .type = expected_type,
                                .left = left_symbol,
                                .right = right_symbol,
                                .index = ir_builder.current_fn->next_index++,
                            }
                        };
                        ir_builder.append(sub_instruction);

                        Symbol result = {
                            .type = Symbol::ID::Index,
                            .result = {
                                .index = sub_instruction.add.index,
                            }
                        };
                        ir_builder.append(result);
                        return result;
                    } break;
                    case BinOp::Mul:
                    {
                        InstructionInfo mul_instruction = {
                            .id = Instruction::Mul,
                            .mul = {
                                .type = expected_type,
                                .left = left_symbol,
                                .right = right_symbol,
                                .index = ir_builder.current_fn->next_index++,
                            }
                        };
                        ir_builder.append(mul_instruction);

                        Symbol result = {
                            .type = Symbol::ID::Index,
                            .result = {
                                .index = mul_instruction.mul.index,
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
                    case BinOp::Cmp_LessThan:
                    {
                        InstructionInfo icmp_instruction = {
                            .id = Instruction::ICmp,
                            .icmp = {
                                .id = ICmp::ID::SignedLessThan, // @TODO: remove signed value and compute based on the operands
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
                    case BinOp::Cmp_GreaterThan:
                    {
                        InstructionInfo icmp_instruction = {
                            .id = Instruction::ICmp,
                            .icmp = {
                                .id = ICmp::ID::SignedGreaterThan, // @TODO: remove signed value and compute based on the operands
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
                    }
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

    void work_block(Allocator* allocator, IRBuilder& ir_builder, TypeBuffer& type_declarations, FunctionDeclaration& ast_current_function, Node* ast_scope, BasicBlock* block_to_work)
    {
        for (auto* st_node : ast_scope->block.statements)
        {
            switch (st_node->type)
            {
                case NodeType::Break:
                {
                    auto* scope_to_jump_to = st_node->break_.block;
                    assert(scope_to_jump_to);

                    while (scope_to_jump_to)
                    {
                        if (scope_to_jump_to->block.type == Block::Type::LoopBody)
                        {
                            break;
                        }

                        scope_to_jump_to = scope_to_jump_to->parent;
                    }

                    assert(scope_to_jump_to);
                    assert(ir_builder.current == block_to_work);

                    PatchBlock patch = {
                        .type = JumpType::Direct,
                        .keyword = HighLevelType::Break,
                        .ir_jump_from = ir_builder.current,
                        .ast_true_jump = scope_to_jump_to,
                        .ast_false_jump = nullptr,
                        .ast_parent_node = nullptr,
                        .ir_parent = block_to_work,
                    };
                    ir_builder.current_patch_list->append(patch);
                } break;
                case NodeType::BinOp:
                {
                    assert(st_node->bin_op.op == BinOp::Assign);
                    ir_builder.encode_assign(allocator, type_declarations, ast_current_function, st_node);
                } break;
            case NodeType::VarDecl:
            {
                auto* ast_assignment_value = st_node->var_decl.value;
                auto* var_type = st_node->var_decl.type;
                assert(var_type);
                if (ast_assignment_value)
                {
                    auto assignment_value = node_to_bytecode_value(allocator, ir_builder, type_declarations, ast_current_function, ast_assignment_value, var_type);
                    ir_builder.create_store(st_node, assignment_value, ast_current_function);
                }
            } break;
            case NodeType::Ret:
            {
                // @TODO: change this
                if (true)
                {
                    auto* ret_expr = st_node->ret.expr;
                    auto* fn_type = ast_current_function.type;
                    assert(fn_type);
                    auto* ret_type = fn_type->function_type.ret_type;
                    assert(ret_type);
                    InstructionInfo instruction;
                    instruction.id = Instruction::Ret;
                    if (ret_expr)
                    {
                        auto symbol = node_to_bytecode_value(allocator, ir_builder, type_declarations, ast_current_function, ret_expr, fn_type->function_type.ret_type);
                        instruction.ret = {
                            .type = ast_current_function.type->function_type.ret_type,
                            .symbol = symbol,
                        };
                        ir_builder.append(instruction);
                    }
                    else
                    {
                        assert(ret_type->id == TypeID::VoidType);
                    }
                }
                else
                {
                    auto* ret_type = ast_current_function.type->function_type.ret_type;
                    assert(ret_type);

                    if (ret_type->id != TypeID::VoidType)
                    {
                        Node* ret_expr = st_node->ret.expr;
                        assert(ret_expr);

                        auto ret_symbol = node_to_bytecode_value(allocator, ir_builder, type_declarations, ast_current_function, ret_expr, ret_type);

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
                }
            } break;
            case NodeType::Conditional:
            {
                auto* current_scope = ir_builder.current;
                auto* condition = st_node->conditional.condition;
                assert(condition);
                // @Info: boolean
                auto* br_type = &type_declarations[static_cast<u32>(NativeTypeID::U1)];
                auto condition_symbol = node_to_bytecode_value(allocator, ir_builder, type_declarations, ast_current_function, condition, br_type);

                auto* ast_if_block = st_node->conditional.if_block;
                assert(ast_if_block);
                assert(ast_if_block->block.type == Block::Type::ConditionalBlock);
                auto* if_basic_block = ir_builder.append_basic_block(allocator, ast_if_block);
                work_block(allocator, ir_builder, type_declarations, ast_current_function, ast_if_block, if_basic_block);

                auto* ast_else_block = st_node->conditional.else_block;
                // Info: if there's an else block, it generates around that. Else it's an empty block (an if-end block, to be clear)
                BasicBlock* next_basic_block = ir_builder.append_basic_block(allocator, ast_else_block); 
                if (ast_else_block)
                {
                    work_block(allocator, ir_builder, type_declarations, ast_current_function, ast_else_block, next_basic_block);
                }
                else
                {
                    bool do_patch = true;
                    for (auto& patch : *ir_builder.current_patch_list)
                    {
                        if (patch.ast_true_jump == st_node)
                        {
                            do_patch = false;
                            break;
                        }
                    }

                    if (do_patch)
                    {
                        // generate some kind of patch here
                        PatchBlock no_else_patch = {
                            .type = JumpType::Direct,
                            .keyword = HighLevelType::NoElse,
                            .ir_jump_from = next_basic_block,
                            .ast_true_jump = st_node->parent,
                            .ast_parent_node = st_node->parent,
                            .ir_parent = next_basic_block->parent,
                        };
                        ir_builder.current_patch_list->append(no_else_patch);
                    }
                }
                for (auto& patch : *ir_builder.current_patch_list)
                {
                    if (!patch.done)
                    {
                        assert(patch.type == JumpType::Direct);
                        if (patch.ast_true_jump == st_node)
                        {
                            if (ast_else_block)
                            {
                                patch.ir_true_block = next_basic_block;
                                ir_builder.append_branch(patch.ir_jump_from, patch.ir_true_block);
                                patch.done = true;
                            }
                            else
                            {
                                assert(patch.ast_true_jump->parent);
                                patch.ast_true_jump = patch.ast_true_jump->parent;
                            }
                        }
                        else if (patch.ast_false_jump == st_node)
                        {
                            RNS_NOT_IMPLEMENTED;
                        }
                    }
                }
                ir_builder.append_conditional_branch(condition_symbol, current_scope, if_basic_block, next_basic_block, type_declarations);
            } break;
            case NodeType::Loop:
            {
                auto* ast_loop_prefix = st_node->loop.prefix;
                auto* ast_loop_body = st_node->loop.body;
                auto* ast_loop_postfix = st_node->loop.postfix;

                auto* loop_parent_basic_block = ir_builder.current;

                auto* loop_prefix_basic_block = ir_builder.append_basic_block(allocator, ast_loop_prefix);
                ir_builder.append_branch(loop_parent_basic_block, loop_prefix_basic_block);

                // @Info: this is supposed to be the loop condition. @TODO: come up with a better name
                assert(ast_loop_prefix->block.statements.len == 1);
                auto* loop_condition = ast_loop_prefix->block.statements[0];

                Symbol cmp_result = node_to_bytecode_value(allocator, ir_builder, type_declarations, ast_current_function, loop_condition, &type_declarations[(u32)NativeTypeID::U1]);

                bool returned = false;
                bool jumps_to_patch = false;
                auto* loop_body_basic_block = ir_builder.append_basic_block(allocator, ast_loop_body);
                work_block(allocator, ir_builder, type_declarations, ast_current_function, ast_loop_body, loop_body_basic_block);
                auto* body_current_scope = ir_builder.current;
                
                ir_builder.change_block(loop_parent_basic_block);
                auto* loop_postfix_basic_block = ir_builder.append_basic_block(allocator, ast_loop_postfix);
                work_block(allocator, ir_builder, type_declarations, ast_current_function, ast_loop_postfix, loop_postfix_basic_block);

                ir_builder.change_block(loop_parent_basic_block);
                auto* loop_end_basic_block = ir_builder.append_basic_block(allocator);
                ir_builder.change_block(loop_parent_basic_block);

                // @TODO: make conditional branch
                ir_builder.append_conditional_branch(cmp_result, loop_prefix_basic_block, loop_body_basic_block, loop_end_basic_block, type_declarations);
                ir_builder.append_branch(body_current_scope, loop_postfix_basic_block);
                ir_builder.append_branch(loop_postfix_basic_block, loop_prefix_basic_block);

                for (auto& patch : *ir_builder.current_patch_list)
                {
                    if (!patch.done)
                    {
                        if (patch.ast_true_jump == ast_loop_body)
                        {
                            assert(patch.type == JumpType::Direct);
                            switch (patch.keyword)
                            {
                                case HighLevelType::Break:
                                {
                                    patch.ir_true_block = loop_end_basic_block;
                                    ir_builder.append_branch(patch.ir_jump_from, patch.ir_true_block);
                                    patch.done = true;
                                } break;
                                case HighLevelType::NoElse:
                                {
                                    patch.ir_true_block = body_current_scope;
                                    ir_builder.append_branch(patch.ir_jump_from, patch.ir_true_block);
                                    patch.done = true;
                                } break;
                                default:
                                    RNS_NOT_IMPLEMENTED;
                                    break;
                            }
                        }
                        else if (patch.ast_false_jump == ast_loop_body)
                        {
                            RNS_NOT_IMPLEMENTED;
                        }
                        else
                        {
                            RNS_NOT_IMPLEMENTED;
                        }
                    }
                }

                ir_builder.change_block(loop_end_basic_block);
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
            }
        }
    }

    void encode(Compiler& compiler, NodeBuffer& node_buffer, FunctionTypeBuffer& function_type_declarations, TypeBuffer& type_declarations, FunctionDeclarationBuffer& function_declarations)
    {
        RNS_PROFILE_FUNCTION();
        compiler.subsystem = Compiler::Subsystem::IR;
        auto llvm_allocator = create_suballocator(&compiler.page_allocator, RNS_MEGABYTE(50));
        assert(function_declarations.len == 1);
        IRBuilder ir_builder = {};

        for (auto& ast_current_function : function_declarations)
        {
            Function llvm_function = {
                .basic_block_labels = Buffer<LLVMID>::create(&llvm_allocator, 64),
                .basic_blocks = Buffer<BasicBlock>::create(&llvm_allocator, 64),
            };

            auto arg_count = 0;
            for (auto* var_node : ast_current_function->function.variables)
            {
                if (var_node->var_decl.is_fn_arg)
                {
                    arg_count++;
                    continue;
                }
                break;
            }

            auto* main_scope = ast_current_function->function.scope_blocks[0];
            auto& main_scope_statements = main_scope->block.statements;

            bool conditional_alloca = ast_current_function->function.type->function_type.ret_type->id != TypeID::VoidType;
            if (conditional_alloca)
            {
                conditional_alloca = false;

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
                    else if (st_node->type == NodeType::Loop)
                    {
                        if (introspect_for_conditional_allocas(st_node->loop.body))
                        {
                            conditional_alloca = true;
                            break;
                        }
                    }
                    // @Warning: here we need to comtemplate other cases which imply new blocks, like loops
                }
            }

            llvm_function.arg_count = arg_count;
            llvm_function.next_index = llvm_function.arg_count + 1;
            llvm_function.conditional_alloca = conditional_alloca;

            auto alloca_count = ast_current_function->function.variables.len + conditional_alloca;
            Buffer<InstructionInfo*> alloca_buffer = {};
            if (alloca_count > 0)
            {
                alloca_buffer = Buffer<InstructionInfo*>::create(&llvm_allocator, alloca_count);
            }

            auto symbol_buffer = Buffer<Symbol>::create(&llvm_allocator, 1024);
            auto patch_buffer = Buffer<PatchBlock>::create(&llvm_allocator, 1024);

            ir_builder.current_fn = &llvm_function;
            // @TODO: abstract this away
            {
                ir_builder.change_block(&ir_builder.current_fn->entry_block);
                ir_builder.current->instructions = ir_builder.current->instructions.create(&llvm_allocator, 1024);
                ir_builder.current_alloca_buffer = &alloca_buffer;
                ir_builder.current_symbol_buffer = &symbol_buffer;
                ir_builder.current_patch_list = &patch_buffer;

                if (conditional_alloca)
                {
                    Alloca conditional_alloca_instruction = {
                        .type = ast_current_function->function.type->function_type.ret_type,
                        .index = ir_builder.current_fn->next_index++,
                        .count = 1,
                        .alignment = 4,
                    };
                    auto* conditional_alloca_instruction_ptr = ir_builder.append({
                        .id = Instruction::Alloca,
                        .alloca_i = conditional_alloca_instruction,
                        });
                    ir_builder.current_alloca_buffer->append(conditional_alloca_instruction_ptr);
                }

                for (auto* var_node : ast_current_function->function.variables)
                {
                    assert(var_node->type == NodeType::VarDecl);

                    ir_builder.create_alloca(var_node);
                }
            }

            work_block(&llvm_allocator, ir_builder, type_declarations, ast_current_function->function, ast_current_function->function.scope_blocks[0], &ir_builder.current_fn->entry_block);

            ir_builder.print_block(&ir_builder.current_fn->entry_block);

            for (auto& basic_block : ir_builder.current_fn->basic_blocks)
            {
                ir_builder.print_block(&basic_block);
            }
        }
    }
}