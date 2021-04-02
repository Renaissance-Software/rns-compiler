#include "llvm_bytecode.h"

#include <RNS/profiler.h>
#include <llvm/Bitcode/LLVMBitCodes.h>
#include "typechecker.h"

namespace LLVM
{
    struct BasicBlock;
    struct Function;
    struct Instruction;
    using BasicBlockBuffer = Buffer<BasicBlock>;
    using InstructionBuffer = Buffer<Instruction>;
    using namespace RNS;
    using namespace AST;
    using IDType = u64;

    enum class InstructionID : u8
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

    struct IDManager
    {
        static Buffer<String> names;
        static StringBuffer buffer;
        static IDType next_id;

        static IDType append(const char* name = nullptr, u32 name_len = 0)
        {
            auto id = next_id++;
            if (name)
            {
                if (name_len == 0)
                {
                    name_len = strlen(name);
                }
                if (name > 0)
                {
                    auto allocated_name = buffer.append(name, name_len);
                    String new_string = {
                        .ptr = allocated_name,
                        .len = name_len,
                    };
                    names.append(new_string);
                    return id;
                }
            }

            names.append({});
            return id;
        }

        static void init(Allocator* allocator, s64 name_count, s64 string_buffer_capacity)
        {
            names = names.create(allocator, name_count);
            buffer = buffer.create(allocator, string_buffer_capacity);
        }
    };
    Buffer<String> IDManager::names;
    StringBuffer IDManager::buffer;
    IDType IDManager::next_id;

    struct Value
    {
        IDType id;
        Type* type;

        static Value New(Type* type = nullptr, const char* name = nullptr)
        {
            Value e = {
                .id = IDManager::append(name),
                .type = type,
            };

            return e;
        }

        void print();
    };

    struct SlotTracker
    {
        u64 next_id;
        u64 not_used;
        u64 starting_id;
        Buffer<Value*> map;

        static SlotTracker create(Allocator* allocator, s64 count)
        {
            SlotTracker slot_tracker = {
                .map = slot_tracker.map.create(allocator, count),
            };
            
            return slot_tracker;
        }

        u64 find(Value* value)
        {
            for (auto i = 0; i < map.len; i++)
            {
                if (map[i] == value)
                {
                    return i + starting_id;
                }
            }

            return UINT64_MAX;
        }

        template <typename T>
        u64 new_id(T value)
        {
            auto id = next_id++;
            map.append(reinterpret_cast<Value*>(value));
            return id;
        }
    };

    struct APInt
    {
        Value e;
        union
        {
            u64 eight_byte;
            u64* more_than_eight_byte;
        } value;
        u32 bit_count;
        bool is_signed;

        static APInt* get(Allocator* allocator, Type* integer_type, u64 value, bool is_signed)
        {
            assert(integer_type->id == TypeID::IntegerType);
            auto bits = integer_type->integer_t.bits;
            assert(bits >= 1 && bits <= 64);

            auto* new_int = new(allocator) APInt;
            assert(new_int);
            *new_int = {
                .e = Value::New(integer_type),
                .value = value,
                .bit_count = bits,
                .is_signed = is_signed,
            };

            return new_int;
        }
    };

