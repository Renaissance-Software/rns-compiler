#include "llvm_backend.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

using namespace RNS;
using namespace AST;
using namespace llvm;

namespace LLVM
{
    struct LLVMBuilder
    {
        LLVMContext& context;
        IRBuilder<>& builder;
    };

    llvm::Type* get_type(LLVMContext& context, Typing::Type* type)
    {
        if (type == nullptr)
        {
            return nullptr;
        }

        switch (type->kind)
        {
            case TypeKind::Native:
            {
                switch (type->native_t)
                {
                    case NativeTypeID::S32:
                    {
                        return llvm::Type::getInt32Ty(context);
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

    llvm::FunctionType* get_fn_type(Allocator* allocator, LLVMContext& context, Typing::FunctionType* fn_type)
    {
        auto* ret_type = get_type(context, fn_type->ret_type);
        llvm::Type* llvm_type_args[256];

        int arg_count = 0;
        for (auto* type : fn_type->arg_types)
        {
            auto* arg_llvm_type = get_type(context, type);
            llvm_type_args[arg_count++] = arg_llvm_type;
        }
        auto arg_array = ArrayRef<llvm::Type*>(llvm_type_args, arg_count);

        auto* result = llvm::FunctionType::get(ret_type, arg_array, false);
        assert(result);
        return result;
    }

    Value* generate_expression(LLVMBuilder llvm, Node* node, Typing::Type* suggested_type = nullptr)
    {
        switch (node->type)
        {
            case NodeType::VarExpr:
            {
                RNS_NOT_IMPLEMENTED;
            } break;
            case NodeType::IntLit:
            {
                auto int_lit = node->int_lit.lit;
                assert(suggested_type->kind == TypeKind::Native);
                assert(suggested_type->native_t == NativeTypeID::S32);
                return ConstantInt::get(get_type(llvm.context, suggested_type), APInt(32, int_lit, true));
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    void process_statement(LLVMBuilder llvm, llvm::Function* llvm_function, FunctionDeclaration& function, Node* statement)
    {
        switch (statement->type)
        {
            case NodeType::VarDecl:
            {
                assert(!statement->var_decl.is_fn_arg);
                auto var_name_slice = statement->var_decl.name;
                auto* var_type = statement->var_decl.type;
                assert(var_type);
                auto* value = statement->var_decl.value;
                assert(value);
                auto* llvm_type = get_type(llvm.context, var_type);
                assert(llvm_type);
                auto* var_alloca = llvm.builder.CreateAlloca(llvm_type);
                assert(var_alloca);
            } break;
            case NodeType::Conditional:
            {
                Node* condition_node = statement->conditional.condition;
                assert(condition_node->type == NodeType::BinOp);
                auto* left_expr = generate_expression(llvm, condition_node->bin_op.left);

                switch (condition_node->bin_op.op)
                {
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }
            }
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    void encode_fn_body(LLVMBuilder llvm, llvm::Function* llvm_fn, FunctionDeclaration& function)
    {
        BasicBlock* fn_bb = BasicBlock::Create(llvm.context, "fn_bb", llvm_fn);
        llvm.builder.SetInsertPoint(fn_bb);

        for (auto* st_node : function.scope.statements)
        {
            process_statement(llvm, llvm_fn, function, st_node);
        }

    }

    void encode_fn(Allocator* allocator, LLVMBuilder llvm, llvm::Module& module, FunctionDeclaration& function, NodeBuffer& node_buffer)
    {
        const auto* fn_name = "foo_function";

        auto* function_type = get_fn_type(allocator, llvm.context, function.type);
        assert(function_type);

        auto llvm_function = llvm::Function::Create(function_type, GlobalValue::LinkageTypes::ExternalLinkage, fn_name, module);
        assert(llvm_function);

        encode_fn_body(llvm, llvm_function, function);
    }

    void encode(RNS::Allocator* allocator, AST::Result* parser_result)
    {
        LLVMContext context;

        Module* module = new Module("new_module", context);
        IRBuilder<> builder(context);

        auto fn_declarations = parser_result->function_declarations;
        auto fn_type_declarations = parser_result->function_type_declarations;
        auto type_declarations = parser_result->type_declarations;
        auto node_buffer = parser_result->node_buffer;

        LLVMBuilder llvm = { context, builder };

        for (auto& fn : fn_declarations)
        {
            encode_fn(allocator, llvm, *module, fn, node_buffer);
        }
    }
}
