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
                if (t->id == static_cast<TokenID>(id))
                {
                    return t;
                }
                return nullptr;
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
            bool result = (t1 && t1->id == static_cast<TokenID>(id1) && t2 && t2->id == static_cast<TokenID>(id2));
            if (result)
            {
                consume(); consume();
            }
            return result;
        }


        inline Token* consume()
        {
            auto* result = &ptr[parser_it++];
            printf("Consuming: ");
            result->print_token_id();
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
                case TokenID::LeftBracket:
                {
                    consume();
                    return BinOp::Subscript;
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

        Type* parser_get_type_scanning(Node* parent)
        {
            auto* t = consume();
            auto id = t->get_id();
            switch (id)
            {
                case TokenID::Type:
                    return t->type;
                case TokenID::Ampersand:
                {
                    auto* type = parser_get_type_scanning(parent);
                    assert(type);
                    return Type::get_pointer_type(type, type_declarations);
                }
                case TokenID::LeftBracket:
                {
                    auto i = 0;
                    Type* array_elements_type = nullptr;
                    Token* in_brackets_token = nullptr;
                    s64 array_length = 0;
                    if ((in_brackets_token = expect_and_consume(TokenID::IntegerLit)))
                    {
                        array_length = in_brackets_token->int_lit;
                    }
                    auto* right_bracket = expect_and_consume(']');
                    assert(right_bracket);
                    array_elements_type = parser_get_type_scanning(parent);
                    assert(array_elements_type);
                    assert(right_bracket);

                    auto* array_type = Type::get_array_type(array_elements_type, array_length, type_declarations);
                    return array_type;
                }
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            return nullptr;
        }

        Node* find_existing_variable(Token* token)
        {
            RNS::String name = { token->symbol, token->offset };
            for (auto* var : current_function->function.arguments)
            {
                assert(var->type == NodeType::VarDecl);
                if (var->var_decl.name.equal(name))
                {
                    return var;
                }
            }
            for (auto* var : current_function->function.variables)
            {
                assert(var->type == NodeType::VarDecl);
                if (var->var_decl.name.equal(name))
                {
                    return var;
                }
            }

            return nullptr;
        }

        Node* find_existing_invoke_expression(Token* token)
        {
            String name = { token->symbol, token->offset };

            for (auto* function_node : this->function_declarations)
            {
                if (function_node->function.name.equal(name))
                {
                    return function_node;
                }
            }

            if (name.equal(current_function->function.name))
            {
                return current_function;
            }

            return nullptr;
        }

        Type* get_type(Node* node, Type* expected_type = nullptr)
        {
            switch (node->type)
            {
                case NodeType::IntLit:
                {
                    if (expected_type)
                    {
                        switch (expected_type->id)
                        {
                            // @TODO: check if it fits
                            case TypeID::IntegerType:
                                return expected_type;
                            case TypeID::ArrayType:
                            {
                                auto* arrtype = expected_type->array_t.type;
                                assert(arrtype);
                                assert(arrtype->id == TypeID::IntegerType);
                                return arrtype;
                            }
                            default:
                                RNS_NOT_IMPLEMENTED;
                                break;
                        }
                    }
                    else
                    {
                        RNS_NOT_IMPLEMENTED;
                    }
                } break;
                case NodeType::VarDecl:
                {
                    auto* var_type = node->var_decl.type;
                    assert(var_type);
                    return var_type;
                } break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            return nullptr;
        }

        Node* parse_primary_expression(Node* parent)
        {
            auto* t = get_next_token();
            auto id = t->get_id();

            switch (id)
            {
                case TokenID::IntegerLit:
                {
                    this->consume();
                    Node* node = nb.append(NodeType::IntLit, parent);
                    node->int_lit.lit = t->int_lit;
                    return node;
                }
                case TokenID::Symbol:
                {
                    consume();
                    auto* next_t = expect_and_consume(':');
                    if (next_t)
                    {
                        auto* var_decl_node = nb.append(NodeType::VarDecl, parent);
                        var_decl_node->var_decl.name.ptr = t->symbol;
                        var_decl_node->var_decl.name.len = t->offset;
                        return var_decl_node;
                    }
                    else if ((u32)get_next_token()->id == '(')
                    {
                        auto* invoke_expr_node = nb.append(NodeType::InvokeExpr, parent);
                        invoke_expr_node->invoke_expr.expr = find_existing_invoke_expression(t);
                        assert(invoke_expr_node->invoke_expr.expr);
                        auto* left_paren = expect_and_consume('(');
                        if (!left_paren)
                        {
                            compiler.print_error({}, "Invoke expression requires opening parenthesis");
                            return nullptr;
                        }

                        bool args_left_to_parse = !expect_and_consume(')');
                        {
                            invoke_expr_node->invoke_expr.arguments = invoke_expr_node->invoke_expr.arguments.create(&allocator, 64);
                        }
                        while (args_left_to_parse)
                        {
                            auto* new_arg = parse_expression(invoke_expr_node);
                            assert(new_arg);
                            invoke_expr_node->invoke_expr.arguments.append(new_arg);
                            args_left_to_parse = !expect_and_consume(')');
                            if (args_left_to_parse)
                            {
                                auto* comma = expect_and_consume(',');
                                assert(comma);
                            }
                        }

                        return invoke_expr_node;
                    }
                    else
                    {
                        auto* var_expr_node = nb.append(NodeType::VarExpr, parent);
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
                    auto* parenthesis_expr = parse_expression(parent);
                    auto* right = expect_and_consume(')');
                    assert(right);
                    assert(parenthesis_expr->type == NodeType::BinOp);
                    parenthesis_expr->bin_op.parenthesis = true;
                    return parenthesis_expr;
                } break;
                case TokenID::Ampersand:
                {
                    consume();
                    auto* addressof_node = nb.append(NodeType::UnaryOp, parent);
                    addressof_node->unary_op.type = UnaryOp::AddressOf;
                    addressof_node->unary_op.pos = UnaryOpType::Prefix;
                    addressof_node->unary_op.node = parse_expression(addressof_node);

                    return addressof_node;
                } break;
                case TokenID::At:
                {
                    consume();
                    auto* p_dereference_node = nb.append(NodeType::UnaryOp, parent);
                    p_dereference_node->unary_op.type = UnaryOp::PointerDereference;
                    p_dereference_node->unary_op.pos = UnaryOpType::Prefix;
                    p_dereference_node->unary_op.node = parse_primary_expression(p_dereference_node);

                    return p_dereference_node;
                } break;
                case TokenID::LeftBracket:
                {
                    consume();
                    auto* array_lit_node = nb.append(NodeType::ArrayLit, parent);
                    bool elements_left_to_parse = !expect_and_consume(']');
                    if (elements_left_to_parse)
                    {
                        array_lit_node->array_lit.elements = array_lit_node->array_lit.elements.create(&allocator, 16);
                        for (;;)
                        {
                            auto* literal_node = parse_expression(array_lit_node);
                            assert(literal_node);
                            if (!literal_node)
                            {
                                compiler.print_error({}, "Couldn't parse array literal element");
                                return nullptr;
                            }
                            array_lit_node->array_lit.elements.append(literal_node);

                            elements_left_to_parse = !expect_and_consume(']');
                            if (elements_left_to_parse)
                            {
                                auto* comma = expect_and_consume(',');
                                assert(comma);
                            }
                            else
                            {
                                break;
                            }
                        }

                        auto* first_element = array_lit_node->array_lit.elements[0];
                        auto* expected_type = get_type(parent);
                        auto* elem_type = get_type(first_element, expected_type);
                        auto arrlen = array_lit_node->array_lit.elements.len;
                        array_lit_node->array_lit.type = Type::get_array_type(elem_type, arrlen, type_declarations);
                    }
                    else
                    {
                        RNS_NOT_IMPLEMENTED;
                    }

                    return array_lit_node;
                } break;
                case TokenID::Semicolon:
                    return nullptr;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            return nullptr;
        }

        Node* parse_conditional(Node* parent)
        {
            consume();
            auto* node = nb.append(NodeType::Conditional, parent);

            node->conditional.condition = parse_expression(node);
            assert(node->conditional.condition);

            bool braces = expect_and_consume('{');

            auto* if_block = nb.append(NodeType::Block, node);
            node->conditional.if_block = if_block;
            current_function->function.scope_blocks.append(if_block);
            if_block->block.type = Block::Type::IfBlock;
            if_block->parent = node;
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

            current_scope = current_scope->parent;

            if (expect_and_consume_if_keyword(KeywordID::Else))
            {
                auto* else_block = nb.append(NodeType::Block, node);
                node->conditional.else_block = else_block;
                current_function->function.scope_blocks.append(else_block);
                else_block->block.type = Block::Type::ElseBlock;
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
                current_scope = current_scope->parent;
            }

            return node;
        }

        Node* parse_for(Node* parent)
        {
            auto* for_t = expect_and_consume_if_keyword(KeywordID::For);
            assert(for_t);
            Node* for_loop = nb.append(NodeType::Loop, parent);
            auto* parent_scope = current_scope;

            auto create_loop_block = [&](Block::Type loop_block_type)
            {
                auto* block_node = nb.append(NodeType::Block, for_loop);
                current_function->function.scope_blocks.append(block_node);
                block_node->block.type = loop_block_type;

                return block_node;
            };

            for_loop->loop.prefix = create_loop_block(Block::Type::LoopPrefix);
            for_loop->loop.body = create_loop_block(Block::Type::LoopBody);
            for_loop->loop.postfix = create_loop_block(Block::Type::LoopBody);

            auto* it_symbol = expect_and_consume(TokenID::Symbol);
            assert(it_symbol);
            Node* it_decl = nb.append(NodeType::VarDecl, for_loop);
            it_decl->var_decl.is_fn_arg = false;
            it_decl->var_decl.name = RNS::String{ it_symbol->symbol, it_symbol->offset };
            it_decl->var_decl.scope = current_scope;
            it_decl->var_decl.type = Type::get_integer_type(32, true, type_declarations);
            // @TODO: we should match it to the right operand
            it_decl->var_decl.value = nb.append(NodeType::IntLit, it_decl);
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
                        right_node = nb.append(NodeType::IntLit, for_loop->loop.prefix);
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
                Node* it_expr = nb.append(NodeType::VarExpr, for_loop->loop.prefix);
                it_expr->var_expr.mentioned = it_decl;
                Node* cmp_op = nb.append(NodeType::BinOp, for_loop->loop.prefix);
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
                Node* var_expr = nb.append(NodeType::VarExpr, current_scope);
                var_expr->value_type = ValueType::LValue;
                var_expr->var_expr.mentioned = it_decl;
                Node* one_lit = nb.append(NodeType::IntLit, current_scope);
                one_lit->int_lit.bit_count = 32;
                one_lit->int_lit.is_signed = false;
                one_lit->int_lit.lit = 1;
                Node* postfix_increment = nb.append(NodeType::BinOp, current_scope);
                postfix_increment->bin_op.left = var_expr;
                postfix_increment->bin_op.op = BinOp::Plus;
                postfix_increment->bin_op.right = one_lit;
                Node* postfix_assign = nb.append(NodeType::BinOp, current_scope);
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

            auto* break_node = nb.append(NodeType::Break, parent);
            assert(current_scope);
            auto* target = current_scope;
            while (target->type != NodeType::Loop)
            {
                auto* parent = target->parent;
                assert(parent);
                target = parent;
            }
            assert(target);
            break_node->break_.target = target;
            return break_node;
        }

        Node* parse_statement(Node* parent)
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
                            statement = parse_return(parent);
                        } break;
                        case KeywordID::If:
                        {
                            statement = parse_conditional(parent);
                        } break;
                        case KeywordID::For:
                        {
                            statement = parse_for(parent);
                        } break;
                        case KeywordID::Break:
                        {
                            statement = parse_break(parent);
                        } break;
                        default:
                            RNS_NOT_IMPLEMENTED;
                            break;
                    }
                } break;
                case TokenID::Symbol:
                {
                    statement = parse_expression(parent);
                } break;
                case TokenID::At:
                {
                    // @Info: pointer dereference
                    statement = parse_expression(parent);
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

        Node* parse_return(Node* parent)
        {
            consume();
            Node* node = nb.append(NodeType::Ret, parent);
            Node* ret_expr = parse_expression(node);
            node->ret.expr = ret_expr;
            return node;
        }

        Node* parse_right_expression(Node** left_expr, Node* parent)
        {
            BinOp bin_op;

            while ((bin_op = is_bin_op()) != BinOp::None)
            {
                assert(bin_op != BinOp::VariableDecl);
                auto* binary_op_left_expression = *left_expr;
                if (bin_op == BinOp::Subscript)
                {
                    Node* subscript_node = nb.append(NodeType::Subscript, parent);
                    binary_op_left_expression->parent = subscript_node;
                    subscript_node->subscript.expr_ref = binary_op_left_expression;
                    subscript_node->subscript.index_ref = parse_expression(subscript_node);
                    auto* right_bracket = expect_and_consume(']');
                    assert(right_bracket);
                    assert(subscript_node->subscript.index_ref);
                    *left_expr = subscript_node;
                }
                else
                {
                    if (bin_op == BinOp::Assign)
                    {
                        binary_op_left_expression->value_type = ValueType::LValue;
                    }

                    auto* binary_op_right_expression = parse_primary_expression(parent);
                    if (!binary_op_right_expression)
                    {
                        return nullptr;
                    }

                    // @TODO: bad practice to use tagged union fields without knowing if they are what you want, but useful here
                    auto left_bin_op = binary_op_left_expression->bin_op.op;
                    auto right_bin_op = bin_op;
                    assert(left_bin_op < BinOp::Count);
                    assert(right_bin_op < BinOp::Count);
                    auto left_expression_operator_precedence = operator_precedence.rules[(u32)left_bin_op];
                    auto right_expression_operator_precedence = operator_precedence.rules[(u32)right_bin_op];
                    bool right_precedes_left = binary_op_left_expression->type == NodeType::BinOp && right_expression_operator_precedence < left_expression_operator_precedence &&
                        !binary_op_left_expression->bin_op.parenthesis && (binary_op_right_expression->type != NodeType::BinOp || (binary_op_right_expression->type == NodeType::BinOp && !binary_op_right_expression->bin_op.parenthesis));

                    if (right_precedes_left)
                    {
                        Node* right_operand_of_left_binary_expression = binary_op_left_expression->bin_op.right;
                        binary_op_left_expression->bin_op.right = nb.append(NodeType::BinOp, parent);
                        auto* new_prioritized_expression = binary_op_left_expression->bin_op.right;
                        new_prioritized_expression->bin_op.op = bin_op;
                        new_prioritized_expression->bin_op.left = right_operand_of_left_binary_expression;
                        new_prioritized_expression->bin_op.right = binary_op_right_expression;
                        // @TODO: redundant?
                        *left_expr = binary_op_left_expression;
                    }
                    else
                    {
                        Node* node = nb.append(NodeType::BinOp, parent);
                        node->bin_op.op = bin_op;
                        node->bin_op.left = binary_op_left_expression;
                        node->bin_op.right = binary_op_right_expression;
                        *left_expr = node;
                    }
                }
            }

            return *left_expr;
        }

        Node* parse_expression(Node* parent)
        {
            Node* left = parse_primary_expression(parent);
            if (!left)
            {
                return nullptr;
            }
            if (left->type == NodeType::VarDecl)
            {
                Node* var_decl_node = left;

                var_decl_node->var_decl.type = parser_get_type_scanning(parent);

                if (expect_and_consume('='))
                {
                    var_decl_node->var_decl.value = parse_expression(var_decl_node);
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
                return parse_right_expression(&left, parent);
            }
        }

        void parse_block(Node* scope_block, bool allow_no_braces)
        {
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
                bool statement_left_to_parse = (next_token = get_next_token())->id != (TokenID)expected_end;
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
                    statement_left_to_parse = (next_token = get_next_token())->id != (TokenID)expected_end;
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

            auto* function_node = nb.append(NodeType::Function, nullptr);
            function_node->function = {
                .scope_blocks = Buffer<Node*>::create(&allocator, 16),
                .name = { t->symbol, t->offset },
            };
            current_function = function_node;

            // @TODO: change this to be properly handled by the function parser
            auto token_id = expect_and_consume('(');

            FunctionType fn_type = {};

            Token* next_token;
            bool arg_left_to_parse = (next_token = get_next_token())->id != (TokenID)')';

            s64 allocated_arg_count = 32;
            
            function_node->function.arguments = function_node->function.arguments.create(&allocator, allocated_arg_count);

            if (arg_left_to_parse)
            {
                fn_type.arg_types = fn_type.arg_types.create(&allocator, allocated_arg_count);
            }

            while (arg_left_to_parse)
            {
                auto* node = parse_expression(function_node);
                if (!node)
                {
                    compiler.print_error({}, "error parsing argument %lld", current_function->function.arguments.len + 1);
                    return nullptr;
                }
                if (node->type != NodeType::VarDecl)
                {
                    compiler.print_error({}, "expected argument", current_function->function.arguments.len + 1);
                    return nullptr;
                }
                node->var_decl.is_fn_arg = true;
                fn_type.arg_types.append(node->var_decl.type);
                function_node->function.arguments.append(node);
                arg_left_to_parse = (next_token = get_next_token())->id != (TokenID)')';
                if (arg_left_to_parse)
                {
                    auto* comma = expect_and_consume(',');
                    assert(comma);
                }
            }

            if (!expect_and_consume(')'))
            {
                compiler.print_error({}, "Expected end of argument list");
                return nullptr;
            }

            if (expect_and_consume('-') && expect_and_consume('>'))
            {
                fn_type.ret_type = parser_get_type_scanning(nullptr);
                if (!fn_type.ret_type)
                {
                    compiler.print_error({}, "Error parsing return type");
                    return nullptr;
                }
            }
            else
            {
                fn_type.ret_type = Type::get_void_type(type_declarations);
            }

            function_node->function.type = nb.append(NodeType::TypeExpr, function_node);
            function_node->function.type->type_expr.id = TypeID::FunctionType;
            function_node->function.type->type_expr.function_t = fn_type;

            if (compiler.errors_reported)
            {
                return nullptr;
            }

            current_scope = nb.append(NodeType::Block, function_node);
            function_node->function.scope_blocks.append(current_scope);
            current_scope->block.type = Block::Type::Function;
            current_scope->parent = function_node;

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