    enum class CmpType : u8
    {
        // Opcode            U L G E    Intuitive operation
        FCMP_FALSE = 0, ///< 0 0 0 0    Always false (always folded)
        FCMP_OEQ = 1,   ///< 0 0 0 1    True if ordered and equal
        FCMP_OGT = 2,   ///< 0 0 1 0    True if ordered and greater than
        FCMP_OGE = 3,   ///< 0 0 1 1    True if ordered and greater than or equal
        FCMP_OLT = 4,   ///< 0 1 0 0    True if ordered and less than
        FCMP_OLE = 5,   ///< 0 1 0 1    True if ordered and less than or equal
        FCMP_ONE = 6,   ///< 0 1 1 0    True if ordered and operands are unequal
        FCMP_ORD = 7,   ///< 0 1 1 1    True if ordered (no nans)
        FCMP_UNO = 8,   ///< 1 0 0 0    True if unordered: isnan(X) | isnan(Y)
        FCMP_UEQ = 9,   ///< 1 0 0 1    True if unordered or equal
        FCMP_UGT = 10,  ///< 1 0 1 0    True if unordered or greater than
        FCMP_UGE = 11,  ///< 1 0 1 1    True if unordered, greater than, or equal
        FCMP_ULT = 12,  ///< 1 1 0 0    True if unordered or less than
        FCMP_ULE = 13,  ///< 1 1 0 1    True if unordered, less than, or equal
        FCMP_UNE = 14,  ///< 1 1 1 0    True if unordered or not equal
        FCMP_TRUE = 15, ///< 1 1 1 1    Always true (always folded)
        FIRST_FCMP_PREDICATE = FCMP_FALSE,
        LAST_FCMP_PREDICATE = FCMP_TRUE,
        BAD_FCMP_PREDICATE = FCMP_TRUE + 1,
        ICMP_EQ = 32,  ///< equal
        ICMP_NE = 33,  ///< not equal
        ICMP_UGT = 34, ///< unsigned greater than
        ICMP_UGE = 35, ///< unsigned greater or equal
        ICMP_ULT = 36, ///< unsigned less than
        ICMP_ULE = 37, ///< unsigned less or equal
        ICMP_SGT = 38, ///< signed greater than
        ICMP_SGE = 39, ///< signed greater or equal
        ICMP_SLT = 40, ///< signed less than
        ICMP_SLE = 41, ///< signed less or equal
        FIRST_ICMP_PREDICATE = ICMP_EQ,
        LAST_ICMP_PREDICATE = ICMP_SLE,
        BAD_ICMP_PREDICATE = ICMP_SLE + 1
    };

    struct CallBase
    {
        FunctionType* function_type;
    };
    struct AllocaInstruction
    {
        Type* allocated_type;
    };

    struct GetElementPtrInstruction
    {
        Type* source_type;
        Type* result_type;
    };

    struct Compare
    {
        CmpType type;
    };

    struct Instruction
    {
        struct
        {
            Value value;
            InstructionID id;
            BasicBlock* parent;
        } base;

        Value* operands[15];
        u8 operand_count;

        union
        {
            AllocaInstruction alloca_i;
            GetElementPtrInstruction get_element_ptr;
            Compare compare;
        };

