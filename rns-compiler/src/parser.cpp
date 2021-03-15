#include "parser.h"

#include <stdio.h>

using namespace Parser;

const char* node_type_to_string(NodeType type)
{
    switch (type)
    {
        rns_case_to_str(NodeType::IntLit);
        rns_case_to_str(NodeType::BinOp);
        rns_case_to_str(NodeType::Ret);
        rns_case_to_str(NodeType::SymName);
        rns_case_to_str(NodeType::Type);
        rns_case_to_str(NodeType::VarDecl);
        rns_case_to_str(NodeType::ArgList);
        rns_case_to_str(NodeType::Struct);
        rns_case_to_str(NodeType::Enum);
        rns_case_to_str(NodeType::Union);
        rns_case_to_str(NodeType::Function);

        default:
            RNS_NOT_IMPLEMENTED;
            return nullptr;
    }
}



u32 mp_size = sizeof(TokenParser) + sizeof(NodeBuffer) + sizeof(bool) + sizeof(ModuleParser::error_message);
//static_assert(sizeof(ModuleParser) == mp_size;

u32 parse_primary_expression(ModuleParser* m)
{
    auto* t = m->tp.get_token();
    auto id = t->get_id();

    switch (id)
    {
        case TokenID::IntegerLit:
        {
            m->tp.consume();
            Node* node = m->nb.append(NodeType::IntLit);
            node->int_lit.lit = t->int_lit;
            return m->nb.get_id(node);
        }
        case TokenID::Symbol:
        {
            auto* t = m->tp.consume();
            Node* node = m->nb.append(NodeType::SymName);
            node->sym_name.len = t->offset;
            node->sym_name.ptr = t->symbol;
            return m->nb.get_id(node);
        }
        case TokenID::Minus:
        {
            m->tp.consume();
            if (m->tp.get_token(0)->get_id() == TokenID::Minus && m->tp.get_token(1)->get_id() == TokenID::Minus)
            {
                return no_value;
            }
            else
            {
                RNS_NOT_IMPLEMENTED;
                return no_value;
            }
        }
        case TokenID::Semicolon:
            return no_value;
        default:
            RNS_NOT_IMPLEMENTED;
            break;
    }

    return no_value;
}

u32 parse_expression(ModuleParser* m);

bool name_already_exists(ModuleParser* m, u32 name_node_id)
{
    Node* name_node = m->nb.get(name_node_id);
    assert(name_node);
    assert(name_node->type == NodeType::SymName);

    Node* scope_node = m->nb.get(m->current_scope);
    assert(scope_node);
    assert(scope_node->type == NodeType::Function);

    u32* it = scope_node->function_decl.variables.ptr;
    s64 len = scope_node->function_decl.variables.len;
    const char* new_name = name_node->sym_name.ptr;
    for (s64 i = 0; i < len; i++)
    {
        u32 node_id = it[i];
        Node* variable_node = m->nb.get(node_id);
        assert(variable_node);
        assert(variable_node->type == NodeType::VarDecl);
        u32 var_name_node_id = variable_node->var_decl.name;
        Node* variable_name_node = m->nb.get(var_name_node_id);
        assert(variable_name_node);
        assert(variable_name_node->type == NodeType::SymName);

        const char* existent_name = variable_name_node->sym_name.ptr;
        if (strcmp(existent_name, new_name) == 0)
        {
            return true;
        }
    }

    return false;
}

