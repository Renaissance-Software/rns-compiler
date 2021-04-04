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
    };

    struct SlotTracker;
    struct Value
    {
        // @TODO: undo this mess
        Type* type;
        TypeID typeID;
        RNS::String name;

        static Value New(TypeID type_ID, String name = {}, Type* type = nullptr)
        {
            Value e = {
                .type = type,
                .typeID = type_ID,
                .name = name,
            };

            return e;
        }

        const char* print(char* buffer, SlotTracker& slot_tracker);
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
                .e = Value::New(TypeID::IntegerType, {}, integer_type),
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

        const char* to_string()
        {
            switch (type)
            {
                case CmpType::ICMP_EQ:
                    return "eq";
                case CmpType::ICMP_NE:
                    return "ne";
                case CmpType::ICMP_SLT:
                    return "slt";
                case CmpType::ICMP_SGT:
                    return "sgt";
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }
            return nullptr;
        }
    };

    const char* type_to_string(Type* type, char* buffer)
    {
        assert(type);
        switch (type->id)
        {
            case TypeID::PointerType:
            {
                char aux_buffer[64];
                sprintf(buffer, "%s*", type_to_string(type->pointer_t.appointee, aux_buffer));
            } break;
            case TypeID::IntegerType:
            {
                auto bits = type->integer_t.bits;
                sprintf(buffer, "i%u", bits);
            } break;
            case TypeID::VoidType:
            {
                sprintf(buffer, "void");
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
        return (const char*)buffer;
    }

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
        u64 id1;
        u64 id2;
        u64 id3;

        union
        {
            AllocaInstruction alloca_i;
            GetElementPtrInstruction get_element_ptr;
            Compare compare;
        };

        void print(SlotTracker& slot_tracker)
        {
            char operand0[64] = {};
            char operand1[64] = {};
            char operand2[64] = {};
            char type_buffer[64];

            printf("\t");
            switch (base.id)
            {
                case InstructionID::Alloca:
                {
                    printf("%%%llu = alloca %s, align 4", id1, type_to_string(alloca_i.allocated_type->pointer_t.appointee, type_buffer));
                } break;
                case InstructionID::Store:
                {
                    printf("store i32 %s, i32* %s, align 4", operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Br:
                {
                    if (operand_count == 1)
                    {
                        printf("br label %s", operands[0]->print(operand0, slot_tracker));
                    }
                    else
                    {
                        printf("br i1 %s, label %s, label %s", operands[2]->print(operand2, slot_tracker), operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                    }
                } break;
                case InstructionID::Load:
                {
                    printf("%%%llu = load i32, i32* %s, align 4", id3, operands[0]->print(operand0, slot_tracker));
                } break;
                case InstructionID::ICmp:
                {
                    printf("%%%llu = icmp %s i32 %s, %s", id3, compare.to_string(), operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Add:
                {
                    printf("%%%llu = add i32 %s, %s, align 4", id3, operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Sub:
                {
                    printf("%%%llu = sub i32 %s, %s, align 4", id3, operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Mul:
                {
                    printf("%%%llu = mul i32 %s, %s, align 4", id3, operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Ret:
                {
                    if (operands[0])
                    {
                        printf("ret i32 %s", operands[0]->print(operand0, slot_tracker));
                    }
                    else
                    {
                        printf("ret void");
                    }
                } break;
                case InstructionID::Call:
                {
                    Value* callee = operands[0];
                    assert(callee->typeID == TypeID::FunctionType);
                    assert(callee->type);
                    auto ret_type_not_void = callee->type->function_t.ret_type->id != TypeID::VoidType;
                    if (ret_type_not_void)
                    {
                        printf("%%%llu = ", id1);
                    }
                    printf("call i32 @%s(", callee->name.ptr);

                    auto arg_count = operand_count - 1;
                    if (arg_count)
                    {
                        auto print_arg = [](Value* operand)
                        {
                            char type_buffer[64];

                            if (operand->typeID == TypeID::LabelType)
                            {
                                // @MaybeBuggy Here are interpreting this operand is a load, which might be not true
                                auto* instr = reinterpret_cast<Instruction*>(operand);
                                assert(instr);
                                switch (instr->base.id)
                                {
                                    case InstructionID::Alloca:
                                    {
                                        printf("%s %%%llu", type_to_string(operand->type, type_buffer), reinterpret_cast<Instruction*>(operand)->id1);
                                    } break;
                                    case InstructionID::Load:
                                    {
                                        printf("%s %%%llu", type_to_string(operand->type, type_buffer), reinterpret_cast<Instruction*>(operand)->id3);
                                    } break;
                                    default:
                                        RNS_NOT_IMPLEMENTED;
                                        break;
                                }
                            }
                            else
                            {
                                RNS_NOT_IMPLEMENTED;
                            }
                        };

                        for (auto i = 1; i < arg_count; i++)
                        {
                            auto* operand = operands[i];
                            print_arg(operand);
                            printf(", ");
                        }
                        auto* operand = operands[arg_count];
                        print_arg(operand);
                    }

                    printf(")");
                } break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            printf("\n");
        }

        void get_info(SlotTracker& slot_tracker)
        {
            switch (base.id)
            {
                case InstructionID::Alloca:
                {
                    id1 = slot_tracker.new_id(this);
                } break;
                case InstructionID::Load:
                {
                    id3 = slot_tracker.new_id(this);
                } break;
                case InstructionID::ICmp:
                {
                    id3 = slot_tracker.new_id(this);
                } break;
                case InstructionID::Mul: case InstructionID::Add: case InstructionID::Sub:
                {
                    id3 = slot_tracker.new_id(this);
                } break;
                case InstructionID::Call:
                {
                    id1 = slot_tracker.new_id(this);
                } break;
                case InstructionID::Store:
                case InstructionID::Br:
                case InstructionID::Ret:
                {
                } break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }
        }
    };

    const char* Value::print(char* buffer, SlotTracker& slot_tracker)
    {
        // @Info: we shouldn't be null here. Void values are treated in the return printing case.
        assert(this);
        switch (this->typeID)
        {
            case TypeID::IntegerType:
            {
                auto* integer_value = (APInt*)this;
                sprintf(buffer, "%llu", integer_value->value.eight_byte);
            } break;
            case TypeID::LabelType:
            {
                auto id = slot_tracker.find(this);
                sprintf(buffer, "%%%llu", id);
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return (const char*)buffer;
    }

    struct Argument
    {
        Value value;
        s64 arg_index;
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

        // @TODO: do typechecking
        Function* find_function(String name, Type* type = nullptr);
    };

    struct Function
    {
        Value value;
        Buffer<BasicBlock*> basic_blocks;
        Argument* arguments;
        s64 arg_count;
        Module* parent;
        // @TODO: symbol table

        static Function* create(Module* module, Type* function_type, /* @TODO: linkage*/ String name)
        {
            auto* function = module->functions.allocate();
            *function = {
                .value = Value::New(TypeID::FunctionType, name, function_type),
                .parent = module,
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

        void print(Allocator* allocator);
    };

    inline Function* Module::find_function(String name, Type* type)
    {
        assert(functions.len);
        for (auto& function : functions)
        {
            if (name.equal(function.value.name))
            {
                return &function;
            }
        }

        return nullptr;
    }

    struct BasicBlock
    {
        Value value;
        Function* parent;
        Buffer<Instruction*> instructions;
        u32 use_count;

        u64 id;

        static BasicBlock create(String name = {})
        {
            // @TODO: consider dealing with inserting the block in the middle of the array
            BasicBlock new_basic_block = {
                .value = Value::New(TypeID::LabelType, name, Type::get_label()),
            };

            return new_basic_block;
        }
        void get_id(SlotTracker& slot_tracker)
        {
            if (parent->basic_blocks[0] != this)
            {
                id = slot_tracker.new_id(this);
            }
        }

        void print(SlotTracker& slot_tracker)
        {
            // @Info: Don't print function's main scope
            if (parent->basic_blocks[0] != this)
            {
                printf("%llu:\n", id);
            }
        }
    };

    void Function::print(Allocator* allocator)
    {
        SlotTracker slot_tracker = SlotTracker::create(allocator, 2048);
        // @TODO: change hardcoding
        assert(slot_tracker.starting_id == 0);
        assert(slot_tracker.next_id == 0);

        for (auto i = 0; i < arg_count; i++)
        {
            slot_tracker.new_id(&arguments[i]);
        }
        slot_tracker.new_id(nullptr);

        for (auto* block : basic_blocks)
        {
            block->get_id(slot_tracker);
            for (auto* instruction : block->instructions)
            {
                instruction->get_info(slot_tracker);
            }
        }

        for (auto* block : basic_blocks)
        {
            for (auto* instruction : block->instructions)
            {
                if (instruction->base.id == InstructionID::Br)
                {
                    instruction->get_info(slot_tracker);
                }
            }
        }

        char ret_type_buffer[64];
        printf("\ndefine dso_local %s @%s(", type_to_string(this->value.type->function_t.ret_type, ret_type_buffer), value.name.ptr);

        // Argument printing
        if (arg_count)
        {
            auto buffer_len = 0;
            auto last_index = arg_count - 1;
            char type_buffer[64];
            for (auto i = 0; i < last_index; i++)
            {
                auto* type = arguments[i].value.type;
                assert(type);
                printf("%s %%%lld, ", type_to_string(type, type_buffer), arguments[i].arg_index);
            }
            auto* type = arguments[last_index].value.type;
            assert(type);
            printf("%s %%%lld", type_to_string(type, type_buffer), arguments[last_index].arg_index);
        }
        printf(")\n{\n");

        // @Info: actual printing
        for (auto* block : basic_blocks)
        {
            block->print(slot_tracker);
            for (auto* instruction : block->instructions)
            {
                instruction->print(slot_tracker);
            }
        }
        printf("}\n");
    }

    struct Builder
    {
        BasicBlock* current;
        Function* function;
        /// <summary> Guarantee pointer stability for members
        BasicBlockBuffer* basic_block_buffer;
        InstructionBuffer* instruction_buffer;
        Module* module;
        /// </summary>
        s64 next_alloca_index;
        // @TODO: remove from here? Should this be userspace?
        /**/
        Instruction* return_alloca;
        BasicBlock* exit_block;
        bool conditional_alloca;
        // @TODO: this should be taken into account in every branch statement
        bool emitted_return;
        bool explicit_return;
        /**/

        // @TODO: guarantee pointer stability
        Instruction* create_alloca(Type* type, Value* array_size = nullptr, const char* name = nullptr)
        {
            Instruction i = {
                .base {
                    .value = Value::New(TypeID::LabelType),
                    .id = InstructionID::Alloca,
                },
                .alloca_i = {
                    .allocated_type = Type::get_pointer_type(type),
                }
            };

            return insert_alloca(i);
        }

        Instruction* create_store(Value* value, Value* ptr, bool is_volatile = false)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(TypeID::VoidType),
                    .id = InstructionID::Store,
                },
                .operands = { value, ptr },
                .operand_count = 2,
            };
            assert((u32)value->typeID);

            return insert_at_end(i);
        }

        Instruction* create_load(Type* type, Value* value, bool is_volatile = false, String name = {})
        {
            Instruction i = {
                .base = {
                    .value = Value::New(TypeID::LabelType, name, type),
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

        BasicBlock* create_block(Allocator* allocator, String name = {})
        {
            BasicBlock* basic_block = basic_block_buffer->append(BasicBlock::create(name));
            // @TODO: this should change
            basic_block->instructions = basic_block->instructions.create(allocator, 64);
            return basic_block;
        }

        Instruction* create_br(BasicBlock* dst_block)
        {
            assert(dst_block);
            if (!is_terminated())
            {
                Instruction i = {
                    .base = {
                        .value = Value::New(TypeID::VoidType),
                        .id = InstructionID::Br,
                    },
                    .operands = { reinterpret_cast<Value*>(dst_block) },
                    .operand_count = 1,
                };

                dst_block->use_count++;
                return insert_at_end(i);
            }
            return nullptr;
        }

        Instruction* create_conditional_br(BasicBlock* true_block, BasicBlock* else_block, Value* condition)
        {
            if (!is_terminated())
            {
                Instruction i = {
                    .base = {
                        .value = Value::New(TypeID::VoidType),
                        .id = InstructionID::Br,
                    },
                    .operands = { reinterpret_cast<Value*>(true_block), reinterpret_cast<Value*>(else_block), condition },
                    .operand_count = 3,
                };
                true_block->use_count++;
                else_block->use_count++;

                return insert_at_end(i);
            }
            return nullptr;
        }

        Instruction* create_icmp(CmpType compare_type, Value* left, Value* right, const char* name = nullptr)
        {
            Instruction i = {
                .base = {
                    .value = Value::New(TypeID::LabelType),
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
                    .value = Value::New(TypeID::LabelType),
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
                    .value = Value::New(TypeID::LabelType),
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
                    .value = Value::New(TypeID::LabelType),
                    .id = InstructionID::Mul,
                },
                .operands = { left, right },
                .operand_count = 2,
            };

            return insert_at_end(i);
        }

        Instruction* create_ret(Value* value)
        {
            if (!is_terminated())
            {

                Instruction i = {
                    .base = {
                        .value = Value::New(TypeID::VoidType),
                        .id = InstructionID::Ret,
                    },
                    .operands = { value },
                    .operand_count = 1,
                };

                return insert_at_end(i);
            }
            return nullptr;
        }

        Instruction* create_ret_void()
        {
            return create_ret(nullptr);
        }

        Instruction* create_call(Value* callee, Slice<Value*> arguments = {})
        {
            Instruction i = {
                .base = {
                    .value = Value::New(TypeID::LabelType, {}, callee->type),
                    .id = InstructionID::Call,
                },
            };

            assert(arguments.len <= rns_array_length(i.operands) - 1);

            i.operand_count = arguments.len + 1;
            i.operands[0] = callee;

            for (auto index = 0; index < arguments.len; index++)
            {
                i.operands[index + 1] = arguments[index];
            }

            return insert_at_end(i);
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

        inline bool is_terminated()
        {
            assert(current);
            assert(current->instructions.cap);
            if (current->instructions.len)
            {
                auto& last_instruction = current->instructions[current->instructions.len - 1];
                switch (last_instruction->base.id)
                {
                    case InstructionID::Br: case InstructionID::Ret:
                        return true;
                    default:
                        return false;
                }
            }
            else
            {
                return false;
            }
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
                for (auto* st_node : node->block.statements)
                {
                    if (!builder.emitted_return)
                    {
                        do_node(allocator, builder, st_node);
                    }
                }
            } break;
            case NodeType::Conditional:
            {
                bool saved_emitted_return = builder.emitted_return;
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

                builder.emitted_return = false;
                builder.append_to_function(if_block);
                builder.set_block(if_block);
                do_node(allocator, builder, ast_if_block);
                bool if_block_returned = builder.emitted_return;

                builder.create_br(exit_block);

                builder.emitted_return = false;
                if (else_block != exit_block)
                {
                    builder.append_to_function(else_block);
                    builder.set_block(else_block);
                    do_node(allocator, builder, ast_else_block);

                    builder.create_br(exit_block);
                }
                bool else_block_returned = builder.emitted_return;

                builder.emitted_return = if_block_returned && else_block_returned;

                if (exit_block_in_use && !builder.emitted_return)
                {
                    builder.append_to_function(exit_block);
                    builder.set_block(exit_block);
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
                builder.set_block(loop_prefix_block);

                assert(ast_loop_prefix->block.statements.len == 1);
                auto* ast_condition = ast_loop_prefix->block.statements[0];
                auto* condition = do_node(allocator, builder, ast_condition, Type::get_bool_type());
                assert(condition);

                builder.create_conditional_br(loop_body_block, loop_end_block, condition);
                builder.append_to_function(loop_body_block);
                builder.set_block(loop_body_block);

                do_node(allocator, builder, ast_loop_body);

                builder.create_br(loop_postfix_block);

                builder.append_to_function(loop_postfix_block);
                builder.set_block(loop_postfix_block);
                do_node(allocator, builder, ast_loop_postfix);

                if (!builder.emitted_return)
                {
                    builder.create_br(loop_prefix_block);
                    builder.append_to_function(loop_end_block);
                    builder.set_block(loop_end_block);
                }
            } break;
            case NodeType::Break:
            {
                auto* ast_jump_target = node->break_.target;
                assert(ast_jump_target->type == NodeType::Loop);
                auto* jump_target = reinterpret_cast<BasicBlock*>(ast_jump_target->loop.exit_block);
                assert(jump_target);

                // @TODO: this may be buggy
#if 0
                emit_jump(allocator, builder, jump_target);
#else
                builder.create_br(jump_target);
#endif
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
                    switch (ast_left->type)
                    {
                        case NodeType::VarExpr:
                        {
                            auto* var_decl = ast_left->var_expr.mentioned;
                            assert(var_decl);
                            auto* alloca_value = var_decl->var_decl.backend_ref;
                            auto* var_type = var_decl->var_decl.type;

                            auto* right_value = do_node(allocator, builder, ast_right, var_type);
                            assert(right_value);
                            builder.create_store(right_value, reinterpret_cast<Value*>(alloca_value));
                        } break;
                        case NodeType::UnaryOp:
                        {
                            assert(ast_left->unary_op.type == UnaryOp::PointerDereference);

                            auto* right_value = do_node(allocator, builder, ast_right);
                            assert(right_value);

                            auto* pointer_load = do_node(allocator, builder, ast_left);
                            builder.create_store(right_value, pointer_load);
                        } break;
                        default:
                            RNS_UNREACHABLE;
                    }
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
                // @TODO: tolerate this in the future?
                assert(!builder.emitted_return);
                auto* ast_return_expression = node->ret.expr;
                if (ast_return_expression)
                {
                    builder.emitted_return = true;
                    builder.explicit_return = true;

                    assert(ast_return_expression);
                    auto* ret_value = do_node(allocator, builder, ast_return_expression);

                    if (builder.conditional_alloca)
                    {
                        assert(builder.return_alloca);
                        assert(builder.exit_block);
                        builder.create_store(ret_value, reinterpret_cast<Value*>(builder.return_alloca));
                        builder.create_br(builder.exit_block);
                    }
                    else
                    {
                        builder.create_ret(ret_value);
                    }
                }
                else
                {
                    if (!builder.explicit_return)
                    {
                        builder.create_ret_void();
                    }
                    else
                    {
                        builder.create_br(builder.exit_block);
                        builder.emitted_return = true;
                        builder.explicit_return = true;
                    }
                }
            } break;
            case NodeType::InvokeExpr:
            {
                auto* invoke_expr = node->invoke_expr.expr;
                assert(invoke_expr);
                assert(invoke_expr->type == NodeType::Function);
                auto function_name = invoke_expr->function.name;
                // @TODO: do typechecking
                auto* function = builder.module->find_function(function_name);
                assert(function);

                auto arg_count = node->invoke_expr.arguments.len;
                if (arg_count == 0)
                {
                    auto* call = builder.create_call(reinterpret_cast<Value*>(function));
                    return reinterpret_cast<Value*>(call);
                }
                else
                {
                    Value* argument_buffer[255];
                    assert(arg_count <= 15);
                    auto& node_arg_buffer = node->invoke_expr.arguments;
                    auto arg_i = 0;
                    auto* function_type = function->value.type;
                    assert(function_type);
                    for (auto* arg_node : node_arg_buffer)
                    {
                        auto* arg = do_node(allocator, builder, arg_node);
                        assert(arg);
                        // @TODO: this may be buggy
                        arg->type = function_type->function_t.arg_types[arg_i];
                        argument_buffer[arg_i++] = arg;
                    }

                    auto* call = builder.create_call(reinterpret_cast<Value*>(function), { argument_buffer, arg_count });
                    return reinterpret_cast<Value*>(call);
                }
            } break;
            case NodeType::UnaryOp:
            {
                assert(node->unary_op.pos == UnaryOpType::Prefix);
                auto unary_op_type = node->unary_op.type;
                auto* unary_op_expr = node->unary_op.node;
                assert(unary_op_expr);

                switch (unary_op_type)
                {
                    case UnaryOp::AddressOf:
                    {
                        assert(unary_op_expr->type == NodeType::VarExpr);
                        auto* ref_var_decl = unary_op_expr->var_expr.mentioned;
                        assert(ref_var_decl);
                        assert(ref_var_decl->type == NodeType::VarDecl);
                        auto* ref_var_decl_type = ref_var_decl->var_decl.type;
                        assert(ref_var_decl_type);
                        auto* var_alloca = reinterpret_cast<Value*>(ref_var_decl->var_decl.backend_ref);
                        assert(var_alloca);
                        return var_alloca;
                    } break;
                    case UnaryOp::PointerDereference:
                    {
                        if (node->value_type == ValueType::LValue)
                        {
                            assert(unary_op_expr->type == NodeType::VarExpr);
                            auto* pointer_to_dereference_decl = unary_op_expr->var_expr.mentioned;
                            assert(pointer_to_dereference_decl);
                            assert(pointer_to_dereference_decl->type == NodeType::VarDecl);
                            auto* pointer_type = pointer_to_dereference_decl->var_decl.type;
                            assert(pointer_type);
                            assert(pointer_type->id == TypeID::PointerType);
                            auto* pointer_alloca = reinterpret_cast<Value*>(pointer_to_dereference_decl->var_decl.backend_ref);
                            assert(pointer_alloca);
                            auto* pointer_load = builder.create_load(pointer_type, pointer_alloca);
                            return reinterpret_cast<Value*>(pointer_load);
                        }
                        else
                        {
                            assert(unary_op_expr->type == NodeType::VarExpr);
                            auto* pointer_to_dereference_decl = unary_op_expr->var_expr.mentioned;
                            assert(pointer_to_dereference_decl);
                            assert(pointer_to_dereference_decl->type == NodeType::VarDecl);
                            auto* pointer_type = pointer_to_dereference_decl->var_decl.type;
                            assert(pointer_type);
                            assert(pointer_type->id == TypeID::PointerType);
                            auto* pointer_alloca = reinterpret_cast<Value*>(pointer_to_dereference_decl->var_decl.backend_ref);
                            assert(pointer_alloca);
                            auto* pointer_load = builder.create_load(pointer_type, pointer_alloca);
                            auto* deref_type = pointer_type->pointer_t.appointee;
                            assert(deref_type);
                            auto* deref_expr = builder.create_load(deref_type, reinterpret_cast<Value*>(pointer_load));
                            return reinterpret_cast<Value*>(deref_expr);
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

        return nullptr;
    }

    void encode(Compiler& compiler, NodeBuffer& node_buffer, FunctionTypeBuffer& function_type_declarations, FunctionDeclarationBuffer& function_declarations)
    {
        RNS_PROFILE_FUNCTION();
        compiler.subsystem = Compiler::Subsystem::IR;
        auto llvm_allocator = create_suballocator(&compiler.page_allocator, RNS_MEGABYTE(50));
        Module module = module.create(&llvm_allocator);
        BasicBlockBuffer basic_block_buffer = basic_block_buffer.create(&llvm_allocator, 1024);
        InstructionBuffer instruction_buffer = instruction_buffer.create(&llvm_allocator, 1024 * 16);
        module.functions = module.functions.create(&llvm_allocator, function_declarations.len);

        for (auto& ast_current_function : function_declarations)
        {
            auto* function_type = &ast_current_function->function.type->type_expr;
            assert(function_type->id == TypeID::FunctionType);
            Function::create(&module, function_type, ast_current_function->function.name);
        }

        for (auto i = 0; i < function_declarations.len; i++)
        {
            auto& ast_current_function = function_declarations[i];
            auto* function = &module.functions[i];
            Builder builder = {};
            builder.basic_block_buffer = &basic_block_buffer;
            builder.instruction_buffer = &instruction_buffer;
            builder.function = function;
            builder.module = &module;

            auto* ast_main_scope = ast_current_function->function.scope_blocks[0];
            auto& ast_main_scope_statements = ast_main_scope->block.statements;
            builder.function->basic_blocks = builder.function->basic_blocks.create(&llvm_allocator, 128);

            BasicBlock* entry_block = builder.create_block(&llvm_allocator);
            builder.append_to_function(entry_block);
            builder.set_block(entry_block);

            function->arg_count = ast_current_function->function.arguments.len;
            auto ret_type = builder.function->value.type->function_t.ret_type;

            bool ret_type_void = ret_type->id == TypeID::VoidType;
            builder.explicit_return = false;

            for (auto* st_node : ast_main_scope_statements)
            {
                if (st_node->type == NodeType::Conditional)
                {
                    if (introspect_for_conditional_allocas(st_node->conditional.if_block))
                    {
                        builder.explicit_return = true;
                        break;
                    }
                    if (introspect_for_conditional_allocas(st_node->conditional.else_block))
                    {
                        builder.explicit_return = true;
                        break;
                    }
                }
                else if (st_node->type == NodeType::Loop)
                {
                    if (introspect_for_conditional_allocas(st_node->loop.body))
                    {
                        builder.explicit_return = true;
                        break;
                    }
                }
                // @Warning: here we need to comtemplate other cases which imply new blocks
            }
            builder.conditional_alloca = !ret_type_void && builder.explicit_return;

            //auto alloca_count = arg_count + ast_current_function->function.variables.len + conditional_alloca;

            // @TODO: reserve as many position as 'alloca_count' in the main basic block, displace the len by that offset and write the rest of instructions there.
            // Alloca can have their special insertion entry point
            // @WARNING: this would imply we could fail with non-optimized build dead code elimination

            if (builder.explicit_return)
            {
                builder.exit_block = builder.create_block(&llvm_allocator);
            }
            if (builder.conditional_alloca)
            {
                builder.return_alloca = builder.create_alloca(ret_type);
            }

            // Arguments
            if (function->arg_count)
            {
                function->arguments = new (&llvm_allocator) Argument[function->arg_count];
                assert(function->arguments);
                auto arg_index = 0;
                for (auto* arg_node : ast_current_function->function.arguments)
                {
                    assert(arg_node->type == NodeType::VarDecl);
                    auto* arg_type = arg_node->var_decl.type;
                    assert(arg_type);
                    assert(arg_node->var_decl.is_fn_arg);
                    auto arg_name = arg_node->var_decl.name;

                    auto* arg = &function->arguments[arg_index++];
                    *arg = {
                        .value = Value::New(TypeID::LabelType, arg_name, arg_type),
                        .arg_index = arg_index,
                    };

                    auto* arg_alloca = builder.create_alloca(arg_type);
                    arg_node->var_decl.backend_ref = arg_alloca;

                    builder.create_store(reinterpret_cast<Value*>(arg), reinterpret_cast<Value*>(arg_alloca));
                }
            }

            do_node(&llvm_allocator, builder, ast_main_scope);

            if (builder.conditional_alloca)
            {
                assert(builder.current->instructions.len != 0);

                builder.append_to_function(builder.exit_block);
                builder.set_block(builder.exit_block);

                auto* loaded_return = builder.create_load(builder.return_alloca->alloca_i.allocated_type, reinterpret_cast<Value*>(builder.return_alloca));
                assert(loaded_return);
                builder.create_ret(reinterpret_cast<Value*>(loaded_return));
            }
            else if (ret_type_void)
            {
                if (builder.explicit_return)
                {
                    if (builder.current->instructions.len == 0)
                    {
                        auto* saved_current = builder.set_block(builder.exit_block);
                        assert(saved_current);
                        auto index = builder.function->basic_blocks.get_id_if_ref_buffer(saved_current);
                        // @Info: this is a no-statements function.
                        // @TODO: not create a basic block if the function has no statements
                        assert(index == 0);
                        builder.function->basic_blocks[index] = builder.current;
                        builder.current->parent = builder.function;
                    }
                    else
                    {
                        builder.append_to_function(builder.exit_block);
                        builder.set_block(builder.exit_block);
                    }
                }

                builder.create_ret_void();
            }

            function->print(&llvm_allocator);
        }
    }
}
