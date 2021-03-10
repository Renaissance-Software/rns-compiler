#include <RNS/types.h>
#include <RNS/arch.h>
#include <RNS/compiler.h>
#include <RNS/os.h>
#include <RNS/data_structures.h>

#include <stdio.h>
#include <stdlib.h>

using namespace RNS;

#define CaseAlphabeticUpper \
case 'A':\
case 'B':\
case 'C': \
case 'D': \
case 'E': \
case 'F': \
case 'G': \
case 'H': \
case 'I': \
case 'J': \
case 'K': \
case 'L': \
case 'M': \
case 'N': \
case 'O': \
case 'P': \
case 'Q': \
case 'R': \
case 'S': \
case 'T': \
case 'U': \
case 'V': \
case 'W': \
case 'X': \
case 'Y': \
case 'Z':

#define CaseAlphabetLower \
case 'a': \
case 'b': \
case 'c': \
case 'd': \
case 'e': \
case 'f': \
case 'g': \
case 'h': \
case 'i': \
case 'j': \
case 'k': \
case 'l': \
case 'm': \
case 'n': \
case 'o': \
case 'p': \
case 'q': \
case 'r': \
case 's': \
case 't': \
case 'u': \
case 'v': \
case 'w': \
case 'x': \
case 'y': \
case 'z': 

#define BlankChars \
case ' ': \
case '\n'

#define SymbolEnd \
case ' ': \
case ',': \
case ':': \
case ';': \
case '.': \
case '\\': \
case '&': \
case '(': \
case ')': \
case '[': \
case ']'

#define SymbolStart \
CaseAlphabeticUpper \
CaseAlphabetLower \
case '_'

#define DecimalDigits \
case '0': \
case '1': \
case '2': \
case '3': \
case '4': \
case '5': \
case '6': \
case '7': \
case '8': \
case '9'

#define NumberStart \
DecimalDigits

enum class TokenID : u8
{
    /* These are names */
    Keyword,
    NativeType,
    Intrinsic,
    Symbol,

    StringLit,
    FloatLit,
    CharLit,
    IntegerLit,

    /* Sign tokens */
    Space = ' ', // 0x20 = 32
    Exclamation = '!', // 0x21 = 33,
    DoubleQuote = '\"', // 0x22 = 34,
    Number = '#', // 0x23, = 35,
    Dollar = '$', // 0x24 = 36,
    Percent = '%', // 0x25 = 37,
    Ampersand = '&', // 0x26 = 38,
    SingleQuote = '\'', // 0x27 = 39
    LeftParen = '(', // 0x28 = 40
    RightParen = ')', // 0x29 = 41,
    Asterisk = '*', // 0x2A = 42
    Plus = '+', // 0x2B = 43
    Comma = ',', // 0x2C = 44
    Minus = '-', // 0x2D = 45
    Period = '.', // 0x2E = 46
    Slash = '/', // 0x2F = 47
    Zero = '0',
    One = '1',
    Two = '2',
    Three = '3',
    Four = '4',
    Five = '5',
    Six = '6',
    Seven = '7',
    Eight = '8',
    Nine = '9',
    Colon = ':', // 0x3A, = 58
    Semicolon = ';', // 0x3B, = 59
    Less = '<', // 0x3C, = 60
    Equal = '=', // 0x3D, = 61
    Greater = '>', // 0x3E, = 62
    Question = '?', // 0x3F, = 63
    At = '@', // 0x40, = 64
    Upper_A = 'A',
    Upper_B = 'B',
    Upper_C = 'C',
    Upper_D = 'D',
    Upper_E = 'E',
    Upper_F = 'F',
    Upper_G = 'G',
    Upper_H = 'H',
    Upper_I = 'I',
    Upper_J = 'J',
    Upper_K = 'K',
    Upper_L = 'L',
    Upper_M = 'M',
    Upper_N = 'N',
    Upper_O = 'O',
    Upper_P = 'P',
    Upper_Q = 'Q',
    Upper_R = 'R',
    Upper_S = 'S',
    Upper_T = 'T',
    Upper_U = 'U',
    Upper_V = 'V',
    Upper_W = 'W',
    Upper_X = 'X',
    Upper_Y = 'Y',
    Upper_Z = 'Z',
    LeftBracket = '[', // 91
    Backslash = '\\', // 92
    RightBracket = ']', // 93
    Caret = '^', // 94
    Underscore = '_', // 95
    Grave = '`', // 96
    Lower_a = 'a',
    Lower_b = 'b',
    Lower_c = 'c',
    Lower_d = 'd',
    Lower_e = 'e',
    Lower_f = 'f',
    Lower_g = 'g',
    Lower_h = 'h',
    Lower_i = 'i',
    Lower_j = 'j',
    Lower_k = 'k',
    Lower_l = 'l',
    Lower_m = 'm',
    Lower_n = 'n',
    Lower_o = 'o',
    Lower_p = 'p',
    Lower_q = 'q',
    Lower_r = 'r',
    Lower_s = 's',
    Lower_t = 't',
    Lower_u = 'u',
    Lower_v = 'v',
    Lower_w = 'w',
    Lower_x = 'x',
    Lower_y = 'y',
    Lower_z = 'z',
    LeftBrace = '{', // 123
    Bar = '|', // 124
    RightBrace = '}', // 125
    Tilde = '~', // 126
};

enum class KeywordID : u8
{
    Return,
    If,
    Else,
    For,
    While,
    Break,
    Continue,
    Count,
};