u32 parse_right_expression(ModuleParser* m, u32* left_expr)
{
    BinOp bin_op;

    while ((bin_op = m->tp.is_bin_op()) != BinOp::None)
    {
        if (bin_op == BinOp::VariableDecl)
        {
            Token* t;
            Node* var_decl_node = m->nb.append(NodeType::VarDecl);

            u32 var_name = *left_expr;
            assert(!name_already_exists(m, var_name));
            var_decl_node->var_decl.name = var_name;

            if ((t = m->tp.expect_and_consume(TokenID::NativeType)))
            {
                Node* type_node = m->nb.append(NodeType::Type);
                type_node->type_expr.is_native = true;
                type_node->type_expr.native = t->native_type;
                var_decl_node->var_decl.type = m->nb.get_id(type_node);
            }
            // @TODO: @Types Consider types could consume more than one token
            else if ((t = m->tp.expect_and_consume(TokenID::Symbol)))
            {
                Node* type_node = m->nb.append(NodeType::Type);
                type_node->type_expr.is_native = false;
                Node* type_name_node = m->nb.append(NodeType::SymName);
                type_name_node->sym_name.len = t->offset;
                type_name_node->sym_name.ptr = t->symbol;
                type_node->type_expr.non_native = m->nb.get_id(type_name_node);
                var_decl_node->var_decl.type = m->nb.get_id(type_node);
            }
            else
            {
                // No type; Should infer
                var_decl_node->var_decl.type = no_value;
            }
            if (m->tp.expect_and_consume('='))
            {
                var_decl_node->var_decl.value = parse_expression(m);
            }

            u32 var_decl_index = m->nb.get_id(var_decl_node);
            *left_expr = var_decl_index;
            Node* scope_node = m->nb.get(m->current_scope);
            assert(scope_node);
            assert(scope_node->type == NodeType::Function);
            scope_node->function_decl.variables.variable_append(var_decl_index);

            return var_decl_index;
        }
        else
        {
            u32 right_expression = parse_expression(m);
            if (right_expression == UINT32_MAX)
            {
                return UINT32_MAX;
            }

            Node* node = m->nb.append(NodeType::BinOp);
            node->bin_op.op = bin_op;
            node->bin_op.left = *left_expr;
            node->bin_op.right = right_expression;
            *left_expr = m->nb.get_id(node);
        }
    }

    return *left_expr;
}

NodeList parse_list(ModuleParser* m, TokenID expected_begin_token, NodeType expected_type)
{
    auto* list_init_token = m->tp.expect_and_consume(expected_begin_token);
    assert(list_init_token);
    auto id = list_init_token->id;
    u8 expected_end;

    switch (id)
    {
        case '{':
            expected_end = '}';
            break;
        case '(':
            expected_end = ')';
            break;
        case '[':
            expected_end = ']';
            break;
        default:
            RNS_UNREACHABLE;
            expected_end = 0;
    }

    Token* next_token;
    NodeList list = {};
    auto base = m->nib.len;

    while ((next_token = m->tp.get_token())->id != expected_end)
    {
        auto declaration = parse_expression(m);
        // @TODO: error logging and out
        if (declaration == no_value)
        {
            sprintf(m->error_message, "Error parsing argument %lld", list.len + 1);
            return list;
        }
        auto* node = m->nb.get(declaration);
        if (node->type != expected_type)
        {

        }
        (void)m->nib.append(declaration);
        list.len++;
    }

    auto end = m->nib.len;
    if (end > base)
    {
        list.ptr = m->nib.ptr + base;
    }

    m->tp.expect_and_consume(expected_end);

    return list;
}

u32 parse_return(ModuleParser* m)
{
    m->tp.consume();
    u32 ret_expr = parse_expression(m);
    Node* node = m->nb.append(NodeType::Ret);
    node->ret_expr.ret_value = ret_expr;
    return m->nb.get_id(node);
}

