#pragma once

#include <RNS/types.h>
#include <RNS/data_structures.h>
#include <RNS/os.h>

using RNS::Allocator;

extern "C" s32 printf(const char*, ...);

enum class UnaryOp
{
    None,
    AddressOf,
    PointerDereference,
};

enum class BinOp
{
    None,
    Plus,
    Minus,
    Mul,
    VariableDecl,
    Assign,
    Cmp_Equal,
    Cmp_NotEqual,
    Cmp_LessThan,
    Cmp_GreaterThan,
    Cmp_LessThanOrEqual,
    Cmp_GreaterThanOrEqual,
    Count,
};

enum class ValueType
{
    RValue,
    LValue,
};

inline bool is_cmp_binop(BinOp binop)
{
    return binop >= BinOp::Cmp_Equal && binop <= BinOp::Cmp_GreaterThanOrEqual;
}

enum class OperatorPrecedenceRule
{
    Unary_ArraySubscript_FunctionCall_MemberAccess_CompoundLiteral,
    Unary_LogicalNot_BitwiseNot_Cast_Dereference_AddressOf_SizeOf_AlignOf,
    Binary_Multiplication_Division_Remainder,
    Binary_Addition_Substraction,
    Binary_BitwiseLeftShit_BitwiseRightShift,
    Binary_RelationalOperators,
    Binary_RelationalOperators_EqualNotEqual,
    Binary_BitwiseAND,
    Binary_BitwiseXOR,
    Binary_BitwiseOR,
    Binary_LogicalAND,
    Binary_LogicalOR,
    Binary_AssignmentOperators,
};

struct OperatorPrecedence
{
    OperatorPrecedenceRule rules[(u32)BinOp::Count];
};

inline constexpr OperatorPrecedence initialize_precedence_rules()
{
#define precedence_rule(_operator_, _rule_) operator_precedence.rules[(u32)BinOp:: ## _operator_] = OperatorPrecedenceRule:: ## _rule_
    OperatorPrecedence operator_precedence;
    precedence_rule(Plus, Binary_Addition_Substraction);
    precedence_rule(Minus, Binary_Addition_Substraction);
    precedence_rule(Mul, Binary_Multiplication_Division_Remainder);
    precedence_rule(Assign, Binary_AssignmentOperators);
    precedence_rule(Cmp_Equal, Binary_RelationalOperators_EqualNotEqual);
    precedence_rule(Cmp_NotEqual, Binary_RelationalOperators_EqualNotEqual);
    precedence_rule(Cmp_LessThan, Binary_RelationalOperators);
    precedence_rule(Cmp_GreaterThan, Binary_RelationalOperators);
    precedence_rule(Cmp_LessThanOrEqual, Binary_RelationalOperators);
    precedence_rule(Cmp_GreaterThanOrEqual, Binary_RelationalOperators);
#undef precedence_rule

    return operator_precedence;
}

inline OperatorPrecedence operator_precedence = initialize_precedence_rules();

namespace Typing
{
    enum class TypeID : u8
    {
        // Primitive types
        HalfType = 0,
        BFloatType,
        FloatType,
        DoubleType,
        X86_FP80Type,
        FP128Type,
        PPC_FP128Type,
        VoidType,
        LabelType,
        MetadataType,
        X86_MMX_Type,
        TokenType,

        // Derived types
        IntegerType,
        FunctionType,
        PointerType,
        StructType,
        ArrayType,
        FixedVectorType,
        ScalableVectorType,
    };

    struct Type;
    using TypeBuffer = RNS::Buffer<Type>;

    struct FloatType
    {

    };

    struct DoubleType
    {
    };

    struct IntegerType
    {
        u16 bits;
        bool is_signed;
    };

    struct DataStructureType
    {
        s64 arg_count;
        RNS::String* field_names;
        Type* field_types;
    };

    struct StructType : public DataStructureType
    {
    };

    struct UnionType : public DataStructureType
    {
    };

    struct EnumType : public DataStructureType
    {
    };

    using TypeRefBuffer = RNS::Buffer<Type*>;
    
    struct FunctionType
    {
        TypeRefBuffer arg_types;
        Type* ret_type;
    };

    struct PointerType
    {
        Type* appointee;
        // @TODO: come back here if we want to diminish pointers memory footprint
        // u16 bits;
    };

    struct Type
    {
        TypeID id;
        RNS::String name;
        union
        {
            IntegerType integer_t;
            StructType struct_t;
            UnionType union_t;
            EnumType enum_t;
            FunctionType function_t;
            PointerType pointer_t;
        };

        static TypeBuffer type_buffer;
        static void init(Allocator* allocator, s64 count);
        static Type* get(RNS::String name);
        static Type* get_pointer_type(Type* type);
        static Type* get_void_type();
        static Type* get_integer_type(u16 bits, bool is_signed);
        static Type* get_label();
        static Type* get_bool_type();
    };

    struct ConstantInt
    {
        u64 lit;
        u16 bit_count;
        bool is_signed;
        u8 padding[5];
    };
    static_assert(sizeof(ConstantInt) == 2 * sizeof(s64));
}
using namespace Typing;
namespace Lexer
{
    enum class TokenID : u8
    {
        /* These are names */
        Keyword,
        Type,
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


    enum class IntrinsicID : u8
    {
        Foo,
    };

    struct Token
    {
        union
        {
            u64 int_lit;
            f64 float_lit;
            char char_lit;
            char* str_lit;
            char* symbol;
            KeywordID keyword;
            Type* type;
            IntrinsicID intrinsic;
        };