        void print(SlotTracker& slot_tracker)
        {
            switch (base.id)
            {
                case InstructionID::Alloca:
                {
                    auto id = slot_tracker.new_id(this);
                    printf("%%%llu = alloca i32, align 4\n", id);
                } break;
                case InstructionID::Store:
                {
                    auto* value = operands[0];
                    auto* ptr = operands[1];
                    auto id = slot_tracker.find(ptr);
                    printf("store i32 ");
                    value->print();
                    printf(", i32* %%%llu, align 4\n", id);
                } break;
                case InstructionID::Br:
                {
                    RNS_NOT_IMPLEMENTED;
                    if (operand_count == 1)
                    {

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
    };

    void Value::print()
    {
        switch (type->id)
        {
            case TypeID::IntegerType:
            {
                auto* integer_value = (APInt*)this;
                printf("%llu", integer_value->value.eight_byte);
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    struct Argument
    {
        Value value;
    };

    struct Module
    {
        Buffer<Function> functions;

        static Module create(Allocator* allocator)
        {
            Module module = {
                .functions = module.functions.create(allocator, 64),
            };

            return module;
        }
    };

    struct Function
    {
        Value value;
        Buffer<BasicBlock*> basic_blocks;
        Argument* arguments;
        s64 arg_count;
        // @TODO: symbol table

        static Function* create(Module* module, Type* function_type, /* @TODO: linkage*/ const char* name)
        {
            auto* function = module->functions.allocate();
            *function = {
                .value = {
                    .id = IDManager::append(name),
                    .type = function_type,
                },
                // @TODO: basic blocks, args...
            };
            return function;
        }

        static FunctionType* get_type(FunctionType* function_type)
        {
            // @TODO: to be implemented in the future
            return function_type;
        }

        static FunctionType* get_type(Type* return_type, Slice<Type*> types, bool var_args)
        {
            RNS_NOT_IMPLEMENTED;
            return nullptr;
        }
    };

    struct BasicBlock
    {
        Value value;
        Function* parent;
        Buffer<Instruction*> instructions;
        u32 use_count;

        static BasicBlock create(const char* name = nullptr)
        {
            // @TODO: consider dealing with inserting the block in the middle of the array
            BasicBlock new_basic_block = {
                .value = Value::New(Type::get_label(), name),
            };

            return new_basic_block;
        }

        void print(SlotTracker& slot_tracker)
        {
            if (parent->basic_blocks[0] == this)
            {
                printf("Entry block:\n");
            }
            else
            {
                auto id = slot_tracker.new_id(this);
                printf("%llu:\n", id);
            }
        }
    };

    struct Builder
    {
        BasicBlock* current;
        Function* function;
        /// <summary> Guarantee pointer stability for members
        BasicBlockBuffer* basic_block_buffer;
        InstructionBuffer* instruction_buffer;
        /// </summary>
        s64 next_alloca_index;

        // @TODO: guarantee pointer stability
        Instruction* create_alloca(Type* type, Value* array_size = nullptr, const char* name = nullptr)
        {
            Instruction i = {
                .base {
                    .value = Value::New(),
                    .id = InstructionID::Alloca,
                },
                .alloca_i = {
                    .allocated_type = PointerType::get(type),
                }
            };

            return insert_alloca(i);
        }

        Instruction* create_store(Value* value, Value* ptr, bool is_volatile = false)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(),
                    .id = InstructionID::Store,
                },
                .operands = { value, ptr },
                .operand_count = 2,
            };

            return insert_at_end(i);
        }

        Instruction* create_load(Type* type, Value* value, bool is_volatile = false, const char* name = nullptr)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(type),
                    .id = InstructionID::Load,
                },
                .operands = { value},
                .operand_count = 1,
            };

            return insert_at_end(i);
        }

        void append_to_function(BasicBlock* block)
        {
            assert(function);

            function->basic_blocks.append(block);
            block->parent = function;
        }

        BasicBlock* set_block(BasicBlock* block)
        {
            assert(current != block);
            auto* previous = current;
            current = block;
            return previous;
        }

        BasicBlock* create_block(Allocator* allocator, const char* name = nullptr)
        {
            BasicBlock* basic_block = basic_block_buffer->append(BasicBlock::create(name));
            // @TODO: this should change
            basic_block->instructions = basic_block->instructions.create(allocator, 64);
            return basic_block;
        }

        Instruction* create_br(BasicBlock* dst_block)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(),
                    .id = InstructionID::Br,
                },
                .operands = { reinterpret_cast<Value*>(dst_block) },
                .operand_count = 1,
            };