enum class NativeTypeID : u8
{
    None,
    U8,
    U16,
    U32,
    U64,
    S8,
    S16,
    S32,
    S64,
    F32,
    F64,
    Count,
};

enum class IntrinsicID : u8
{
    Foo,
};

struct Token
{
    u32 id : 7;
    u32 start : 25;
    u16 offset;
    u16 line;
    union
    {
        u64 int_lit;
        f64 float_lit;
        char char_lit;
        char* str_lit;
        char* symbol;
        KeywordID keyword;
        NativeTypeID native_type;
        IntrinsicID intrinsic;
    };

    inline TokenID get_id()
    {
        return static_cast<TokenID>(id);
    }

};

static_assert(sizeof(Token) == 2 * sizeof(s64));

enum class BinOp
{
    None,
    Plus,
    Minus,
    Function,
    VariableDecl,
    Assign,
    Cmp_Equal,
};

struct TokenBuffer
{
    Token* ptr;
    s64 len;
    s64 cap;
    s64 line_count;
    s64 file_size;
    const char* file;
    StringBuffer sb;
    s64 consume_count;

    inline Token* new_token(TokenID type, u32 start, u32 end, u16 line)
    {
        assert(len + 1 < cap);
        Token* token = &ptr[len++];
        token->id = static_cast<u8>(type);
        token->start = start;
        token->offset = static_cast<u16>(end - start);
        token->line = line;
        return token;
    }

};

struct TokenParser
{
    Token* ptr;
    s64 parser_it;
    s64 len;

    inline Token* get_token(s64 i)
    {
        if (parser_it + i < len)
        {
            return &ptr[parser_it + i];
        }
        return nullptr;
    }

    inline Token* get_token()
    {
        return get_token(0);
    }

    template<typename T>
    inline Token* expect(T id)
    {
        Token* t = get_token();
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
            parser_it++;
        }
        return t;
    }

    template<typename T>
    inline bool expect_and_consume_twice(T id1, T id2)
    {
        Token* t1 = get_token(0);
        Token* t2 = get_token(1);
        bool result = (t1 && t1->id == static_cast<u8>(id1) && t2 && t2->id == static_cast<u8>(id2));
        if (result)
        {
            consume(); consume();
        }
        return result;
    }


    inline Token* consume()
    {
        return &ptr[parser_it++];
    }

    inline BinOp is_bin_op()
    {
        auto* first_t = get_token();
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
                if (expect('='))
                {
                    return BinOp::Cmp_Equal;
                }
                else
                {
                    return BinOp::Assign;
                }
            case TokenID::Plus:
                consume();
                return BinOp::Plus;
            case TokenID::Minus:
            case TokenID::Asterisk:
            case TokenID::Slash:
                // single char
                RNS_NOT_IMPLEMENTED;
            case TokenID::Exclamation:
            case TokenID::Greater:
            case TokenID::Less:
                // two characters or more
                RNS_NOT_IMPLEMENTED;
            case TokenID::Keyword:
                // and/or
                RNS_NOT_IMPLEMENTED;
            default:
                return BinOp::None;
        }
    }
};

#define KW_DEF(x) #x
const char* keywords[] =
{
KW_DEF(return),
KW_DEF(if),
KW_DEF(else),
KW_DEF(for),
KW_DEF(while),
KW_DEF(break),
KW_DEF(continue),
};

#define native_type(x) #x
const char* native_types[] =
{
    "", // void type, @TODO: maybe modify?
    native_type(u8),
    native_type(u16),
    native_type(u32),
    native_type(u64),
    native_type(s8),
    native_type(s16),
    native_type(s32),
    native_type(s64),
    native_type(f32),
    native_type(f64),
};

const auto keyword_count = rns_array_length(keywords);
const auto native_type_count = rns_array_length(native_types);
const auto intrinsic_count = 0;
constexpr bool check()
{
    constexpr bool keyword_count_check = static_cast<u8>(KeywordID::Count) == keyword_count;
    constexpr bool native_type_count_check = static_cast<u8>(NativeTypeID::Count) == native_type_count;
    return keyword_count_check && native_type_count;
}
static_assert(check());

struct NameMatch
{
    TokenID token_id;
    union
    {
        KeywordID keyword;
        NativeTypeID native_type;
        IntrinsicID intrinsic;
    };
};

static inline NameMatch match_name(const char* name, u32 len)
{
    for (auto i = 0; i < keyword_count; i++)
    {
        const char* kw_str = keywords[i];
        auto kw_len = strlen(kw_str);
        if (kw_len == len && strncmp(kw_str, name, len) == 0)
        {
            return { .token_id = TokenID::Keyword, .keyword = static_cast<KeywordID>(i) };
        }
    }

    for (auto i = 0; i < native_type_count; i++)
    {
        const char* type_str = native_types[i];
        auto type_len = strlen(type_str);
        if (type_len == len && strncmp(type_str, name, len) == 0)
        {
            return { .token_id = TokenID::NativeType, .native_type = static_cast<NativeTypeID>(i) };
        }
    }
    
    for (auto i = 0; i < intrinsic_count; i++)
    {}

    return { .token_id = TokenID::Symbol };
}

