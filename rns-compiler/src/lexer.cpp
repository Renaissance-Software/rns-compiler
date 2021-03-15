#include "lexer.h"

#include <RNS/os.h>
#include <string.h>
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




static_assert(sizeof(Token) == 2 * sizeof(s64));



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
