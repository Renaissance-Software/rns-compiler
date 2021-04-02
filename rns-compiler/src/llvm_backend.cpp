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

    }

    llvm::FunctionType* get_fn_type(Allocator* allocator, LLVMContext& context, Typing::Type* fn_type)
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
                return llvm::ConstantInt::get(get_type(llvm.context, suggested_type), APInt(32, int_lit, true));
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

    }

    void encode_fn(Allocator* allocator, LLVMBuilder llvm, llvm::Module& module, FunctionDeclaration& ast_function, NodeBuffer& node_buffer)
    {
        const auto* fn_name = "foo_function";

        auto* function_type = get_fn_type(allocator, llvm.context, ast_function.type->type_expr);
        assert(function_type);

        auto function = llvm::Function::Create(function_type, GlobalValue::LinkageTypes::ExternalLinkage, fn_name, module);
        assert(function);

        encode_fn_body(llvm, function, ast_function);
        BasicBlock* fn_bb = BasicBlock::Create(llvm.context, "fn_bb");
        llvm.builder.SetInsertPoint(fn_bb);

        for (auto* st_node : ast_function.scope_blocks[0]->block.statements)
        {
            process_statement(llvm, function, ast_function, st_node);
        }
    }

    void encode(Compiler& compiler, AST::Result* parser_result)
    {
        LLVMContext context;
        Allocator allocator = create_suballocator(&compiler.page_allocator, RNS_MEGABYTE(200));

        Module* module = new Module("new_module", context);
        IRBuilder<> builder(context);

        auto fn_declarations = parser_result->function_declarations;
        auto fn_type_declarations = parser_result->function_type_declarations;
        auto node_buffer = parser_result->node_buffer;

        LLVMBuilder llvm = { context, builder };

        for (auto& fn : fn_declarations)
        {
            encode_fn(&allocator, llvm, *module, fn->function, node_buffer);
        }
    }
}