TokenBuffer lex(Allocator* token_allocator, Allocator* name_allocator, const char* file, u32 file_size)
{
    s64 tokens_to_allocate = 64 + ((s64)file_size / 2);
    s64 chars_to_allocate_in_string_buffer = 64 + (tokens_to_allocate / 5);
    TokenBuffer token_buffer = {
        .ptr = new(token_allocator) Token[tokens_to_allocate],
        .len = 0,
        .cap = tokens_to_allocate,
        .file_size = file_size,
        .file = file,
        .sb = StringBuffer::create(name_allocator, chars_to_allocate_in_string_buffer),
    };


    Token* token = nullptr;
#if 0
    s64 freq;
    s64 start, end;
#endif

    for (u32 i = 0; i < file_size; i++)
    {
        char c = file[i];
        u32 start;
        u32 end;

        for (;;)
        {
            switch (c)
            {
                case '\n':
                    token_buffer.line_count++;
                    assert(token_buffer.line_count <= UINT16_MAX);
                case ' ':
                    c = file[++i];
                    break;
                default:
                    goto end_blank;
            }
        }
    end_blank:

        switch (c)
        {
            SymbolStart:
            {
                start = i;
                for (;;)
                {
                    switch (c)
                    {
                    SymbolEnd:
                        goto end_symbol;
                        default:
                            c = file[++i];
                    }
                }
            end_symbol:
                end = i;
                i--;
                u32 len = end - start;
                auto match = match_name(&token_buffer.file[start], len);
                Token* t = token_buffer.new_token(match.token_id, start, end, static_cast<u16>(token_buffer.line_count));
                switch (match.token_id)
                {
                    case TokenID::Keyword:
                        t->keyword = match.keyword;
                        break;
                    case TokenID::NativeType:
                        t->native_type = match.native_type;
                        break;
                        // @TODO: intrinsics should not be names at all, they should go under the intrinsic character (presumably '@'?) switch case
                    case TokenID::Intrinsic:
                        t->intrinsic = match.intrinsic;
                        break;
                    case TokenID::Symbol:
                        t->symbol = token_buffer.sb.append(&token_buffer.file[start], len);
                        break;
                    default:
                        RNS_UNREACHABLE;
                        break;
                }
                break;
            }
            NumberStart:
            {
                start = i;
                const auto number_buffer_max_digits = 32;
                char number_buffer[number_buffer_max_digits];
                auto number_buffer_len = 0;
                for (;;)
                {
                    switch (c)
                    {
                    NumberStart:
                        number_buffer[number_buffer_len++] = c;
                        assert(number_buffer_len + 1 <= number_buffer_max_digits);
                        c = file[++i];
                        break;
                        default:
                            goto end_number;
                    }
                }
            end_number:
                end = i;
                number_buffer[number_buffer_len] = 0;
                i--;
                Token* t = token_buffer.new_token(TokenID::IntegerLit, start, end, static_cast<u16>(token_buffer.line_count));
                char* dummy_ptr = nullptr;
                t->int_lit = strtoull(number_buffer, &dummy_ptr, 10);
                break;
            }
            case '\"':
            {
                start = i;

                do
                {
                    c = file[++i];
                } while (c != '\"');

                end = i + 1;
                Token* t = token_buffer.new_token(TokenID::StringLit, start, end, static_cast<u16>(token_buffer.line_count));
                s64 string_len = (i - (start + 1));
                t->str_lit = token_buffer.sb.append(&token_buffer.file[start], string_len);
                break;
            }
            case '\'':
            {
                start = i;
                char char_lit = file[++i];
                end = ++i;

                Token* t = token_buffer.new_token(TokenID::CharLit, start, end, static_cast<u16>(token_buffer.line_count));
                t->char_lit = char_lit;
                break;
            }
            case '\0':
                break;
            case '\t':
            case '\r':
                assert(false);
                break;
            default:
                (void)token_buffer.new_token(static_cast<TokenID>(c), (start = i), (end = (i + 1)), static_cast<u16>(token_buffer.line_count));
                break;
        }
    }
    token_buffer.line_count++;

#if 0
    f64 ns = ((f64)(end - start) * 1000 * 1000 * 1000 / freq);
    printf("Lexed in %f ns. %f MB/s\n", ns, (u64)file_size / ((f64)(end - start) * 1000 * 1000 / freq) );
#endif

    return token_buffer;
}

const u32 no_value = UINT32_MAX;

enum class NodeType
{
    IntLit,
    BinOp,
    Ret,
    SymName,
    Type,
    VarDecl,
    ArgList,
    Struct,
    Enum,
    Union,
    Function,
};

struct IntLiteral
{
    u64 lit;
};

struct BinaryOp
{
    BinOp op;
    u32 left;
    u32 right;
};

struct RetExpr
{
    u32 ret_value;
};

struct SymName
{
    s64 len;
    char* ptr;
};

struct Type
{
    u8 is_native;
    union
    {
        NativeTypeID native;
        u32 non_native;
    };
};

struct VarDecl
{
    u32 name;
    u32 type;
    u32 value;
};

struct NodeList
{
    u32* ptr;
    s64 len;
    s64 cap;

    s64 variable_append(u32 node_id)
    {
        assert(len + 1 < cap);
        s64 index = len++;
        ptr[index] = node_id;
        return index;
    }
};

struct Struct
{
    NodeList fields;
    u32 name;
};

struct Union
{
    NodeList fields;
    u32 name;
};

struct Enum
{
    NodeList values;
    u32 name;
};