        u64 start;
        u32 line;
        u32 column;
        u32 offset;

        TokenID id;

        inline TokenID get_id()
        {
            return static_cast<TokenID>(id);
        }

        void print_token_id()
        {
            auto token_id = static_cast<TokenID>(id);

            switch (token_id)
            {
                case Lexer::TokenID::Keyword:
                    printf("Keyword\n");
                    break;
                case Lexer::TokenID::Type:
                    printf("Type\n");
                    break;
                case Lexer::TokenID::Intrinsic:
                    printf("Intrinsic\n");
                    break;
                case Lexer::TokenID::Symbol:
                    printf("Symbol\n");
                    break;
                case Lexer::TokenID::StringLit:
                    printf("String lit\n");
                    break;
                case Lexer::TokenID::FloatLit:
                    printf("Float lit\n");
                    break;
                case Lexer::TokenID::CharLit:
                    printf("Char lit\n");
                    break;
                case Lexer::TokenID::IntegerLit:
                    printf("Integer lit\n");
                    break;
                default:
                    printf("Token: %c\n", id);
                    break;
            }
        }
    };
    static_assert(sizeof(Token) == 4 * (sizeof(u64)));

    struct LexerResult
    {
        Token* ptr;
        s64 len;
    };

}
using namespace Lexer;

namespace AST
{
    inline const u32 no_value = UINT32_MAX;

    struct Node;
    using NodeSlice  = RNS::Buffer<Node>;
    using FieldNames = RNS::Slice<RNS::String>;
    using FieldTypes = RNS::Slice<Type>;

    enum class NodeType
    {
        IntLit,
        UnaryOp,
        BinOp,
        Ret,
        VarDecl,
        VarExpr,
        InvokeExpr,
        Conditional,
        Block,
        Loop,
        Break,
        Function,
        TypeExpr,
    };

    enum class UnaryOpType
    {
        Prefix,
        Postfix,
    };

    struct UnaryOperation
    {
        UnaryOp type;
        UnaryOpType pos;
        Node* node;
    };

    struct BinaryOperation
    {
        Node* left;
        Node* right;
        BinOp op;
        bool parenthesis;
    };

    struct RetExpr
    {
        Node* expr;
    };

    struct VarExpr
    {
        Node* mentioned;
    };


    using NodeRefBuffer = RNS::Buffer<Node*>;
    struct Block
    {
        enum class Type
        {
            None,
            LoopPrefix,
            LoopBody,
            LoopPostfix,
            IfBlock,
            ElseBlock,
            Function,
        };
        NodeRefBuffer statements;
        Type type;
    };

    struct VarDecl
    {
        RNS::String name;
        // @TODO: should consider fully integrating the type in here
        Type* type;
        Node* value;
        Node* scope;
        void* backend_ref;
        bool is_fn_arg;
    };

    struct Conditional
    {
        Node* condition;
        Node* if_block;
        Node* else_block;
        void* exit_block;
    };

    struct Loop
    {
        Node* prefix;
        Node* body;
        Node* postfix;
        void* exit_block;
        void* continue_block;
    };

    struct Break
    {
        Node* target;
        Node* origin;
    };

    struct FunctionDeclaration
    {
        NodeRefBuffer scope_blocks;
        NodeRefBuffer arguments;
        NodeRefBuffer variables;
        RNS::String name;
        Node* type;
    };

    struct InvokeExpr
    {
        NodeRefBuffer arguments;
        Node* expr;
    };

    struct Node
    {
        Node* parent;
        NodeType type;
        ValueType value_type;
        union
        {
            ConstantInt int_lit;
            UnaryOperation unary_op;
            BinaryOperation bin_op;
            RetExpr ret;
            VarDecl var_decl;
            VarExpr var_expr;
            InvokeExpr invoke_expr;
            Block block;
            Conditional conditional;
            Loop loop;
            Break break_;
            FunctionDeclaration function;
            Type type_expr;
        };
    };

    // @TODO: we are transforming this to inherit from the RNS-lib buffer. This may have some drawbacks. Come back to this when the compiler is more mature.
    struct NodeBuffer : public RNS::Buffer<Node>
    {
        Node* append(NodeType type, Node* parent)
        {
            if (type != NodeType::Function)
            {
                assert(parent);
            }
            assert(len + 1 <= cap);
            Node* result = &ptr[len++];
            result->type = type;
            result->parent = parent;
            if (type == NodeType::VarDecl)
            {
                result->value_type = ValueType::LValue;
            }

            return result;
        }
    };

    using FunctionDeclarationBuffer = NodeRefBuffer;
    using StructBuffer = NodeRefBuffer;
    using UnionBuffer = NodeRefBuffer;
    using EnumBuffer = NodeRefBuffer;
    using FunctionTypeBuffer = NodeRefBuffer;

    struct Result
    {
        NodeBuffer node_buffer;
        FunctionTypeBuffer function_type_declarations;
        FunctionDeclarationBuffer function_declarations;
    };
}

namespace RNS
{
    struct MetaContext
    {
        const char* filename;
        const char* function_name;
        u32 line_number;
        u32 column_number;
    };

    struct Compiler
    {
        enum class Subsystem : u8
        {
            Lexer,
            Parser,
            IR,
            MachineCodeGen,
            Count,
        };

        RNS::Allocator page_allocator;
        RNS::Allocator common_allocator;
        Subsystem subsystem;
        u32 errors_reported;

        void print_error(MetaContext context, const char* message, ...);
    };
}

using RNS::Compiler;