u32 parse_statement(ModuleParser* m)
{
    auto* t = m->tp.get_token();
    TokenID id = static_cast<TokenID>(t->id);
    u32 statement = no_value;
    if (id == TokenID::Keyword)
    {
        auto keyword = t->keyword;
        switch (keyword)
        {
            case KeywordID::Return:
                statement = parse_return(m);
                break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }
    else
    {
        switch (id)
        {
            case TokenID::Symbol:
                statement = parse_expression(m);
                break;
            case TokenID::RightBrace:
                break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    if (statement != no_value)
    {
        m->tp.expect_and_consume(';');
    }
    return statement;
}

NodeList parse_block(ModuleParser* m, bool allow_no_braces)
{
    NodeList list = {};
    bool has_braces = m->tp.expect_and_consume('{');
    const u8 expected_end = '}';
    if (!has_braces && !allow_no_braces)
    {
        strcpy(m->error_message, "Expected braces");
        return list;
    }

    Token* next_token;
    auto base = m->nib.len;

    if (has_braces)
    {
        while ((next_token = m->tp.get_token())->id != expected_end)
        {
            auto statement = parse_statement(m);
            // @TODO: error logging and out
            if (statement == no_value)
            {
                sprintf(m->error_message, "Error parsing block statement %lld", list.len + 1);
                return list;
            }
            auto* node = m->nb.get(statement);
            (void)m->nib.append(statement);
            list.len++;
        }
    }
    else
    {
        auto statement = parse_expression(m);
        // @TODO: error logging and out
        if (statement == no_value)
        {
            sprintf(m->error_message, "Error parsing block statement %lld", list.len + 1);
            return list;
        }
        auto* node = m->nb.get(statement);
        (void)m->nib.append(statement);
        list.len++;
    }

    auto end = m->nib.len;
    if (end > base)
    {
        list.ptr = m->nib.ptr + base;
    }

    if (has_braces)
    {
        m->tp.expect_and_consume(expected_end);
    }

    return list;
}

u32 parse_expression(ModuleParser* m)
{
    auto* t = m->tp.get_token();
    u32 left = parse_primary_expression(m);
    if (left == UINT32_MAX)
    {
        return UINT32_MAX;
    }

    return parse_right_expression(m, &left);
}

u32 parse_function(ModuleParser* m)
{
    Token* t = m->tp.expect_and_consume(TokenID::Symbol);
    if (!t)
    {
        strcpy(m->error_message, "Expected symbol at the beginning of a declaration");
        return no_value;
    }

    if (!m->tp.expect_and_consume_twice(':', ':'))
    {
        return no_value;
    }

    // Here we can expect it to be a function
    Node* fn_node = m->nb.append(NodeType::Function);
    m->current_scope = m->nb.get_id(fn_node);

    Node* fn_name_node = m->nb.append(NodeType::SymName);
    fn_name_node->sym_name.ptr = t->symbol;
    fn_name_node->sym_name.len = t->offset;
    fn_node->function_decl.name = m->nb.get_id(fn_name_node);

    // @TODO: change this to be properly handled by the function parser
    fn_node->function_decl.arg_list = parse_list(m, TokenID::LeftParen, NodeType::VarDecl);
    const s64 reserved_for_variables = 32;
    assert(m->nib.len + reserved_for_variables < m->nib.cap);
    fn_node->function_decl.variables.ptr = m->nib.ptr + m->nib.len;
    fn_node->function_decl.variables.cap = reserved_for_variables;
    m->nib.len += reserved_for_variables;
    if (m->failed())
    {
        return no_value;
    }

    if (m->tp.expect('-'))
    {
        m->tp.consume();
        assert(m->tp.expect_and_consume('>'));
        // @TODO: parse complex type
        Token* token;
        if (token = m->tp.expect_and_consume(TokenID::NativeType))
        {
            fn_node->function_decl.ret_type.is_native = true;
            fn_node->function_decl.ret_type.native = token->native_type;
        }
        else if ((token = m->tp.expect_and_consume(TokenID::NativeType)))
        {
            fn_node->function_decl.ret_type.is_native = false;
            Node* ret_type_symbol = m->nb.append(NodeType::SymName);
            ret_type_symbol->sym_name.ptr = token->str_lit;
            ret_type_symbol->sym_name.len = token->offset;
            fn_node->function_decl.ret_type.non_native = m->nb.get_id(ret_type_symbol);
        }
        else
        {
            RNS_UNREACHABLE;
        }
    }
    else
    {

        fn_node->function_decl.ret_type.is_native = true;
        fn_node->function_decl.ret_type.native = NativeTypeID::None;
    }

    fn_node->function_decl.statements = parse_block(m, false);

    return m->nb.get_id(fn_node);
}

ModuleParser parse(TokenBuffer* tb, RNS::Allocator* node_allocator, RNS::Allocator* node_id_allocator, RNS::Allocator* tld_allocator)
{
    ModuleParser module_parser = {
        .tp = {
            .ptr = tb->ptr,
            .parser_it = 0,
            .len = tb->len,
        },
        .nb = NodeBuffer::create(node_allocator),
        .nib = NodeIDBuffer::create(node_id_allocator),
        .tldb = NodeIDBuffer::create(tld_allocator),
    };

    while (module_parser.tp.parser_it < module_parser.tp.len)
    {
        u32 top_level_decl = parse_function(&module_parser);

        if (module_parser.failed())
        {
            return module_parser;
        }
        else if (top_level_decl != no_value)
        {
            module_parser.tldb.append(top_level_decl);
            continue;
        }
    }

    return module_parser;
}