struct Function
{
    NodeList arg_list;
    NodeList statements;
    NodeList variables;
    Type ret_type;
    u32 name;
    // @TODO: function optionals (inline, calling convention, etc.)
};

struct Node
{
    NodeType type;
    union
    {
        IntLiteral int_lit;
        BinaryOp bin_op;
        RetExpr ret_expr;
        SymName sym_name;
        Type type_expr;
        VarDecl var_decl;
        Enum enum_decl;
        Struct struct_decl;
        Union union_decl;
        Function function_decl;
    };
};

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

struct NodeBuffer
{
    Node* ptr;
    s64 len;
    s64 cap;
    Allocator* allocator;

    static NodeBuffer create(Allocator* allocator, s64 size = 32)
    {
        NodeBuffer nb
        {
            .ptr = new (allocator) Node[size],
            .len = 0,
            .cap = size,
            .allocator = allocator,
        };

        return nb;
    }

    Node* append(NodeType type)
    {
        assert(len + 1 <= cap);
        Node* result = &ptr[len++];
        result->type = type;
        return result;
    }

    u32 get_id(Node* node)
    {
        u64 result = node - ptr;
        assert(result <= UINT32_MAX);
        return (u32)result;
    }

    Node* get(u32 id)
    {
        if (id < len)
        {
            return ptr + id;
        }
        return nullptr;
    }
};

struct NodeIDBuffer
{
    // @Info: this is the node pointer for the args
    u32* ptr;
    s64 len;
    s64 cap;

    static NodeIDBuffer create(Allocator* allocator)
    {
        NodeIDBuffer ab =
        {
            .ptr = (u32*)(allocator->pool.ptr + allocator->pool.used),
            .len = 0,
            .cap = (allocator->pool.cap - allocator->pool.used) / (s64)sizeof(u32),
        };

        return ab;
    }

    // @Info: this function is a bit tricky since it returns an index inside this buffer, not the actual value.
    // The user is responsible for getting the offset of the first entry added in question and substracting it
    // from the buffer final free address
    // @SingleThreaded
    s64 append(u32 value)
    {
        assert(len + 1 < cap);
        s64 index = len++;
        ptr[index] = value;
        return index;
    }
};

struct ModuleParser
{
    TokenParser tp;
    NodeBuffer nb;
    NodeIDBuffer nib;
    NodeIDBuffer tldb;
    u32 current_scope;

    char error_message[255];

    inline bool failed()
    {
        return *error_message;
    }
};

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

ModuleParser parse(TokenBuffer* tb, Allocator* node_allocator, Allocator* node_id_allocator, Allocator* tld_allocator)
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

const s64 ir_no_value = INT64_MAX;

constexpr const static u32 sizes[static_cast<u32>(NativeTypeID::Count)] = {
    0,
    sizeof(u8), sizeof(u16), sizeof(u32), sizeof(u64),
    sizeof(s8), sizeof(s16), sizeof(s32), sizeof(s64),
    sizeof(f32), sizeof(f64),
};

struct IRFunction
{

};

enum class InstructionID
{
    Invalid,
    Add,
    Assign,
    Decl,
    Ret,
};

enum class OperandType
{
    Invalid,
    New,
    StackAddress,
    IntLit,
};

struct Stack
{
    s64 id;
};

struct PrintBuffer
{
    char ptr[512];
};

const char* type_to_string(NativeTypeID id)
{
    switch (id)
    {
        rns_case_to_str(NativeTypeID::None);
        rns_case_to_str(NativeTypeID::U8);
        rns_case_to_str(NativeTypeID::U16);
        rns_case_to_str(NativeTypeID::U32);
        rns_case_to_str(NativeTypeID::U64);
        rns_case_to_str(NativeTypeID::S8);
        rns_case_to_str(NativeTypeID::S16);
        rns_case_to_str(NativeTypeID::S32);
        rns_case_to_str(NativeTypeID::S64);
        rns_case_to_str(NativeTypeID::F32);
        rns_case_to_str(NativeTypeID::F64);
        default:
            RNS_NOT_IMPLEMENTED;
            return nullptr;
    }
}

const char* operand_type_to_string(OperandType type)
{
    switch (type)
    {
        rns_case_to_str(OperandType::Invalid);
        rns_case_to_str(OperandType::New);
        rns_case_to_str(OperandType::StackAddress);
        rns_case_to_str(OperandType::IntLit);
        default:
            RNS_NOT_IMPLEMENTED;
            return nullptr;
    }
}

struct Value
{
    NativeTypeID type;
    OperandType op_type;
    union
    {
        IntLiteral int_lit;
        Stack stack;
    };

    PrintBuffer to_string(s64 id)
    {
        PrintBuffer bf;
        auto len = 0;
        len += sprintf(bf.ptr, "ID: %lld %s %s ", id, type_to_string(type), operand_type_to_string(op_type));

        switch (op_type)
        {
            case OperandType::Invalid:
                sprintf(bf.ptr + len, "Invalid");
                break;
            case OperandType::IntLit:
                sprintf(bf.ptr + len, "Int literal %llu", int_lit.lit);
                break;
            case OperandType::StackAddress:
                sprintf(bf.ptr + len, "Variable addressing: %lld", stack.id);
                break;
            case OperandType::New:
                sprintf(bf.ptr + len, "Variable declaration");
                break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return bf;
    }
};

struct ValueBuffer
{
    Value* ptr;
    s64 len;
    s64 cap;

