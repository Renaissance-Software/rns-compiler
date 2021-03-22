#pragma once

#include <RNS/types.h>
#include <RNS/data_structures.h>
#include <RNS/os.h>

extern "C" s32 printf(const char*, ...);
namespace Lexer
{
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

        void print_token_id()
        {
            auto token_id = static_cast<TokenID>(id);

            switch (token_id)
            {
                case Lexer::TokenID::Keyword:
                    printf("Keyword\n");
                    break;
                case Lexer::TokenID::NativeType:
                    printf("NativeType\n");
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

    struct TokenBuffer
    {
        Token* ptr;
        s64 len;
        s64 cap;
        s64 line_count;
        s64 file_size;
        const char* file;
        RNS::StringBuffer sb;
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

}

using namespace Lexer;

namespace AST
{
    inline const u32 no_value = UINT32_MAX;
    struct TokenParser
    {
    };
    enum class NodeType
    {
        IntLit,
        BinOp,
        Ret,
        VarDecl,
        VarExpr,
        Scope,
    };

    struct Node;
    struct IntLiteral
    {
        u64 lit;
    };

    struct BinaryOp
    {
        Node* left;
        Node* right;
        Lexer::BinOp op;
    };

    struct RetExpr
    {
        Node* expr;
    };

    enum class TypeKind
    {
        Unknown,
        Native,
        Struct,
        Union,
        Enum,
        Function,
    };

    struct StructType;
    struct UnionType;
    struct EnumType;
    struct FunctionType;

    struct Type
    {
        TypeKind kind;
        union
        {
            Lexer::NativeTypeID native_t;
            StructType* struct_t;
            UnionType* union_t;
            EnumType* enum_t;
            FunctionType* function_t;
        };
    };

    struct VarDecl
    {
        RNS::String name;
        // @TODO: should consider fully integrating the type in here
        Type* type;
        Node* value;
        bool is_fn_arg;
    };

    struct VarExpr
    {
        Node* mentioned;
    };

    using NodeSlice = RNS::Buffer<Node>;
    using FieldNames = RNS::Slice<RNS::String>;
    using FieldTypes = RNS::Slice<Type>;

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

    enum class ScopeType
    {
        Function,
    };

    struct Scope
    {
        Scope* parent;
        ScopeType type;
    };

    using NodeRefBuffer = RNS::Buffer<Node*>;
    struct ScopeBlock : public Scope
    {
        NodeRefBuffer variables;
        NodeRefBuffer statements;
    };

    struct FunctionDeclaration
    {
        // @TODO: consider: right now normal functions use anonymous function types
        FunctionType* type;
        ScopeBlock scope; // First scope variables are variable arguments
        s64 arg_count;
    };

    struct Node
    {
        NodeType type;
        union
        {
            IntLiteral int_lit;
            BinaryOp bin_op;
            RetExpr ret;
            VarDecl var_decl;
            VarExpr var_expr;
            ScopeBlock scope;
        };
    };

    // @TODO: we are transforming this to inherit from the RNS-lib buffer. This may have some drawbacks. Come back to this when the compiler is more mature.
    struct NodeBuffer : public RNS::Buffer<Node>
    {
        Node* append(NodeType type)
        {
            assert(len + 1 <= cap);
            Node* result = &ptr[len++];
            result->type = type;
            return result;
        }
    };

#if 0
    struct NodeIDBuffer
    {
        // @Info: this is the node pointer for the args
        u32* ptr;
        s64 len;
        s64 cap;

        static NodeIDBuffer create(RNS::Allocator* allocator)
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
#endif

    using FunctionDeclarationBuffer = RNS::Buffer<FunctionDeclaration>;
    using TypeBuffer = RNS::Buffer<Type>;
    using StructBuffer = RNS::Buffer<StructType>;
    using UnionBuffer = RNS::Buffer<UnionType>;
    using EnumBuffer = RNS::Buffer<EnumType>;
    using FunctionTypeBuffer = RNS::Buffer<FunctionType>;

    struct Result
    {
        NodeBuffer node_buffer;
        FunctionTypeBuffer function_type_declarations;
        // @Info: this includes native types
        TypeBuffer type_declarations;
        FunctionDeclarationBuffer function_declarations;
        const char* error_message;

        bool failed()
        {
            return error_message;
        }
    };
}

namespace Bytecode
{
    inline const s64 no_value = INT64_MAX;
    inline const char* type_to_string(NativeTypeID id)
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
    enum class OperandType
    {
        Invalid,
        New,
        StackAddress,
        IntLit,
    };

    inline const char* operand_type_to_string(OperandType type)
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

    struct Stack
    {
        s64 id;
    };

    struct PrintBuffer
    {
        char ptr[512];
    };

    struct Value
    {
        NativeTypeID type;
        OperandType op_type;
        union
        {
            AST::IntLiteral int_lit;
            Stack stack;
        };

        PrintBuffer to_string(s64 id);
    };

    struct ValueBuffer
    {
        Value* ptr;
        s64 len;
        s64 cap;

        static ValueBuffer create(RNS::Allocator* allocator)
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

        inline Value* get(s64 id)
        {
            return &ptr[id];
        }
    };

    inline PrintBuffer value_id_to_string(ValueBuffer* vb, s64 id)
    {
        PrintBuffer bf;
        if (id != Bytecode::no_value)
        {
            Value* v = vb->get(id);
            bf = v->to_string(id);
        }
        else
        {
            strcpy(bf.ptr, "NULL");
        }
        return bf;
    }

    enum class InstructionID
    {
        Invalid,
        Add,
        Assign,
        Decl,
        Ret,
    };

    struct Instruction
    {
        s64 operands[2];
        s64 result;
        InstructionID id;

        void print(ValueBuffer* vb);

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

        static InstructionBuffer create(RNS::Allocator* allocator)
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
}
