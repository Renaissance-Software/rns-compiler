#include "llvm_bytecode.h"

#include <llvm/Bitcode/LLVMBitCodes.h>
#include "typechecker.h"

namespace LLVM
{
    using namespace RNS;

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

    struct InstructionInfo
    {
        Instruction id;
        union
        {
            Ret ret;
        };
    };

    struct Value
    {
        ValueID id;
        union
        {
            Constant constant;
        };
    };

    bool typecheck(Type* type, LLVM::Value* value)
    {
        return false;
    }

    Value* node_to_bytecode_value(Allocator* allocator, NodeBuffer& node_buffer, TypeBuffer& type_declarations, FunctionDeclaration& current_function, Node* node)
    {
        switch (node->type)
        {
            case NodeType::IntLit:
            {
                Value* value = new(allocator) Value;
                *value = {
                    .constant = {
                        .id = ConstantID::Int,
                        .integer = node->int_lit,
                    }
                };
                return value;
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return nullptr;
    }

    void do_statement_node(Allocator* allocator, Buffer<InstructionInfo>& llvm_instructions, NodeBuffer& node_buffer, TypeBuffer& type_declarations, FunctionDeclaration& current_function, Node* node)
    {
        InstructionInfo instruction;
        switch (node->type)
        {
            case NodeType::VarDecl:
            {
                assert(!node->var_decl.is_fn_arg);
                auto var_name = node->var_decl.name;
                auto* var_type = node->var_decl.type;
                auto* var_value_node = node->var_decl.value;
                assert(var_type);
                assert(var_value_node);
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
                    auto* ret_value = node_to_bytecode_value(allocator, node_buffer, type_declarations, current_function, ret_expr);
                    instruction.ret = {
                        .type = nullptr,
                        .value = ret_value,
                    };
                    llvm_instructions.append(instruction);
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

    void encode(Allocator* allocator, NodeBuffer& node_buffer, FunctionTypeBuffer& function_type_declarations, TypeBuffer& type_declarations, FunctionDeclarationBuffer& function_declarations)
    {
        assert(function_declarations.len == 1);
        for (auto& function : function_declarations)
        {
            Buffer<InstructionInfo> ib = Buffer<InstructionInfo>::create(allocator, 1024);

            for (auto* st_node : function.scope.statements)
            {
                do_statement_node(allocator, ib, node_buffer, type_declarations, function, st_node);
            }
        }
    }
}