    static ValueBuffer create(Allocator* allocator)
    {
        ValueBuffer buffer = {
            .ptr = reinterpret_cast<Value*>(allocator->pool.ptr + allocator->pool.used),
            .len = 0,
            .cap = (allocator->pool.cap - allocator->pool.used) / static_cast<s64>(sizeof(ValueBuffer)),
        };

        return buffer;
    }

    inline s64 append(Value value)
    {
        assert(len + 1 < cap);
        s64 index = len++;
        ptr[index] = value;
        return index;
    }

    inline Value get(s64 id)
    {
        return ptr[id];
    }
};

PrintBuffer value_id_to_string(ValueBuffer* vb, s64 id)
{
    PrintBuffer bf;
    if (id != ir_no_value)
    {
        Value v = vb->get(id);
        bf = v.to_string(id);
    }
    else
    {
        strcpy(bf.ptr, "NULL");
    }
    return bf;
}

struct Instruction
{
    s64 operands[2];
    s64 result;
    InstructionID id;

    void print(ValueBuffer* vb)
    {
        const char* op1_string = value_id_to_string(vb, operands[0]).ptr;
        const char* op2_string = value_id_to_string(vb, operands[1]).ptr;
        const char* result_string = value_id_to_string(vb, result).ptr;
        printf("Instruction: %s\n\tOperand 1: %s\n\tOperand 2: %s\n\tResult: %s\n\n", id_to_string(), op1_string, op2_string, result_string);
    }

    const char* id_to_string()
    {
        switch (id)
        {
            rns_case_to_str(InstructionID::Add);
            rns_case_to_str(InstructionID::Assign);
            rns_case_to_str(InstructionID::Decl);
            rns_case_to_str(InstructionID::Ret);
            default:
                RNS_NOT_IMPLEMENTED;
                return nullptr;
        }
    }
};

struct InstructionBuffer
{
    Instruction* ptr;
    s64 len;
    s64 cap;

    static InstructionBuffer create(Allocator* allocator)
    {
        InstructionBuffer ib =
        {
            .ptr = (Instruction*)(allocator->pool.ptr + allocator->pool.used),
            .len = 0,
            .cap = (allocator->pool.cap - allocator->pool.used) / (s64)sizeof(Instruction),
        };

        return ib;
    }

