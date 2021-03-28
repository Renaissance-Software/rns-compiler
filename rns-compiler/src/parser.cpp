#include "parser.h"

#include <RNS/profiler.h>
#include <stdio.h>

using namespace RNS;
using namespace AST;

namespace AST
{
    struct Parser
    {
        Token* ptr;
        s64 parser_it;
        s64 len;
        Allocator allocator;
        NodeBuffer nb;
        Compiler& compiler;

        FunctionDeclarationBuffer function_declarations;
        TypeBuffer type_declarations;
        StructBuffer struct_declarations;
        UnionBuffer union_declarations;
        EnumBuffer enum_declarations;
        FunctionTypeBuffer function_type_declarations;

        ScopeBlock* current_scope;
        FunctionDeclaration* current_function;

        inline Token* get_next_token(s64 i)
        {
            if (parser_it + i < len)
            {
                return &ptr[parser_it + i];
            }
            return nullptr;
        }

        inline Token* get_next_token()
        {
            return get_next_token(0);
        }

        template<typename T>
        inline Token* expect(T id)
        {
            Token* t = get_next_token();
            if (t)
            {
                if (t->id == static_cast<u8>(id))
                {
                    return t;
                }
            }
            return nullptr;
        }

        template<typename T>
        inline Token* expect_and_consume(T id)
        {
            Token* t = expect(id);
            if (t)
            {
                return consume();
            }
            return nullptr;
        }

        inline Token* expect_and_consume_if_keyword(KeywordID keyword)
        {
            auto* t = expect(TokenID::Keyword);
            if (t && t->keyword == keyword)
            {
                consume();
                return t;
            }
            return nullptr;
        }

        template<typename T>
        inline bool expect_and_consume_twice(T id1, T id2)
        {
            Token* t1 = get_next_token(0);
            Token* t2 = get_next_token(1);
            bool result = (t1 && t1->id == static_cast<u8>(id1) && t2 && t2->id == static_cast<u8>(id2));
            if (result)
            {
                consume(); consume();
            }
            return result;
        }


        inline Token* consume()
        {
            auto* result = &ptr[parser_it++];
            //printf("Consuming: ");
            //result->print_token_id();
            return result;
        }

        inline BinOp is_bin_op()
        {
            auto* first_t = get_next_token();
            auto token_id = static_cast<TokenID>(first_t->id);

            switch (token_id)
            {
                case TokenID::Colon:
                    consume();
                    if (expect_and_consume(':'))
                    {
                        if (expect('('))
                        {
                            return BinOp::Function;
                        }
                        else
                        {
                            RNS_UNREACHABLE;
                            return BinOp::None;
                        }
                    }
                    else
                    {
                        return BinOp::VariableDecl;
                    }
                    break;
                case TokenID::Equal:
                    consume();
                    if (expect_and_consume('='))
                    {
                        return BinOp::Cmp_Equal;
                    }
                    else
                    {
                        return BinOp::Assign;
                    }
                case TokenID::Plus:
                {
                    consume();
                    return BinOp::Plus;
                }
                case TokenID::Minus:
                {
                    consume();
                    return BinOp::Minus;
                }
                case TokenID::Asterisk:
                case TokenID::Slash:
                    // single char
                    RNS_NOT_IMPLEMENTED;
                    return BinOp::None;
                case TokenID::Exclamation:
                case TokenID::Greater:
                case TokenID::Less:
                    // two characters or more
                    RNS_NOT_IMPLEMENTED;
                    return BinOp::None;
                case TokenID::Keyword:
                    // and/or
                    RNS_NOT_IMPLEMENTED;
                    return BinOp::None;
                default:
                    return BinOp::None;
            }
        }

        Type* get_native_type(NativeTypeID native_type)
        {
            return this->type_declarations.get(static_cast<s64>(native_type));
        }

        Type* get_type(Token* t)
        {
            if (t->get_id() == TokenID::Type)
            {
                return t->type;
            }
            else if (t->get_id() == TokenID::Symbol)
            {
                RNS_NOT_IMPLEMENTED;
            }
            else
            {
                RNS_UNREACHABLE;
            }

            return nullptr;
        }

        Node* find_existing_variable(Token* token)
        {
            RNS::String name = { token->symbol, token->offset };
            for (auto var : current_function->variables)
            {
                assert(var->type == NodeType::VarDecl);
                if (var->var_decl.name.equal(&name))
                {
                    return var;
                }
            }

            return nullptr;
        }

