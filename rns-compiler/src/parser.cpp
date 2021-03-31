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
        StructBuffer struct_declarations;
        UnionBuffer union_declarations;
        EnumBuffer enum_declarations;
        FunctionTypeBuffer function_type_declarations;
        TypeBuffer& type_declarations;

        Node* current_scope;
        Node* current_function;

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
                            RNS_UNREACHABLE;
                            //return BinOp::Function;
                        }
                        else
                        {
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
                case TokenID::Less:
                {
                    consume();
                    if (expect_and_consume(TokenID::Equal))
                    {
                        return BinOp::Cmp_LessThanOrEqual;
                    }
                    else
                    {
                        return BinOp::Cmp_LessThan;
                    }
                }
                case TokenID::Greater:
                {
                    consume();
                    if (expect_and_consume(TokenID::Equal))
                    {
                        return BinOp::Cmp_GreaterThanOrEqual;
                    }
                    else
                    {
                        return BinOp::Cmp_GreaterThan;
                    }
                }
                case TokenID::Asterisk:
                {
                    consume();
                    return BinOp::Mul;
                }
                case TokenID::Slash:
                    // single char
                    RNS_NOT_IMPLEMENTED;
                    return BinOp::None;
                case TokenID::Exclamation:
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

            return BinOp::None;
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
            for (auto* var : current_function->function.variables)
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
                case TokenID::LeftParen:
                {
                    consume();
                    auto* parenthesis_expr = parse_expression();
                    auto* right = expect_and_consume(')');
                    assert(right);
                    assert(parenthesis_expr->type == NodeType::BinOp);
                    parenthesis_expr->bin_op.parenthesis = true;
                    return parenthesis_expr;
                } break;
                case TokenID::Semicolon:
                    return nullptr;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            return nullptr;
        }

        Node* parse_conditional(Node* current_parent)
        {
            consume();
            auto* node = nb.append(NodeType::Conditional);

            node->conditional.condition = parse_expression();
            assert(node->conditional.condition);

            bool braces = expect_and_consume('{');

            auto* if_block = nb.append(NodeType::Block);
            node->conditional.if_block = if_block;
            current_function->function.scope_blocks.append(if_block);
            if_block->block.type = Block::Type::ConditionalBlock;
            if_block->block.parent = current_scope;
            current_scope = if_block;

            if (braces)
            {
                if_block->block.statements = if_block->block.statements.create(&allocator, 128);
                while (!expect_and_consume('}'))
                {
                    auto* st_node = parse_statement(node);
                    assert(st_node);
                    if_block->block.statements.append(st_node);
                }
            }
            else
            {
                if_block->block.statements = if_block->block.statements.create(&allocator, 1);
                auto* st_node = parse_statement(node);
                if_block->block.statements.append(st_node);
            }

            current_scope = current_scope->block.parent;

            if (expect_and_consume_if_keyword(KeywordID::Else))
            {
                auto* else_block = nb.append(NodeType::Block);
                node->conditional.else_block = else_block;
                current_function->function.scope_blocks.append(else_block);
                else_block->block.parent = current_scope;
                else_block->block.type = Block::Type::ConditionalBlock;
                current_scope = else_block;

                braces = expect_and_consume('{');
                if (braces)
                {
                    else_block->block.statements = else_block->block.statements.create(&allocator, 128);
                    while (!expect_and_consume('}'))
                    {
                        auto* st_node = parse_statement(node);
                        assert(st_node);
                        else_block->block.statements.append(st_node);
                    }
                }
                else
                {
                    else_block->block.statements = else_block->block.statements.create(&allocator, 1);
                    auto* st_node = parse_statement(node);
                    assert(st_node);
                    else_block->block.statements.append(st_node);
                }
                current_scope = current_scope->block.parent;
            }
            else
            {
                node->conditional.fake_else = true;
                auto* fake_else_block = nb.append(NodeType::Block);
                node->conditional.else_block = fake_else_block;
                current_function->function.scope_blocks.append(fake_else_block);
                fake_else_block->block.parent = current_scope;
                fake_else_block->block.type = Block::Type::ConditionalBlock;
                current_scope = fake_else_block;
            }

            return node;
        }

        Node* parse_for()
        {
            auto* for_t = expect_and_consume_if_keyword(KeywordID::For);
            assert(for_t);
            Node* for_loop = nb.append(NodeType::Loop);
            auto* parent_scope = current_scope;

            auto create_loop_block = [&](Block::Type loop_block_type)
            {
                auto* block_node = nb.append(NodeType::Block);
                current_function->function.scope_blocks.append(block_node);
                block_node->block.type = loop_block_type;
                block_node->block.parent = for_loop;

                return block_node;
            };

            for_loop->loop.prefix = create_loop_block(Block::Type::LoopPrefix);
            for_loop->loop.body = create_loop_block(Block::Type::LoopBody);
            for_loop->loop.postfix = create_loop_block(Block::Type::LoopBody);

            auto* it_symbol = expect_and_consume(TokenID::Symbol);
            Node* it_decl = nb.append(NodeType::VarDecl);
            it_decl->var_decl.is_fn_arg = false;
            it_decl->var_decl.name = RNS::String{ it_symbol->symbol, it_symbol->offset };
            it_decl->var_decl.scope = current_scope;
            it_decl->var_decl.type = &type_declarations[(u32)NativeTypeID::S32];
            // @TODO: we should match it to the right operand
            it_decl->var_decl.value = nb.append(NodeType::IntLit);
            it_decl->var_decl.value->int_lit.bit_count = 32;
            it_decl->var_decl.value->int_lit.is_signed = false;
            it_decl->var_decl.value->int_lit.lit = 0;
            this->current_function->function.variables.append(it_decl);
            this->current_scope->block.statements.append(it_decl);

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
                for_loop->loop.prefix->block.statements = for_loop->loop.prefix->block.statements.create(&allocator, rns_array_length(prefix_statements));
                for (auto* prefix_st : prefix_statements)
                {
                    for_loop->loop.prefix->block.statements.append(prefix_st);
                }
            }

            {
                current_scope = for_loop->loop.body;
                parse_block(current_scope, true);
            }

            {
                current_scope = for_loop->loop.postfix;
                current_scope->block.statements = current_scope->block.statements.create(&allocator, 1);
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
                Node* postfix_assign = nb.append(NodeType::BinOp);
                postfix_assign->bin_op.left = var_expr;
                postfix_assign->bin_op.right = postfix_increment;
                postfix_assign->bin_op.op = BinOp::Assign;
                current_scope->block.statements.append(postfix_assign);
            }

            current_scope = parent_scope;
            return for_loop;
        }

        Node* parse_break(Node* parent)
        {
            auto* break_token = expect_and_consume_if_keyword(KeywordID::Break);
            assert(break_token);

            auto* break_node = nb.append(NodeType::Break);
            assert(current_scope);
            break_node->break_.block = current_scope;
            return break_node;
        }

        Node* parse_statement(Node* current_parent = nullptr)
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
                            statement = parse_conditional(current_parent);
                        } break;
                        case KeywordID::For:
                        {
                            statement = parse_for();
                        } break;
                        case KeywordID::Break:
                        {
                            statement = parse_break(current_parent);
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

            if (statement && statement->type != NodeType::Conditional && statement->type != NodeType::Loop)
            {
                auto* colon = expect_and_consume(';');
                assert(colon);
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
                auto* binary_op_left_expression = *left_expr;
                auto* binary_op_right_expression = parse_primary_expression();

                if (!binary_op_right_expression)
                {
                    return nullptr;
                }

                // @TODO: bad practice to use tagged union fields without knowing if they are what you want, but useful here
                auto left_expression_operator_precedence = operator_precedence.rules[(u32)binary_op_left_expression->bin_op.op];
                auto right_expression_operator_precedence = operator_precedence.rules[(u32)bin_op];
                bool right_precedes_left = binary_op_left_expression->type == NodeType::BinOp && right_expression_operator_precedence < left_expression_operator_precedence &&
                    !binary_op_left_expression->bin_op.parenthesis && (binary_op_right_expression->type != NodeType::BinOp || (binary_op_right_expression->type == NodeType::BinOp && !binary_op_right_expression->bin_op.parenthesis));

                if (right_precedes_left)
                {
                    Node* right_operand_of_left_binary_expression = binary_op_left_expression->bin_op.right;
                    binary_op_left_expression->bin_op.right = nb.append(NodeType::BinOp);
                    auto* new_prioritized_expression = binary_op_left_expression->bin_op.right;
                    new_prioritized_expression->bin_op.op = bin_op;
                    new_prioritized_expression->bin_op.left = right_operand_of_left_binary_expression;
                    new_prioritized_expression->bin_op.right = binary_op_right_expression;
                    // @TODO: redundant?
                    *left_expr = binary_op_left_expression;
                }
                else
                {
                    Node* node = nb.append(NodeType::BinOp);
                    node->bin_op.op = bin_op;
                    node->bin_op.left = binary_op_left_expression;
                    node->bin_op.right = binary_op_right_expression;
                    *left_expr = node;
                }
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
                if (current_function->function.variables.cap == 0)
                {
                    current_function->function.variables = current_function->function.variables.create(&allocator, 16);
                }
                current_function->function.variables.append(var_decl_node);

                return var_decl_node;
            }
            else
            {
                return parse_right_expression(&left);
            }
        }

        void parse_block(Node* scope_block, bool allow_no_braces)
        {
            scope_block->block.parent = current_scope;
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
                    scope_block->block.statements = scope_block->block.statements.create(&allocator, 64);
                }

                while (statement_left_to_parse)
                {
                    auto* statement = parse_statement(scope_block);
                    // @TODO: error logging and out
                    if (!statement)
                    {
                        compiler.print_error({}, "Error parsing block statement %lld", scope_block->block.statements.len + 1);
                        return;
                    }
                    scope_block->block.statements.append(statement);
                    if (statement->type == NodeType::Conditional && statement->conditional.fake_else)
                    {
                        scope_block = statement->conditional.else_block;
                        scope_block->block.statements = scope_block->block.statements.create(&allocator, 64);
                    }
                    statement_left_to_parse = (next_token = get_next_token())->id != expected_end;
                }

                expect_and_consume(expected_end);
            }
            else
            {
                auto statement = parse_statement(scope_block);
                // @TODO: error logging and out
                if (!statement)
                {
                    compiler.print_error({}, "Error parsing block statement");
                    return;
                }
                scope_block->block.statements = scope_block->block.statements.create(&allocator, 1);
                scope_block->block.statements.append(statement);
            }
        }

        Node* parse_function(bool* parsed_ok)
        {
            Token* t = expect_and_consume(TokenID::Symbol);
            if (!t)
            {
                compiler.print_error({}, "Expected symbol at the beginning of a declaration");
                return nullptr;
            }

            if (!expect_and_consume_twice(':', ':'))
            {
                return nullptr;
            }

            auto* function_node = nb.append(NodeType::Function);
            function_node->function = {
                .scope_blocks = Buffer<Node*>::create(&allocator, 16),
            };
            current_function = function_node;

            // @TODO: change this to be properly handled by the function parser
            auto token_id = expect_and_consume('(');

            FunctionType fn_type = {};

            Token* next_token;
            bool arg_left_to_parse = (next_token = get_next_token())->id != ')';

            s64 allocated_arg_count = 32;
            
            function_node->function.variables = function_node->function.variables.create(&allocator, allocated_arg_count);

            if (arg_left_to_parse)
            {
                fn_type.arg_types = fn_type.arg_types.create(&allocator, allocated_arg_count);
            }

            while (arg_left_to_parse)
            {
                auto* node = parse_expression();
                if (!node)
                {
                    compiler.print_error({}, "error parsing argument %lld", current_function->function.variables.len + 1);
                    return nullptr;
                }
                if (node->type != NodeType::VarDecl)
                {
                    compiler.print_error({}, "expected argument", current_function->function.variables.len + 1);
                    return nullptr;
                }
                node->var_decl.is_fn_arg = true;
                fn_type.arg_types.append(node->var_decl.type);
                function_node->function.variables.append(node);
                arg_left_to_parse = (next_token = get_next_token())->id != ')';
            }

            if (!expect_and_consume(')'))
            {
                compiler.print_error({}, "Expected end of argument list");
                return nullptr;
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
                    return nullptr;
                }
                fn_type.ret_type = get_type(t);
                if (!fn_type.ret_type)
                {
                    compiler.print_error({}, "Error parsing return type");
                    return nullptr;
                }
            }
            else
            {
                fn_type.ret_type = get_native_type(NativeTypeID::None);
            }

            function_node->function.type = nb.append(NodeType::FunctionType);
            function_type_declarations.append(function_node->function.type);
            function_node->function.type->function_type = fn_type;

            if (compiler.errors_reported)
            {
                return nullptr;
            }

            current_scope = nb.append(NodeType::Block);
            function_node->function.scope_blocks.append(current_scope);
            current_scope->block.type = Block::Type::Function;
            current_scope->block.parent = nullptr;

            parse_block(current_scope, false);

            if (compiler.errors_reported)
            {
                return nullptr;
            }

            *parsed_ok = true;

            return function_node;
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
AST::Result parse(Compiler& compiler, LexerResult& lexer_result, TypeBuffer& type_declarations)
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
        .struct_declarations = StructBuffer::create(&parser.allocator, 64),
        .union_declarations = UnionBuffer::create(&parser.allocator, 64),
        .enum_declarations = EnumBuffer::create(&parser.allocator, 64),
        .function_type_declarations = FunctionTypeBuffer::create(&parser.allocator, 64),
        .type_declarations = type_declarations,
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