    inline s64 append(Instruction instruction)
    {
        assert(len + 1 < cap);
        s64 index = len++;
        ptr[index] = instruction;
        return index;
    }
};

struct IR
{
    InstructionBuffer ib;
    ValueBuffer vb;
};

static inline Instruction instr(InstructionID id, s64 operand1 = INT64_MAX, s64 operand2 = INT64_MAX, s64 result = INT64_MAX)
{
    Instruction i = {
        .operands = { operand1, operand2 },
        .result = result,
        .id = id,
    };

    return i;
}

struct ScopedStorage
{
    s64 ir_local_variables[100];
    u32 ast_local_variables[100];
    s64 local_variable_count;
};

s64 transform_node_to_ir(ModuleParser* m, IR* ir, u32 node_id, ScopedStorage* scoped_storage)
{
    assert(node_id != no_value);
    Node* node = m->nb.get(node_id);
    assert(node);

    auto type = node->type;

    switch (type)
    {
        case NodeType::IntLit:
        {
            // @TODO: change hardcoded value
            Value int_lit = {};
            int_lit.type = NativeTypeID::S32;
            int_lit.op_type = OperandType::IntLit;
            int_lit.int_lit = node->int_lit;
            return ir->vb.append(int_lit);
        } break;
        case NodeType::SymName:
        {
            const char* var_name = node->sym_name.ptr;
            assert(var_name);

            for (s64 i = 0; i < scoped_storage->local_variable_count; i++)
            {
                u32 ast_local = scoped_storage->ast_local_variables[i];
                Node* ast_local_node = m->nb.get(ast_local);
                assert(ast_local_node);
                if (ast_local_node->type == NodeType::VarDecl)
                {
                    u32 ast_local_name_id = ast_local_node->var_decl.name;
                    Node* ast_local_name_node = m->nb.get(ast_local_name_id);
                    assert(ast_local_name_node);
                    assert(ast_local_name_node->type == NodeType::SymName);
                    const char* local_name = ast_local_name_node->sym_name.ptr;
                    assert(local_name);

                    if (strcmp(local_name, var_name) == 0)
                    {
                        return scoped_storage->ir_local_variables[i];
                    }
                }
            }
            assert(0);
            return {};
        } break;
        case NodeType::BinOp:
        {
            Value op_result;
            op_result.type = NativeTypeID::S32;
            op_result.op_type = OperandType::New;
            s64 op_result_id = ir->vb.append(op_result);
            s64 index = scoped_storage->local_variable_count++;
            scoped_storage->ir_local_variables[index] = op_result_id;
            scoped_storage->ast_local_variables[index] = node_id;
            ir->ib.append(instr(InstructionID::Decl, op_result_id, ir_no_value, op_result_id));

            u32 left_node_id = node->bin_op.left;
            u32 right_node_id = node->bin_op.right;
            s64 left_value_id = transform_node_to_ir(m, ir, left_node_id, scoped_storage);
            s64 right_value_id = transform_node_to_ir(m, ir, right_node_id, scoped_storage);

            auto op = node->bin_op.op;

            switch (op)
            {
                case BinOp::Plus:
                    ir->ib.append(instr(InstructionID::Add, left_value_id, right_value_id, op_result_id));
                    break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            return op_result_id;
        } break;
        case NodeType::VarDecl:
        {
            auto type_node_id = node->var_decl.type;
            Node* type_node = m->nb.get(type_node_id);
            assert(type_node);
            assert(type_node->type == NodeType::Type);
            assert(type_node->type_expr.is_native);

            Value var_value;
            var_value.type = type_node->type_expr.native;
            var_value.op_type = OperandType::New;
            s64 var_operand_id = ir->vb.append(var_value);
            s64 index = scoped_storage->local_variable_count++;
            scoped_storage->ir_local_variables[index] = var_operand_id;
            scoped_storage->ast_local_variables[index] = node_id;
            ir->ib.append(instr(InstructionID::Decl, var_operand_id, ir_no_value, var_operand_id));

            u32 value_id = node->var_decl.value;
            s64 expression_id;

            if (value_id != no_value)
            {
                expression_id = transform_node_to_ir(m, ir, value_id, scoped_storage);
            }
            else
            {
                Value zero_lit = { .type = NativeTypeID::S32, .op_type = OperandType::IntLit };
                expression_id = ir->vb.append(zero_lit);
            }
            ir->ib.append(instr(InstructionID::Assign, var_operand_id, expression_id, var_operand_id));
            return var_operand_id;
        } break;
        case NodeType::Ret:
        {
            u32 ret_node_id = node->ret_expr.ret_value;
            s64 ret_value_id = transform_node_to_ir(m, ir, ret_node_id, scoped_storage);
            ir->ib.append(instr(InstructionID::Ret, ret_value_id));
            return ret_value_id;
        } break;
        default:
            RNS_NOT_IMPLEMENTED;
            return {};
    }
}


IR generate_ir(ModuleParser* m, Allocator* instruction_allocator, Allocator* value_allocator)
{
    IR ir = {
        .ib = InstructionBuffer::create(instruction_allocator),
        .vb = ValueBuffer::create(value_allocator),
    };

    for (auto i = 0; i < m->tldb.len; i++)
    {
        ScopedStorage scoped_storage = {};
        u32 tld_id = m->tldb.ptr[i];
        Node* tld_node = m->nb.get(tld_id);
        assert(tld_node);
        assert(tld_node->type == NodeType::Function);

        for (auto i = 0; i < tld_node->function_decl.statements.len; i++)
        {
            auto st_id = tld_node->function_decl.statements.ptr[i];
            transform_node_to_ir(m, &ir, st_id, &scoped_storage);
        }
    }

    return ir;
}

namespace Interp
{
    struct Variable
    {
        s32 value;
    };

    struct Runtime
    {
        Interp::Variable variables[100];
        s64 ir_ids_reserved[30];
        s64 var_count = 0;
    };
}

Interp::Variable* get_local_variable(Interp::Runtime* runtime, s64 id)
{
    for (auto i = 0; i < runtime->var_count; i++)
    {
        if (id == runtime->ir_ids_reserved[i])
        {
            return &runtime->variables[i];
        }
    }

    return nullptr;
}
// @TODO: for now, we are only dealing with s32 values
s32 resolve_index(IR* ir, Interp::Runtime* runtime, s64 index)
{
    Interp::Variable* variable = get_local_variable(runtime, index);
    if (variable)
    {
        return variable->value;
    }

    auto value = ir->vb.get(index);

    switch (value.type)
    {
        case NativeTypeID::S32:
            return (s32)value.int_lit.lit;
        default:
            RNS_NOT_IMPLEMENTED;
            return 0;
    }
}


void interpret(IR* ir)
{
    s32 result = 0;
    auto instruction_count = ir->ib.len;

    Interp::Runtime runtime = {};
    memset(&runtime.ir_ids_reserved, 0xcccccccc, sizeof(runtime.ir_ids_reserved));

    for (auto i = 0; i < instruction_count; i++)
    {
        auto instruction = ir->ib.ptr[i];

        switch (instruction.id)
        {
            case InstructionID::Decl:
            {
                // This should be the equivalent of stack reserving
                auto id = instruction.operands[0];
                runtime.ir_ids_reserved[runtime.var_count++] = id;
            } break;
            case InstructionID::Add:
            {
                auto op1_id = instruction.operands[0];
                auto op2_id = instruction.operands[1];
                auto result_id = instruction.result;
                s32 op1_value = resolve_index(ir, &runtime, op1_id);
                s32 op2_value = resolve_index(ir, &runtime, op2_id);
                auto* result_variable = get_local_variable(&runtime, result_id);
                assert(result_variable);
                result_variable->value = op1_value + op2_value;
            } break;
            case InstructionID::Assign:
            {
                auto op1_id = instruction.operands[0];
                auto op2_id = instruction.operands[1];
                auto result_id = instruction.result;
                s32 op1_value = resolve_index(ir, &runtime, op1_id);
                s32 op2_value = resolve_index(ir, &runtime, op2_id);
                auto* result_variable = get_local_variable(&runtime, result_id);
                assert(result_variable);
                result_variable->value = op2_value;
            } break;
            case InstructionID::Ret:
            {
                auto op1_id = instruction.operands[0];
                s32 op1_value = resolve_index(ir, &runtime, op1_id);
                printf("Function returned %d\n", op1_value);
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }
}

void jit(IR* ir)
{

}

void compile(IR* ir)
{

}

#include "file.h"

static inline DebugAllocator create_allocator(s64 size)
{
    void* address = virtual_alloc(nullptr, size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 });
    assert(address);
    DebugAllocator allocator = DebugAllocator::create(address, size);
    return allocator;
}

#include <RNS/os_internal.h>
/*
typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;
  */

struct FileBuffer
{
    u8* ptr;
    s64 len;
    s64 cap;

    static FileBuffer create(Allocator* allocator)
    {
        FileBuffer ab =
        {
            .ptr = reinterpret_cast<u8*>(allocator->pool.ptr + allocator->pool.used),
            .len = 0,
            .cap = (allocator->pool.cap - allocator->pool.used) / (s64)sizeof(u8),
        };

        return ab;
    }

    template <typename T>
    T* append(T element) {
        s64 size = sizeof(T);
        assert(len + size < cap);
        auto index = len;
        memcpy(&ptr[index], &element, size);
        len += size;
        return reinterpret_cast<T*>(&ptr[index]);
    }

    template <typename T>
    T* append(T* element)
    {
        s64 size = sizeof(T);
        assert(len + size < cap);
        auto index = len;
        memcpy(&ptr[index], element, size);
        len += size;
        return reinterpret_cast<T*>(&ptr[index]);
    }

    template<typename T>
    T* append(T* arr, s64 array_len)
    {
        s64 size = sizeof(T) * array_len;
        assert(len + size < cap);
        auto index = len;
        memcpy(&ptr[index], arr, size);
        len += size;
        return reinterpret_cast<T*>(&ptr[index]);
    }
};

enum class ImageDirectoryIndex
{
    Export,
    Import,
    Resource,
    Exception,
    Security,
    Relocation,
    Debug,
    Architecture,
    GlobalPtr,
    TLS,
    LoadConfig,
    BoundImport,
    IAT,
    Delay,
    CLR,
};

void write_executable()
{
    DebugAllocator file_allocator = create_allocator(RNS_MEGABYTE(1));
    FileBuffer file_buffer = FileBuffer::create(&file_allocator);
    IMAGE_DOS_HEADER dos_header
    {
        .e_magic = IMAGE_DOS_SIGNATURE,
        .e_lfanew = sizeof(IMAGE_DOS_HEADER),
    };
    file_buffer.append(&dos_header);
    file_buffer.append((u32)IMAGE_NT_SIGNATURE);

    IMAGE_FILE_HEADER image_file_header = {
        .Machine = IMAGE_FILE_MACHINE_AMD64,
        .NumberOfSections = 2,
        .TimeDateStamp = 0x5EF48E56,
        .SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64),
        .Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE,
    };
    file_buffer.append(&image_file_header);

    IMAGE_OPTIONAL_HEADER64 oh = {
        .Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC,
        .SizeOfCode = 0x200, // @TODO: change
        .SizeOfInitializedData = 0x400, // @TODO: change (global data)
        .SizeOfUninitializedData = 0x0, // @TODO: what's the difference?
        .AddressOfEntryPoint = 0x1000, // @TODO: change
        .BaseOfCode = 0x1000,// @TODO: change
        .ImageBase = 0x0000000140000000,
        .SectionAlignment = 0x1000,
        .FileAlignment = 0x200,
        .MajorOperatingSystemVersion = 6, // @TODO: change
        .MinorOperatingSystemVersion = 0, // @TODO: change
        .MajorImageVersion = 0,
        .MinorImageVersion = 0,
        .MajorSubsystemVersion = 6,
        .MinorSubsystemVersion = 0,
        .Win32VersionValue = 0,
        .SizeOfImage = 0x3000,
        .SizeOfHeaders = 0,
        .CheckSum = 0,
        .Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI,
        .DllCharacteristics = IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA | IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE,
        .SizeOfStackReserve = 0x100000,
        .SizeOfStackCommit = 0x1000,
        .SizeOfHeapReserve = 0x100000,
        .SizeOfHeapCommit = 0x1000,
        .NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES,
    };
    auto* optional_header = file_buffer.append(&oh);

    IMAGE_SECTION_HEADER tsh = {
        .Name = ".text",
        .Misc = {.VirtualSize = 0x10}, // sizeof machine code in bytes
        .VirtualAddress = optional_header->BaseOfCode, // calculate this
        .SizeOfRawData = optional_header->SizeOfCode, // calculate this
        .PointerToRawData = 0,
        .Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE,
    };
    auto* text_section_header = file_buffer.append(&tsh);

    IMAGE_SECTION_HEADER rdsh = {
      .Name = ".rdata",
      .Misc = 0x10,
      .VirtualAddress = 0x2000, // calculate this
      .SizeOfRawData = 0x200, // calculate this
      .PointerToRawData = 0,
      .Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,
    };
    auto* rdata_section_header = file_buffer.append(&rdsh);

    // end list with a NULL header
    file_buffer.append(IMAGE_SECTION_HEADER{});

    optional_header->SizeOfHeaders = static_cast<DWORD>(align(file_buffer.len, optional_header->FileAlignment));
    file_buffer.len = optional_header->SizeOfHeaders;

    IMAGE_SECTION_HEADER* sections[] = { text_section_header, rdata_section_header };
    auto section_offset = optional_header->SizeOfHeaders;
    for (auto i = 0; i < rns_array_length(sections); i++)
    {
        sections[i]->PointerToRawData = section_offset;
        section_offset += sections[i]->SizeOfRawData;
    }

    // .text section
    file_buffer.len = text_section_header->PointerToRawData;

    u8 sub_rsp_28_bytes[] = { 0x48, 0x83, 0xEC, 0x28, 0xB9, 0x2A, 0x00, 0x00, 0x00, };
    file_buffer.append(sub_rsp_28_bytes, rns_array_length(sub_rsp_28_bytes));
    u8 call_bytes[] = { 0xFF, 0x15, 0xF1, 0x0F, 0x00, 0x00, };
    file_buffer.append(call_bytes, rns_array_length(call_bytes));
    u8 int3_bytes[] = { 0xCC };
    file_buffer.append(int3_bytes, rns_array_length(int3_bytes));

    const char* functions[] = { "ExitProcess" };

    struct Import
    {
        const char* library_name;
        const char** functions;
        u32 function_count;
    } kernel32 = {
        .library_name = "kernel32.dll",
        .functions = functions,
        .function_count = rns_array_length(functions),
    };

    (void)kernel32;

#define file_offset_to_rva(_section_header_)\
    (static_cast<DWORD>(file_buffer.len) - (_section_header_)->PointerToRawData + (_section_header_)->VirtualAddress)

    // .rdata section
    file_buffer.len = rdata_section_header->PointerToRawData;

    DWORD IAT_RVA = file_offset_to_rva(rdata_section_header);
    u64* exit_process_name_rva = file_buffer.append((u64)0);
    file_buffer.append((u64)0);
    optional_header->DataDirectory[static_cast<u32>(ImageDirectoryIndex::Import)].VirtualAddress = static_cast<DWORD>(IAT_RVA);
    optional_header->DataDirectory[static_cast<u32>(ImageDirectoryIndex::Import)].Size = static_cast<DWORD>(file_buffer.len) - rdata_section_header->PointerToRawData;

    u8 exit_process_name[] = "ExitProcess";
    u8 library_name[] = "KERNEL32.dll";

    DWORD kernel32_name_RVA = file_offset_to_rva(rdata_section_header);
    {
        file_buffer.append(library_name, rns_array_length(library_name));
        s64 to_be_added = align(rns_array_length(library_name), 2) - rns_array_length(library_name);
        file_buffer.len += to_be_added;
    }

    auto import_directory_RVA = file_offset_to_rva(rdata_section_header);
    optional_header->DataDirectory[static_cast<u32>(ImageDirectoryIndex::Import)].VirtualAddress = static_cast<DWORD>(import_directory_RVA);

    IMAGE_IMPORT_DESCRIPTOR iid = {
        .Name = kernel32_name_RVA,
        .FirstThunk = IAT_RVA,
    };
    auto* image_import_descriptor = file_buffer.append(&iid);
    DWORD image_thunk_RVA = file_offset_to_rva(rdata_section_header);
    image_import_descriptor->OriginalFirstThunk = image_thunk_RVA;
    
    u64* image_thunk = file_buffer.append((u64)0);
    file_buffer.append((u64)0);

    {
        DWORD import_name_rva = file_offset_to_rva(rdata_section_header);

        *exit_process_name_rva = import_name_rva;
        *image_thunk = import_name_rva;

        file_buffer.append((u16)0);

        file_buffer.append(exit_process_name, rns_array_length(exit_process_name));
        s64 to_be_added = align(rns_array_length(exit_process_name), 2) - rns_array_length(exit_process_name);
        file_buffer.len += to_be_added;
    }

    optional_header->DataDirectory[static_cast<u32>(ImageDirectoryIndex::Import)].Size = file_offset_to_rva(rdata_section_header) - import_directory_RVA;
    file_buffer.len = (u64)rdata_section_header->PointerToRawData + (u64)rdata_section_header->SizeOfRawData;

    auto file_handle = CreateFileA("test.exe", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(file_handle != INVALID_HANDLE_VALUE);

    DWORD bytes_written = 0;
    WriteFile(file_handle, file_buffer.ptr, (DWORD)file_buffer.len, &bytes_written, 0);
    CloseHandle(file_handle);
}

s32 rns_main(s32 argc, char* argv[])
{
    DebugAllocator lexer_allocator = create_allocator(RNS_MEGABYTE(100));
    DebugAllocator name_allocator = create_allocator(RNS_MEGABYTE(20));

    TokenBuffer tb = lex(&lexer_allocator, &name_allocator, file, rns_array_length(file));

    DebugAllocator node_allocator = create_allocator(RNS_MEGABYTE(200));
    DebugAllocator node_id_allocator = create_allocator(RNS_MEGABYTE(100));
    DebugAllocator tld_allocator = create_allocator(RNS_MEGABYTE(100));
    
    ModuleParser module_parser = parse(&tb, &node_allocator, &node_id_allocator, &tld_allocator);
    if (module_parser.failed())
    {
        printf("Parsing failed! Error: %s\n", module_parser.error_message);
        return -1;
    }
    else
    {
        printf("Parsing finished successfully!\n");
    }

    DebugAllocator instruction_allocator = create_allocator(RNS_MEGABYTE(300));
    DebugAllocator value_allocator = create_allocator(RNS_MEGABYTE(300));
    IR ir = generate_ir(&module_parser, &instruction_allocator, &value_allocator);
    //for (auto i = 0; i < ir.ib.len; i++)
    //{
    //    auto instruction = ir.ib.ptr[i];
    //    instruction.print(&ir.vb);
    //}

    interpret(&ir);

    write_executable();
    return 0;
}