        Node* parse_primary_expression()
        {
            auto* t = get_next_token();
            auto id = t->get_id();

            switch (id)
            {
                case TokenID::IntegerLit:
                {
                    this->consume();
                    Node* node = nb.append(NodeType::IntLit);
                    node->int_lit.lit = t->int_lit;
                    return node;
                }
                case TokenID::Symbol:
                {
                    consume();
                    auto* next_t = expect_and_consume(':');
                    if (next_t)
                    {
                        auto* var_decl_node = nb.append(NodeType::VarDecl);
                        var_decl_node->var_decl.name.ptr = t->symbol;
                        var_decl_node->var_decl.name.len = t->offset;
                        return var_decl_node;
                    }
                    else
                    {
                        auto* var_expr_node = nb.append(NodeType::VarExpr);
                        var_expr_node->var_expr.mentioned = find_existing_variable(t);
                        assert(var_expr_node->var_expr.mentioned);
                        return var_expr_node;
                    }
                } break;
                case TokenID::Minus:
                {
                    consume();
                    if (get_next_token(0)->get_id() == TokenID::Minus && get_next_token(1)->get_id() == TokenID::Minus)
                    {
                        return nullptr;
                    }
                    else
                    {
                        RNS_NOT_IMPLEMENTED;
                        return nullptr;
                    }
                }
                case TokenID::Semicolon:
                    return nullptr;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            return nullptr;
        }

        Node* parse_conditional()
        {
            consume();
            auto* node = nb.append(NodeType::Conditional);

            node->conditional.condition = parse_expression();
            assert(node->conditional.condition);

            bool braces = expect_and_consume('{');

            node->conditional.if_block = current_function->scope_blocks.allocate();
            auto* if_block = node->conditional.if_block;
            if_block->parent = current_scope;
            if_block->type = ScopeType::ConditionalBlock;
            current_scope = if_block;

            if (braces)
            {
                if_block->statements = if_block->statements.create(&allocator, 128);
                while (!expect_and_consume('}'))
                {
                    auto* st_node = parse_statement();
                    assert(st_node);
                    if_block->statements.append(st_node);
                }
            }
            else
            {
                if_block->statements = if_block->statements.create(&allocator, 1);
                auto* st_node = parse_statement();
                if_block->statements.append(st_node);
            }

            current_scope = static_cast<ScopeBlock*>(current_scope->parent);

            if (expect_and_consume_if_keyword(KeywordID::Else))
            {
                node->conditional.else_block = current_function->scope_blocks.allocate();
                auto* else_block = node->conditional.else_block;
                else_block->parent = current_scope;
                else_block->type = ScopeType::ConditionalBlock;
                current_scope = else_block;

                braces = expect_and_consume('{');
                if (braces)
                {
                    else_block->statements = else_block->statements.create(&allocator, 128);
                    while (!expect_and_consume('}'))
                    {
                        auto* st_node = parse_statement();
                        assert(st_node);
                        else_block->statements.append(st_node);
                    }
                }
                else
                {
                    else_block->statements = else_block->statements.create(&allocator, 1);
                    auto* st_node = parse_statement();
                    assert(st_node);
                    else_block->statements.append(st_node);
                }
                current_scope = static_cast<ScopeBlock*>(current_scope->parent);
            }

            return node;
        }

        Node* parse_for()
        {
            auto* for_t = expect_and_consume_if_keyword(KeywordID::For);
            Node* for_loop = nb.append(NodeType::Loop);
            auto* parent_scope = current_scope;
            for_loop->loop.prefix = this->current_function->scope_blocks.allocate();
            for_loop->loop.prefix->type = ScopeType::LoopPrefix;
            for_loop->loop.prefix->parent = parent_scope;
            for_loop->loop.body = this->current_function->scope_blocks.allocate();
            for_loop->loop.body->type = ScopeType::LoopBody;
            for_loop->loop.body->parent = parent_scope;
            for_loop->loop.postfix = this->current_function->scope_blocks.allocate();
            for_loop->loop.postfix->type = ScopeType::LoopPostfix;
            for_loop->loop.postfix->parent = parent_scope;
            assert(for_t);

            auto* it_symbol = expect_and_consume(TokenID::Symbol);
            Node* it_decl = nb.append(NodeType::VarDecl);
            it_decl->var_decl.is_fn_arg = false;
            it_decl->var_decl.name = RNS::String{ it_symbol->symbol, it_symbol->offset };
            it_decl->var_decl.scope = current_scope;
            it_decl->var_decl.type = &type_declarations[(u32)NativeTypeID::S32];
            // @TODO: we should match it to the right operand
            it_decl->var_decl.value = nb.append(NodeType::IntLit);
            it_decl->var_decl.value->int_lit.bit_count = 32;
            it_decl->var_decl.value->int_lit.is_signed = true;
            it_decl->var_decl.value->int_lit.lit = 0;
            this->current_function->variables.append(it_decl);

            {
                current_scope = for_loop->loop.prefix;
                auto* colon = expect_and_consume(':');
                assert(colon);
                auto* t = consume();
                auto token_id = t->get_id();
                Node* right_node = nullptr;
                switch (token_id)
                {
                    case TokenID::IntegerLit:
                    {
                        right_node = nb.append(NodeType::IntLit);
                        auto literal = t->int_lit;
                        assert(literal >= 0 && literal <= UINT32_MAX);
                        right_node->int_lit.bit_count = 32;
                        right_node->int_lit.is_signed = false;
                        right_node->int_lit.lit = literal;
                    } break;
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }

                assert(right_node);
                Node* it_expr = nb.append(NodeType::VarExpr);
                it_expr->var_expr.mentioned = it_decl;
                Node* cmp_op = nb.append(NodeType::BinOp);
                cmp_op->bin_op.op = BinOp::Cmp_LessThan;
                cmp_op->bin_op.left = it_expr;
                cmp_op->bin_op.right = right_node;
                Node* prefix_statements[] = { cmp_op };
                for_loop->loop.prefix->statements = for_loop->loop.prefix->statements.create(&allocator, rns_array_length(prefix_statements));
                for (auto* prefix_st : prefix_statements)
                {
                    for_loop->loop.prefix->statements.append(prefix_st);
                }
            }

            {
                current_scope = for_loop->loop.body;
                parse_block(current_scope, true);
            }

            {
                current_scope = for_loop->loop.postfix;
                current_scope->statements = current_scope->statements.create(&allocator, 1);
                Node* var_expr = nb.append(NodeType::VarExpr);
                var_expr->var_expr.mentioned = it_decl;
                Node* one_lit = nb.append(NodeType::IntLit);
                one_lit->int_lit.bit_count = 32;
                one_lit->int_lit.is_signed = false;
                one_lit->int_lit.lit = 1;
                Node* postfix_increment = nb.append(NodeType::BinOp);
                postfix_increment->bin_op.left = var_expr;
                postfix_increment->bin_op.op = BinOp::Plus;
                postfix_increment->bin_op.right = one_lit;
                current_scope->statements.append(postfix_increment);
            }
            current_scope = parent_scope;
            return for_loop;
        }

        Node* parse_statement()
        {
            auto* t = get_next_token();
            TokenID id = static_cast<TokenID>(t->id);
            Node* statement = nullptr;

            switch (id)
            {
                case TokenID::Keyword:
                {
                    auto keyword = t->keyword;
                    switch (keyword)
                    {
                        case KeywordID::Return:
                        {
                            statement = parse_return();
                        } break;
                        case KeywordID::If:
                        {
                            statement = parse_conditional();
                        } break;
                        case KeywordID::For:
                        {
                            statement = parse_for();
                        } break;
                        default:
                            RNS_NOT_IMPLEMENTED;
                            break;
                    }
                } break;
                case TokenID::Symbol:
                {
                    statement = parse_expression();
                } break;
                case TokenID::RightBrace:
                    break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            if (statement)
            {
                expect_and_consume(';');
            }
            return statement;
        }

        Node* parse_return()
        {
            consume();
            Node* ret_expr = parse_expression();
            Node* node = nb.append(NodeType::Ret);
            node->ret.expr = ret_expr;
            return node;
        }

        Node* parse_right_expression(Node** left_expr)
        {
            BinOp bin_op;

            while ((bin_op = is_bin_op()) != BinOp::None)
            {
                assert(bin_op != BinOp::VariableDecl);
                auto* right_expression = parse_primary_expression();
                if (!right_expression)
                {
                    return nullptr;
                }

                Node* node = nb.append(NodeType::BinOp);
                node->bin_op.op = bin_op;
                node->bin_op.left = *left_expr;
                node->bin_op.right = right_expression;
                *left_expr = node;
            }

            return *left_expr;
        }

        Node* parse_expression()
        {
            Node* left = parse_primary_expression();
            if (!left)
            {
                return nullptr;
            }
            if (left->type == NodeType::VarDecl)
            {
                Token* t;
                Node* var_decl_node = left;

                if ((t = expect_and_consume(TokenID::Type)) || (t = expect_and_consume(TokenID::Symbol)))
                {
                    var_decl_node->var_decl.type = get_type(t);
                }
                // @TODO: @Types Consider types could consume more than one token
                else if ((t = expect_and_consume(TokenID::Symbol)))
                {
                    // @TODO: support more than native types
                    RNS_NOT_IMPLEMENTED;
                    //                Node* type_node = nb.append(NodeType::Type);
                    //                type_node->type_expr.kind = TypeKind::Native;
                    //                Node* type_name_node = nb.append(NodeType::SymName);
                    //                assert(false);
                    //#if 0
                    //                type_name_node->sym_name.len = t->offset;
                    //                type_name_node->sym_name.ptr = t->symbol;
                    //                type_node->type_expr.non_native = nb.get_id(type_name_node);
                    //                var_decl_node->var_decl.type = nb.get_id(type_node);
                    //#endif
                }
                else
                {
                    // @TODO: No type; Should infer
                    RNS_UNREACHABLE;
                }

                if (expect_and_consume('='))
                {
                    var_decl_node->var_decl.value = parse_expression();
                }

                // @TODO: should append to the current scope
                if (current_function->variables.cap == 0)
                {
                    current_function->variables = current_function->variables.create(&allocator, 16);
                }
                current_function->variables.append(var_decl_node);

                return var_decl_node;
            }
            else
            {
                return parse_right_expression(&left);
            }
        }

        // Integrate in parse function
        void parse_prototype(FunctionDeclaration* fn)
        {
            auto token_id = expect_and_consume('(');

            FunctionType fn_type = {};

            Token* next_token;
            bool arg_left_to_parse = (next_token = get_next_token())->id != ')';

            if (arg_left_to_parse)
            {
                s64 allocated_arg_count = 32;
                fn->variables = fn->variables.create(&allocator, allocated_arg_count);
                fn_type.arg_types = fn_type.arg_types.create(&allocator, allocated_arg_count);
            }

            while (arg_left_to_parse)
            {
                auto* node = parse_expression();
                if (!node)
                {
                    compiler.print_error({}, "error parsing argument %lld", current_function->variables.len + 1);
                    return;
                }
                if (node->type != NodeType::VarDecl)
                {
                    compiler.print_error({}, "expected argument", current_function->variables.len + 1);
                    return;
                }
                node->var_decl.is_fn_arg = true;
                fn_type.arg_types.append(node->var_decl.type);
                fn->variables.append(node);
                arg_left_to_parse = (next_token = get_next_token())->id != ')';
            }

            if (!expect_and_consume(')'))
            {
                compiler.print_error({}, "Expected end of argument list");
                return;
            }

            if (expect_and_consume('-') && expect_and_consume('>'))
            {
                auto* t = expect_and_consume(TokenID::Type);
                if (!t)
                {
                    t = expect_and_consume(TokenID::Symbol);
                }
                if (!t)
                {
                    compiler.print_error({}, "Expected return type");
                    return;
                }
                fn_type.ret_type = get_type(t);
                if (!fn_type.ret_type)
                {
                    compiler.print_error({}, "Error parsing return type");
                    return;
                }
            }
            else
            {
                fn_type.ret_type = get_native_type(NativeTypeID::None);
            }

            fn->type = function_type_declarations.append(fn_type);
        }

        void parse_block(ScopeBlock* scope_block, bool allow_no_braces)
        {
            scope_block->parent = current_scope;
            current_scope = scope_block;

            bool has_braces = expect_and_consume('{');
            const u8 expected_end = '}';

            if (!has_braces && !allow_no_braces)
            {
                compiler.print_error({}, "Expected braces");
                return;
            }

            Token* next_token;
            if (has_braces)
            {
                bool statement_left_to_parse = (next_token = get_next_token())->id != expected_end;
                if (statement_left_to_parse)
                {
                    scope_block->statements = scope_block->statements.create(&allocator, 64);
                }

                while (statement_left_to_parse)
                {
                    auto* statement = parse_statement();
                    // @TODO: error logging and out
                    if (!statement)
                    {
                        compiler.print_error({}, "Error parsing block statement %lld", scope_block->statements.len);
                        return;
                    }
                    scope_block->statements.append(statement);
                    statement_left_to_parse = (next_token = get_next_token())->id != expected_end;
                }

                expect_and_consume(expected_end);
            }
            else
            {
                auto statement = parse_statement();
                // @TODO: error logging and out
                if (!statement)
                {
                    compiler.print_error({}, "Error parsing block statement");
                    return;
                }
                scope_block->statements = scope_block->statements.create(&allocator, 1);
                scope_block->statements.append(statement);
            }
        }

        void parse_body(FunctionDeclaration* fn)
        {
            current_scope = fn->scope_blocks.allocate();
            current_scope->type = ScopeType::Function;
            current_scope->parent = nullptr;

            parse_block(current_scope, false);
        }

        FunctionDeclaration parse_function(bool* parsed_ok)
        {
            Token* t = expect_and_consume(TokenID::Symbol);
            if (!t)
            {
                compiler.print_error({}, "Expected symbol at the beginning of a declaration");
                return {};
            }

            if (!expect_and_consume_twice(':', ':'))
            {
                return {};
            }

            FunctionDeclaration fn = {
                .scope_blocks = Buffer<ScopeBlock>::create(&allocator, 16),
            };
            current_function = &fn;

            // @TODO: change this to be properly handled by the function parser
            parse_prototype(&fn);
            if (compiler.errors_reported)
            {
                return fn;
            }

            parse_body(&fn);
            if (compiler.errors_reported)
            {
                return fn;
            }

            *parsed_ok = true;

            return fn;
        }
    };
}

const char* node_type_to_string(NodeType type)
{
    switch (type)
    {
        rns_case_to_str(NodeType::IntLit);
        rns_case_to_str(NodeType::BinOp);
        rns_case_to_str(NodeType::Ret);
        rns_case_to_str(NodeType::VarDecl);

        default:
            RNS_NOT_IMPLEMENTED;
            return nullptr;
    }
}



//bool name_already_exists(ModuleParser* parser, Node* node)
//{
//    Node* name_node = parser->nb.get(name_node_id);
//    assert(name_node);
//    assert(name_node->type == NodeType::SymName);
//
//    Node* scope_node = parser->nb.get(parser->current_scope);
//    assert(scope_node);
//    assert(scope_node->type == NodeType::Function);
//
//    u32* it = scope_node->function_decl.variables.ptr;
//    s64 len = scope_node->function_decl.variables.len;
//    const char* new_name = name_node->sym_name.ptr;
//    for (s64 i = 0; i < len; i++)
//    {
//        u32 node_id = it[i];
//        Node* variable_node = parser->nb.get(node_id);
//        assert(variable_node);
//        assert(variable_node->type == NodeType::VarDecl);
//        u32 var_name_node_id = variable_node->var_decl.name;
//        Node* variable_name_node = parser->nb.get(var_name_node_id);
//        assert(variable_name_node);
//        assert(variable_name_node->type == NodeType::SymName);
//
//        const char* existent_name = variable_name_node->sym_name.ptr;
//        if (strcmp(existent_name, new_name) == 0)
//        {
//            return true;
//        }
//    }
//
//    return false;
//}

// @TODO: reconsider what type is to be returned from top level declarations
AST::Result parse(Compiler& compiler, LexerResult& lexer_result)
{
    RNS_PROFILE_FUNCTION();
    compiler.subsystem = Compiler::Subsystem::Parser;

    Parser parser = {
        .ptr = lexer_result.ptr,
        .parser_it = 0,
        .len = lexer_result.len,
        .allocator = create_suballocator(&compiler.page_allocator, RNS_MEGABYTE(300)),
        .nb = NodeBuffer::create(&parser.allocator, 1024),
        .compiler = compiler,
        .function_declarations = FunctionDeclarationBuffer::create(&parser.allocator, 64),
        .type_declarations = TypeBuffer::create(&parser.allocator, 64),
        .struct_declarations = StructBuffer::create(&parser.allocator, 64),
        .union_declarations = UnionBuffer::create(&parser.allocator, 64),
        .enum_declarations = EnumBuffer::create(&parser.allocator, 64),
        .function_type_declarations = FunctionTypeBuffer::create(&parser.allocator, 64),
    };

    while (parser.parser_it < parser.len)
    {
        bool parsed_ok = false;
        auto fn_decl = parser.parse_function(&parsed_ok);
        if (compiler.errors_reported)
        {
            return { .node_buffer = parser.nb, .function_type_declarations = parser.function_type_declarations, .function_declarations = parser.function_declarations };
        }
        else if (parsed_ok)
        {
            parser.function_declarations.append(fn_decl);
            continue;
        }

        compiler.print_error({}, "Unknown top level declaration");
        return { .node_buffer = parser.nb, .function_type_declarations = parser.function_type_declarations, .function_declarations = parser.function_declarations };
    }

    return { .node_buffer = parser.nb, .function_type_declarations = parser.function_type_declarations, .function_declarations = parser.function_declarations }; // Omit error message as it's only filled when there's an actual error message
}