            dst_block->use_count++;
            return insert_at_end(i);
        }
        
        Instruction* create_conditional_br(BasicBlock* true_block, BasicBlock* else_block, Value* condition)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(),
                    .id = InstructionID::Br,
                },
                .operands = { reinterpret_cast<Value*>(true_block),  reinterpret_cast<Value*>(true_block), condition },
                .operand_count = 3,
            };
            true_block->use_count++;
            else_block->use_count++;

            return insert_at_end(i);
        }

        Instruction* create_icmp(CmpType compare_type, Value* left, Value* right, const char* name = nullptr)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(),
                    .id = InstructionID::ICmp,
                },
                .operands = { left, right },
                .operand_count = 2,
                .compare = { .type = compare_type }
            };

            return insert_at_end(i);
        }

        Instruction* create_add(Value* left, Value* right, const char* name = nullptr)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(),
                    .id = InstructionID::Add,
                },
                .operands = { left, right },
                .operand_count = 2,
            };

            return insert_at_end(i);
        }

        Instruction* create_sub(Value* left, Value* right, const char* name = nullptr)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(),
                    .id = InstructionID::Sub,
                },
                .operands = { left, right },
                .operand_count = 2,
            };

            return insert_at_end(i);
        }

        Instruction* create_mul(Value* left, Value* right, const char* name = nullptr)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(),
                    .id = InstructionID::Mul,
                },
                .operands = { left, right },
                .operand_count = 2,
            };

            return insert_at_end(i);
        }

        Instruction* create_ret(Value* value)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(),
                    .id = InstructionID::Ret,
                },
                .operands = { value },
                .operand_count = 1,
            };

            return insert_at_end(i);
        }
        Instruction* create_ret_void()
        {
            RNS_NOT_IMPLEMENTED;
            return nullptr;
        }

    private:
        Instruction* insert_alloca(Instruction alloca_instruction)
        {
            assert(function);
            assert(instruction_buffer);

            auto* i = instruction_buffer->append(alloca_instruction);
            auto* entry_block = function->basic_blocks[0];
            entry_block->instructions.insert_at(i, next_alloca_index++);
            i->base.parent = entry_block;
            return i;
        }

        inline Instruction* insert_at_end(Instruction instruction)
        {
            assert(function);
            assert(instruction_buffer);
            assert(current);
            auto* i = instruction_buffer->append(instruction);
            auto* result = current->instructions.append(i);
            i->base.parent = current;
            return i;
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

    void emit_jump(Allocator* allocator, Builder& builder, BasicBlock* dst_block)
    {
        builder.create_br(dst_block);
        auto* new_block = builder.create_block(allocator);
        builder.append_to_function(new_block);
        builder.set_block(new_block);
    }

    Value* do_node(Allocator* allocator, Builder& builder, Node* node, Type* expected_type = nullptr)
    {
        switch (node->type)
        {
            case NodeType::Block:
            {
                assert(node->block.type != Block::Type::Function);

                for (auto* st_node : node->block.statements)
                {
                    do_node(allocator, builder, st_node);
                }
            } break;
            case NodeType::Conditional:
            {
                auto* ast_condition = node->conditional.condition;
                auto* ast_if_block = node->conditional.if_block;
                auto* ast_else_block = node->conditional.else_block;

                auto* exit_block = builder.create_block(allocator);
                auto* if_block = builder.create_block(allocator);
                BasicBlock* else_block = exit_block;
                if (ast_else_block)
                {
                    else_block = builder.create_block(allocator);
                }

                node->conditional.exit_block = exit_block;

                auto* condition = do_node(allocator, builder, ast_condition, Type::get_bool_type());
                assert(condition);

                auto exit_block_in_use = true;
                if (if_block != else_block)
                {
                    builder.create_conditional_br(if_block, else_block, condition);
                }
                else
                {
                    if ((exit_block_in_use = exit_block->use_count))
                    {
                        builder.create_br(exit_block);
                    }
                }

                if (if_block != exit_block)
                {
                    builder.append_to_function(if_block);
                    do_node(allocator, builder, ast_if_block);

                    builder.create_br(exit_block);
                }

                if (else_block != exit_block)
                {
                    builder.append_to_function(else_block);
                    do_node(allocator, builder, ast_else_block);

                    builder.create_br(exit_block);
                }

                if (exit_block_in_use)
                {
                    builder.append_to_function(exit_block);
                }
            } break;
            case NodeType::Loop:
            {
                auto* ast_loop_prefix = node->loop.prefix;
                auto* ast_loop_body = node->loop.body;
                auto* ast_loop_postfix = node->loop.postfix;

                auto loop_prefix_block = builder.create_block(allocator);
                auto loop_body_block = builder.create_block(allocator);
                auto loop_postfix_block = builder.create_block(allocator);
                auto loop_end_block = builder.create_block(allocator);

                auto& loop_continue_block = loop_postfix_block;
                node->loop.continue_block = loop_continue_block;
                node->loop.exit_block = loop_end_block;

                builder.create_br(loop_prefix_block);
                builder.append_to_function(loop_prefix_block);

                assert(ast_loop_prefix->block.statements.len == 1);
                auto* ast_condition = ast_loop_prefix->block.statements[0];
                auto* condition = do_node(allocator, builder, ast_condition, Type::get_bool_type());
                assert(condition);

                builder.create_conditional_br(loop_body_block, loop_end_block, condition);
                builder.append_to_function(loop_body_block);

                do_node(allocator, builder, ast_loop_body);

                builder.create_br(loop_postfix_block);

                builder.append_to_function(loop_postfix_block);
                do_node(allocator, builder, ast_loop_postfix);

                builder.create_br(loop_prefix_block);
                builder.append_to_function(loop_end_block);
            } break;
            case NodeType::Break:
            {
                auto* ast_jump_target = node->break_.target;
                assert(ast_jump_target->type == NodeType::Loop);
                auto* jump_target = reinterpret_cast<BasicBlock*>(ast_jump_target->loop.exit_block);
                assert(jump_target);

                emit_jump(allocator, builder, jump_target);
            } break;
            case NodeType::VarDecl:
            {
                auto* type = node->var_decl.type;
                auto* value = node->var_decl.value;
                auto* var_alloca = builder.create_alloca(type);
                node->var_decl.backend_ref = var_alloca;
                if (value)
                {
                    assert(type);
                    auto* expression = do_node(allocator, builder, value, type);
                    assert(expression);
                    builder.create_store(expression, reinterpret_cast<Value*>(var_alloca), false);
                }
            } break;
            case NodeType::IntLit:
            {
                auto* result = APInt::get(allocator, Type::get_integer_type(32, true), node->int_lit.lit, node->int_lit.is_signed);
                assert(result);
                return reinterpret_cast<Value*>(result);
            }
            case NodeType::BinOp:
            {
                auto* ast_left = node->bin_op.left;
                auto* ast_right = node->bin_op.right;
                auto binary_op_type = node->bin_op.op;
                assert(ast_left);
                assert(ast_right);

                if (binary_op_type == BinOp::Assign)
                {
                    assert(ast_left->type == NodeType::VarExpr);
                    auto* var_decl = ast_left->var_expr.mentioned;
                    assert(var_decl);
                    auto* alloca_value = var_decl->var_decl.backend_ref;
                    auto* var_type = var_decl->var_decl.type;

                    auto* right_value = do_node(allocator, builder, ast_right, var_type);
                    assert(right_value);
                    builder.create_store(right_value, reinterpret_cast<Value*>(alloca_value));
                }
                else
                {
                    auto* left = do_node(allocator, builder, ast_left);
                    auto* right = do_node(allocator, builder, ast_right);
                    assert(left);
                    assert(right);

                    Instruction* binary_op_instruction;
                    switch (binary_op_type)
                    {
                        case BinOp::Cmp_LessThan:
                        {
                            binary_op_instruction = builder.create_icmp(CmpType::ICMP_SLT, left, right);
                        } break;
                        case BinOp::Cmp_GreaterThan:
                        {
                            binary_op_instruction = builder.create_icmp(CmpType::ICMP_SGT, left, right);
                        } break;
                        case BinOp::Cmp_Equal:
                        {
                            binary_op_instruction = builder.create_icmp(CmpType::ICMP_EQ, left, right);
                        } break;
                        case BinOp::Plus:
                        {
                            binary_op_instruction = builder.create_add(left, right);
                        } break;
                        case BinOp::Minus:
                        {
                            binary_op_instruction = builder.create_sub(left, right);
                        } break;
                        case BinOp::Mul:
                        {
                            binary_op_instruction = builder.create_mul(left, right);
                        } break;
                        default:
                            RNS_NOT_IMPLEMENTED;
                            break;
                    }

                    return reinterpret_cast<Value*>(binary_op_instruction);
                }
            } break;
            case NodeType::VarExpr:
            {
                auto* var_decl = node->var_expr.mentioned;
                assert(var_decl);
                auto* alloca_ptr = var_decl->var_decl.backend_ref;
                assert(alloca_ptr);
                Instruction* var_alloca = reinterpret_cast<Instruction*>(alloca_ptr);

                auto* type = var_decl->var_decl.type;
                assert(type);
                return reinterpret_cast<Value*>(builder.create_load(type, reinterpret_cast<Value*>(var_alloca)));
            } break;
            case NodeType::Ret:
            {
                // @TODO: deal with conditional alloca
                auto* ast_return_expression = node->ret.expr;
                // @TODO: void
                assert(ast_return_expression);
                // @TODO: check type
                auto* ret_value = do_node(allocator, builder, ast_return_expression);
                builder.create_ret(ret_value);
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return nullptr;
    }

    void encode(Compiler& compiler, NodeBuffer& node_buffer, FunctionTypeBuffer& function_type_declarations, FunctionDeclarationBuffer& function_declarations)
    {
        RNS_PROFILE_FUNCTION();
        compiler.subsystem = Compiler::Subsystem::IR;
        auto llvm_allocator = create_suballocator(&compiler.page_allocator, RNS_MEGABYTE(50));
        assert(function_declarations.len == 1);
        IDManager::init(&llvm_allocator, 1024, 1024 * 16);
        Module module = module.create(&llvm_allocator);
        BasicBlockBuffer basic_block_buffer = basic_block_buffer.create(&llvm_allocator, 1024);
        InstructionBuffer instruction_buffer = instruction_buffer.create(&llvm_allocator, 1024 * 16);

        for (auto& ast_current_function : function_declarations)
        {
            Builder builder = {};
            builder.basic_block_buffer = &basic_block_buffer;
            builder.instruction_buffer = &instruction_buffer;
            auto* function = Function::create(&module, &ast_current_function->function.type->type_expr, "main");
            builder.function = function;

            auto* ast_main_scope = ast_current_function->function.scope_blocks[0];
            auto& ast_main_scope_statements = ast_main_scope->block.statements;
            builder.function->basic_blocks = builder.function->basic_blocks.create(&llvm_allocator, 128);

            BasicBlock* entry_block = builder.create_block(&llvm_allocator);
            builder.append_to_function(entry_block);
            builder.set_block(entry_block);

            auto arg_count = builder.function->arg_count;


            auto ret_type = builder.function->value.type->function_t.ret_type;
            bool conditional_alloca = ret_type->id != TypeID::VoidType;
            if (conditional_alloca)
            {
                conditional_alloca = false;

                for (auto* st_node : ast_main_scope_statements)
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
                    // @Warning: here we need to comtemplate other cases which imply new blocks
                }
            }

            //llvm_function.arg_count = arg_count;
            //llvm_function.next_index = llvm_function.arg_count + 1;
            //llvm_function.conditional_alloca = conditional_alloca;

            auto alloca_count = arg_count + ast_current_function->function.variables.len + conditional_alloca;
            //Buffer<InstructionInfo*> alloca_buffer = {};
            if (alloca_count > 0)
            {
                //alloca_buffer = Buffer<InstructionInfo*>::create(&llvm_allocator, alloca_count);
            }

            // @TODO: abstract this away
            {
                if (conditional_alloca)
                {
                    builder.create_alloca(ret_type);
                }

                // Arguments
                for (auto* arg_node : ast_current_function->function.arguments)
                {
                    assert(arg_node->type == NodeType::VarDecl);
                    auto* arg_type = arg_node->var_decl.type;
                    assert(arg_type);
                    builder.create_alloca(arg_type);
                    // @TODO: store?
                }
            }

            for (auto* statement_node : ast_main_scope_statements)
            {
                do_node(&llvm_allocator, builder, statement_node);
            }

            SlotTracker slot_tracker = SlotTracker::create(&llvm_allocator, 2048);
            // @TODO: change hardcoding
            slot_tracker.next_id = slot_tracker.starting_id = 1;

            // print function:
            for (auto* block : function->basic_blocks)
            {
                block->print(slot_tracker);
                for (auto* instruction : block->instructions)
                {
                    instruction->print(slot_tracker);
                }
            }
        }
    }
}
