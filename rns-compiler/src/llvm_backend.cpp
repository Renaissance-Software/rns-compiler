#include "llvm_backend.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

using namespace Parser;
using namespace llvm;

void encode(LLVMContext& context, Module& module, IRBuilder<>& builder, Parser::ModuleParser* parser, Parser::Node* function)
{
    auto name_node_id = function->function_decl.name;
    auto* name_node = parser->nb.get(name_node_id);
    assert(name_node);
    assert(name_node->type == NodeType::SymName);
    auto* fn_name = name_node->sym_name.ptr;

    auto function_type = FunctionType::get(llvm::Type::getInt32Ty(context), false);
    auto fn = llvm::Function::Create(function_type, GlobalValue::LinkageTypes::ExternalLinkage, fn_name, module);
}
void LLVM::encode(Parser::ModuleParser* module_parser, RNS::Allocator* allocator)
{
    LLVMContext context;

    Module* module = new Module("new_module", context);
    IRBuilder<> builder(context);

    assert(module_parser->tldb.len == 1);
    for (s64 i = 0; i < module_parser->tldb.len; i++)
    {
        auto tld = module_parser->tldb.ptr[i];
        auto* tld_node = module_parser->nb.get(tld);
        assert(tld_node->type == Parser::NodeType::Function);
        ::encode(context, *module, builder, module_parser, tld_node);
    }
}
