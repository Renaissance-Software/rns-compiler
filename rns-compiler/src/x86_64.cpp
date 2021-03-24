#include "x86_64.h"

#include <RNS/compiler.h>
#include <RNS/os.h>
#include <RNS/os_internal.h>
#include <RNS/profiler.h>

#include "bytecode.h"

#include <stdio.h>
#include <string.h>
#include <immintrin.h>
#include <time.h>


using namespace RNS;
using ID = s64;

template <typename T>
bool fits_into(T number, usize size, bool signedness)
{
    if (signedness)
    {
        switch (size)
        {
            case sizeof(s8):
                return number >= INT8_MIN && number <= INT8_MAX;
            case sizeof(s16):
                return number >= INT16_MIN && number <= INT16_MAX;
            case sizeof(s32):
                return number >= INT32_MIN && number <= INT32_MAX;
            case sizeof(s64):
                return number >= INT64_MIN && number <= INT64_MAX;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }
    else
    {
        switch (size)
        {
            case sizeof(u8):
                return number >= 0 && number <=  UINT8_MAX;
            case sizeof(u16):
                return number >= 0 && number <= UINT16_MAX;
            case sizeof(u32):
                return number >= 0 && number <= UINT32_MAX;
            case sizeof(u64):
                return number >= 0 && number <= UINT64_MAX;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    return false;
}

struct MetaContext
{
    const char* filename;
    const char* function_name;
    s32 line_number;
};

#define METACONTEXT { __FILE__,  __func__, __LINE__ }

enum class CallingConvention
{
    MSVC,
    SystemV,
};

CallingConvention calling_convention =
#ifdef RNS_OS_WINDOWS
CallingConvention::MSVC;
#define MSVC_x86_64 1
#define SYSTEM_V_x86_64 0
#else
#if defined(RNS_OS_LINUX)
CallingConvention::SystemV;
#define MSVC_x86_64 0
#define SYSTEM_V_x86_64 1
#else
#error
#endif
#endif

typedef void RetVoid_Param_P_S32(s32*);
typedef void RetVoidParamVoid(void);
typedef void VoidRetVoid(void);
typedef s32 Ret_S32_Args_s32(s32);
typedef s32 RetS32_ParamS32_S32(s32, s32);
typedef s32 RetS32_Param_VoidP(void*);
typedef s32 RetS32ParamVoid(void);
typedef s64 RetS64_ALotOfS64Args(s64, s64, s64, s64, s64, s64, s64, s64);
typedef s64 (fn_type_void_to_s64)(void);
typedef s64 RetS64_ParamS64_S64(s64, s64);
typedef s64 RetS64_ParamS64(s64);
typedef s64 RetS64ParamVoid(void);
typedef s64 RetS64ParamVoidStar_s64(void*, s64);

enum class OperandSize : u8
{
    Any = 0,
    Bits_8 = 1,  // 1  byte
    Bits_16 = 2,  // 2  bytes
    Bits_32 = 4,  // 4  bytes
    Bits_48 = 6,  // 6  bytes
    Bits_64 = 8,  // 8  bytes
    Bits_80 = 10, // 10 bytes
};

enum class OperandType : u8
{
    None = 0,
    Register,
    Immediate,
    MemoryIndirect,
    Relative,
    RIP_Relative,
    Import_RIP_Relative,
};

enum class SIB : u8
{
    Scale_1 = 0b00,
    Scale_2 = 0b01,
    Scale_4 = 0b10,
    Scale_8 = 0b11,
};

enum class Mod : u8
{
    NoDisplacement = 0b00,
    Displacement_8 = 0b01,
    Displacement_32 = 0b10,
    Register = 0b11,
};

const u8 OperandSizeOverride = 0x66;

enum class Rex : u8
{
    None = 0,
    Rex = 0x40,
    B = 0x41,
    X = 0x42,
    R = 0x44,
    W = 0x48,
};

enum class Register : u8
{
    A = 0,
    C = 1,
    D = 2,
    B = 3,
    SP = 4,
    BP = 5,
    SI = 6,
    DI = 7,
    AH = SP,
    CH = BP,
    DH = SI,
    BH = DI,

    r8 = 8,
    r9 = 9,
    r10 = 10,
    r11 = 11,
    r12 = 12,
    r13 = 13,
    r14 = 14,
    r15 = 15,
};

const u8 Register_N_Flag = 0b1000;

const char* register_to_string(Register r)
{
    switch (r)
    {
        rns_case_to_str(Register::A);
        rns_case_to_str(Register::C);
        rns_case_to_str(Register::D);
        rns_case_to_str(Register::B);
        rns_case_to_str(Register::SP);
        rns_case_to_str(Register::BP);
        rns_case_to_str(Register::SI);
        rns_case_to_str(Register::DI);
        rns_case_to_str(Register::r8);
        rns_case_to_str(Register::r9);
        rns_case_to_str(Register::r10);
        rns_case_to_str(Register::r11);
        rns_case_to_str(Register::r12);
        rns_case_to_str(Register::r13);
        rns_case_to_str(Register::r14);
        rns_case_to_str(Register::r15);
        default:
            return NULL;
    }
}

struct OperandMemoryIndirect
{
    s32 displacement;
    Register reg;
};

struct OperandMemoryDirect
{
    u64 memory;
};

struct OperandRIPRelative
{
    s64 offset;
};

const u32 max_label_location_count = 32;

struct LabelLocation
{
    u64 patch_target;
    u64 from_offset;
};

struct Label
{
    OperandSize label_size;
    u64 target;
    LabelLocation* locations;
    u32 location_count;
};

inline Label* label(Allocator* allocator, OperandSize size)
{
    auto* label = new(allocator) Label;
    assert(label);
    *label = {
        .label_size = size,
        .locations = new(allocator) LabelLocation[max_label_location_count],
        .location_count = 0,
    };

    return label;
}

struct Import_RIP_Relative
{
    const char* library_name;
    const char* symbol_name;
};

struct Operand
{
    OperandType type;
    u32 size;
    union
    {
        union
        {
            u8  imm8;
            u16 imm16;
            u32 imm32;
            u64 imm64;
        };
        Label* relative;
        Register reg;
        OperandMemoryIndirect indirect;
        OperandRIPRelative rip_rel;
        Import_RIP_Relative import_rip_rel;
    };
};

enum class DescriptorType : u8
{
    Void,
    Integer,
    Pointer,
    FixedSizeArray,
    Function,
    Struct,
    TaggedUnion,
};

struct Value;
using ValueBuffer = Buffer<Value>;
const u32 max_arg_count = 16;
struct DescriptorFunction
{
    ValueBuffer arg_list;
    Value* return_value;
    bool frozen;
    Value* next_overload;
};

struct DescriptorFixedSizeArray
{
    const struct Descriptor* data;
    s64 len;
};

struct DescriptorInteger
{
    u32 size;
};

struct DescriptorStructField
{
    const char* name;
    const struct Descriptor* descriptor;
    s64 offset;
};

struct DescriptorStruct
{
    const char* name;
    struct DescriptorStructField* field_list;
    s64 arg_count;
};

struct DescriptorTaggedUnion
{
    DescriptorStruct* struct_list;
    s64 struct_count;
};

struct Descriptor
{
    DescriptorType type;
    union
    {
        DescriptorInteger integer;
        DescriptorFunction function;
        Descriptor* pointer_to;
        DescriptorFixedSizeArray fixed_size_array;
        DescriptorStruct struct_;
        DescriptorTaggedUnion tagged_union;
    };
};

const u32 pointer_size = sizeof(usize);

s64 descriptor_size(const Descriptor* descriptor);

s64 descriptor_struct_size(DescriptorStruct* descriptor)
{
    s64 arg_count = descriptor->arg_count;
    assert(arg_count);
    s64 alignment = 0;
    s64 raw_size = 0;

    for (s64 i = 0; i < arg_count; i++)
    {
        DescriptorStructField* field = &descriptor->field_list[i];
        s64 field_size = descriptor_size(field->descriptor);
        alignment = max(alignment, field_size);
        bool is_last_field = i == arg_count - 1;
        if (is_last_field)
        {
            raw_size = field->offset + field_size;
        }
    }

    return RNS::align(raw_size, alignment);
}

s64 descriptor_size(const Descriptor* descriptor)
{
    DescriptorType type = descriptor->type;
    switch (type)
    {
        case DescriptorType::Void:
            return 0;
        case DescriptorType::TaggedUnion:
        {
            s64 struct_count = descriptor->tagged_union.struct_count;
            // @TODO: maybe change this?
            u32 tag_size = sizeof(s64);
            s64 body_size = 0;
            for (s64 i = 0; i < struct_count; i++)
            {
                DescriptorStruct* struct_ = &descriptor->tagged_union.struct_list[i];
                s64 struct_size = descriptor_struct_size(struct_);
                body_size = max(body_size, struct_size);
            }

            return tag_size + body_size;
        }
        case DescriptorType::Struct:
        {
            return descriptor_struct_size((DescriptorStruct*)&descriptor->struct_);
        }
        case DescriptorType::Integer:
            return descriptor->integer.size;
        case DescriptorType::Pointer:
            return pointer_size;
        case DescriptorType::FixedSizeArray:
            return descriptor_size(descriptor->fixed_size_array.data) * descriptor->fixed_size_array.len;
        case DescriptorType::Function:
            return pointer_size;
        default:
            RNS_NOT_IMPLEMENTED;
            return 0;
    }
}

#define define_descriptor(_type_)\
    Descriptor descriptor_ ## _type_ =\
    {\
        .type = DescriptorType::Integer,\
        .integer = { .size = sizeof(_type_) },\
    }

define_descriptor(u8);
define_descriptor(u16);
define_descriptor(u32);
define_descriptor(u64);
define_descriptor(s8);
define_descriptor(s16);
define_descriptor(s32);
define_descriptor(s64);



struct StructBuilderField
{
    DescriptorStructField descriptor;
    struct StructBuilderField* next;
};


struct StructBuilder
{
    s64 offset;
    s64 arg_count;
    StructBuilderField* field_list;
};

enum class InstructionExtensionType : u8
{
    IET_None,
    IET_Register,
    IET_OpCode,
    IET_Plus_Register,
};

enum class OperandEncodingType : u8
{
    None = 0,
    Register,
    Register_A,
    RegisterOrMemory,
    Relative,
    Memory,
    Immediate,
};

struct OperandEncoding
{
    OperandEncodingType type;
    u8 size;
};
static_assert(sizeof(OperandEncoding) == 2 * sizeof(u8));

const u32 max_operand_count = 4;

struct OperandCombination
{
    Rex rex_byte;
    OperandEncoding operand_encodings[max_operand_count];
};
static_assert(sizeof(OperandCombination) == sizeof(Rex) + (sizeof(OperandEncoding) * max_operand_count));

enum class InstructionOptionType : u8
{
    None = 0,
    Digit,
    Reg,
    OpCodePlusReg,
    ExplicitByteSize,
};

struct InstructionOptions
{
    InstructionOptionType type;
    u8 digit;
    u8 explicit_byte_size;
};

const u8 max_op_code_bytes = 4;

struct InstructionEncoding
{
    u8 op_code[max_op_code_bytes];
    InstructionOptions options;
    OperandCombination operand_combinations[4];
};

struct Mnemonic
{
    const InstructionEncoding* encodings;
    u32 encoding_count;
};

struct Instruction
{
    Mnemonic mnemonic;
    Operand operands[4];
    Label* label;
    MetaContext context;
};

struct ImportSymbol
{
    const char* name;
    u32 name_RVA;
    u32 offset_in_data;
};

using ImportSymbolBuffer = Buffer<ImportSymbol>;

struct ImportLibrary
{
    ImportSymbolBuffer symbols;
    const char* name;
    u32 name_RVA;
    u32 RVA;
    u32 image_thunk_RVA;
};

struct BufferU8 : public Buffer<u8>
{
    static BufferU8 create(Allocator* allocator, s64 size)
    {
        assert(allocator);
        assert(size > 0);
        BufferU8 buffer;
        buffer.ptr = new(allocator) u8[size];
        buffer.len = 0;
        buffer.cap = size;
        buffer.allocator = allocator;

        assert(buffer.ptr);

        return buffer;
    }

    u8* allocate_bytes(s64 bytes)
    {
        assert(len + bytes <= cap);
        auto* result =  &ptr[len];
        len += bytes;
        return result;
    }

    template <typename T>
    T* append(T element) {
        s64 size = sizeof(T);
        assert(len + size <= cap);
        auto index = len;
        memcpy(&ptr[index], &element, size);
        len += size;
        return reinterpret_cast<T*>(&ptr[index]);
    }

    template <typename T>
    T* append(T* element)
    {
        s64 size = sizeof(T);
        assert(len + size <= cap);
        auto index = len;
        memcpy(&ptr[index], element, size);
        len += size;
        return reinterpret_cast<T*>(&ptr[index]);
    }

    template<typename T>
    T* append(T* arr, s64 array_len)
    {
        s64 size = sizeof(T) * array_len;
        assert(len + size <= cap);
        auto index = len;
        memcpy(&ptr[index], arr, size);
        len += size;
        return reinterpret_cast<T*>(&ptr[index]);
    }
};

struct InstructionBuffer : public Buffer<Instruction>
{
    void append(Instruction instruction, MetaContext metacontext)
    {
        instruction.context = metacontext;
        append(instruction);
    }
private:
    void append(Instruction instruction)
    {
        assert(len + 1 <= cap);
        ptr[len++] = instruction;
    }
};

struct FunctionBuilder;
using DescriptorBuffer = Buffer<Descriptor>;
using StructBuilderFieldBuffer = Buffer<StructBuilderField>;
using ImportLibraryBuffer = Buffer<ImportLibrary>;
using ExecutionBuffer = BufferU8;
using GlobalBuffer = BufferU8;
using InstructionBlock = Buffer<Instruction>;
using FileBuffer = BufferU8;
using FunctionBuilderBuffer = Buffer<FunctionBuilder>;
using IDBuffer = Buffer<ID>;

struct Program
{
    GlobalBuffer data;
    ImportLibraryBuffer import_libraries;
    FunctionBuilder* entry_point;
    FunctionBuilderBuffer functions;
    s64 code_base_RVA;
    s64 data_base_RVA;
};

struct JIT
{
    ExecutionBuffer code;
    GlobalBuffer data;
};

const u32 max_instruction_count = 4096;
struct FunctionBuilder
{
    InstructionBuffer instruction_buffer;
    Descriptor* descriptor;
    Program* program;

    Label* prologue_label;
    Label* epilogue_label;

    Value* value;
    s64 stack_offset;
    s64 max_call_parameter_stack_size;
    u8 next_arg;
};

Descriptor* descriptor_pointer_to(DescriptorBuffer* descriptor_buffer, Descriptor* descriptor)
{
    Descriptor* result = descriptor_buffer->allocate();
    *result = {
        .type = DescriptorType::Pointer,
        .pointer_to = descriptor,
    };
    return result;
}

Descriptor* descriptor_array_of(DescriptorBuffer* descriptor_buffer, const Descriptor* descriptor, s64 len)
{
    Descriptor* result = descriptor_buffer->allocate();
    *result = {
        .type = DescriptorType::FixedSizeArray,
        .fixed_size_array = {
            .data = descriptor,
            .len = len,
        },
    };
    return result;
}

struct Value
{
    Descriptor* descriptor;
    Operand operand;
};

void assert_not_register(Value* value, Register r)
{
    assert(value);
    if (value->operand.type == OperandType::Register)
    {
        assert(value->operand.reg != r);
    }
}

Descriptor descriptor_void =
{
    .type = DescriptorType::Void,
};

// @Volatile @Reflection
typedef struct DescriptorStructReflection
{
    s64 arg_count;
} DescriptorStructReflection;

DescriptorStructField struct_reflection_fields[] =
{
    { .name = "field_count", .descriptor = &descriptor_s64, .offset = 0, },
};

Descriptor descriptor_struct_reflection = {
    .type = DescriptorType::Struct,
    .struct_ = {
        .field_list = struct_reflection_fields,
        .arg_count = rns_array_length(struct_reflection_fields),
    },
};


Value void_value = {
    .descriptor = &descriptor_void,
};

#define reg_init(reg_index, reg_size) { .type = OperandType::Register, .size = static_cast<u32>(reg_size), .reg = reg_index, }
#define define_register(reg_name, reg_index, reg_size)\
    .reg_name = reg_init(reg_index, reg_size)

#if MSVC_x86_64
const Register parameter_registers[] =
{
    Register::C,
    Register::D,
    Register::r8,
    Register::r9,
};

const Register return_registers[] =
{
    Register::A,
};

const Register scratch_registers[] =
{
    Register::A,
    Register::C,
    Register::D,
    Register::r8,
    Register::r9,
    Register::r10,
    Register::r11,
};

const Register preserved_registers[] =
{
    Register::B,
    Register::DI,
    Register::SI,
    Register::SP,
    Register::BP,
    Register::r12,
    Register::r13,
    Register::r14,
    Register::r15,
};

#elif (SYSTEM_V_x86_64)

const Register parameter_registers[] =
{
    Register::DI,
    Register::SI,
    Register::D,
    Register::C,
    Register::r8,
    Register::r9,
};

const Register return_registers[] =
{
    Register::A,
    Register::D,
};

const Register scratch_registers[] =
{
    Register::A,
    Register::DI,
    Register::SI,
    Register::D,
    Register::C,
    Register::r8,
    Register::r9,
    Register::r10,
    Register::r11,
};

const Register preserved_registers[] =
{
    Register::B,
    Register::SP,
    Register::BP,
    Register::r12,
    Register::r13,
    Register::r14,
    Register::r15,
};
#endif
const u32 register_count_per_size = 16;
const u32 register_sizes_count = 4;
union
{
    Operand arr[register_sizes_count][register_count_per_size];
    struct
    {
        /* 64-bit registers */
        const Operand rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
        /* 32-bit registers */
        const Operand eax, ecx, edx, ebx, esp, ebp, esi, edi, r8d, r9d, r10d, r11d, r12d, r13d, r14d, r15d;
        /* 16-bit registers */
        const Operand ax, cx, dx, bx, sp, bp, si, di, r8w, r9w, r10w, r11w, r12w, r13w, r14w, r15w;
        /* 8-bit registers */
        const Operand al, cl, dl, bl, ah, ch, dh, bh, r8b, r9b, r10b, r11b, r12b, r13b, r14b, r15b;
    };
} reg =
{
    /* 64-bit registers */
    define_register(rax,    Register::A,  8),
    define_register(rcx,    Register::C,  8),
    define_register(rdx,    Register::D,  8),
    define_register(rbx,    Register::B,  8),
    define_register(rsp,    Register::SP, 8),
    define_register(rbp,    Register::BP, 8),
    define_register(rsi,    Register::SI, 8),
    define_register(rdi,    Register::DI, 8),
    define_register(r8,     Register::r8,  8),
    define_register(r9,     Register::r9,  8),
    define_register(r10,    Register::r10, 8),
    define_register(r11,    Register::r11, 8),
    define_register(r12,    Register::r12, 8),
    define_register(r13,    Register::r13, 8),
    define_register(r14,    Register::r14, 8),
    define_register(r15,    Register::r15, 8),

    /* 32-bit registers */
    define_register(eax,    Register::A,  4),
    define_register(ecx,    Register::C,  4),
    define_register(edx,    Register::D,  4),
    define_register(ebx,    Register::B,  4),
    define_register(esp,    Register::SP, 4),
    define_register(ebp,    Register::BP, 4),
    define_register(esi,    Register::SI, 4),
    define_register(edi,    Register::DI, 4),
    define_register(r8d,    Register::r8,  4),
    define_register(r9d,    Register::r9,  4),
    define_register(r10d,   Register::r10, 4),
    define_register(r11d,   Register::r11, 4),
    define_register(r12d,   Register::r12, 4),
    define_register(r13d,   Register::r13, 4),
    define_register(r14d,   Register::r14, 4),
    define_register(r15d,   Register::r15, 4),

    /* 16-bit registers */
    define_register(ax,     Register::A,  2),
    define_register(cx,     Register::C,  2),
    define_register(dx,     Register::D,  2),
    define_register(bx,     Register::B,  2),
    define_register(sp,     Register::SP, 2),
    define_register(bp,     Register::BP, 2),
    define_register(si,     Register::SI, 2),
    define_register(di,     Register::DI, 2),
    define_register(r8w,    Register::r8,  2),
    define_register(r9w,    Register::r9,  2),
    define_register(r10w,   Register::r10, 2),
    define_register(r11w,   Register::r11, 2),
    define_register(r12w,   Register::r12, 2),
    define_register(r13w,   Register::r13, 2),
    define_register(r14w,   Register::r14, 2),
    define_register(r15w,   Register::r15, 2),

    /* 8-bit registers */
    define_register(al,     Register::A,  1),
    define_register(cl,     Register::C,  1),
    define_register(dl,     Register::D,  1),
    define_register(bl,     Register::B,  1),
    define_register(ah,     Register::AH, 1),
    define_register(ch,     Register::CH, 1),
    define_register(dh,     Register::DH, 1),
    define_register(bh,     Register::BH, 1),
    define_register(r8b,    Register::r8,  1),
    define_register(r9b,    Register::r9,  1),
    define_register(r10b,   Register::r10, 1),
    define_register(r11b,   Register::r11, 1),
    define_register(r12b,   Register::r12, 1),
    define_register(r13b,   Register::r13, 1),
    define_register(r14b,   Register::r14, 1),
    define_register(r15b,   Register::r15, 1),
};

static inline Operand imm8(u8 value)
{
    Operand operand =
    {
        .type = OperandType::Immediate,
        .size = static_cast<u32>(OperandSize::Bits_8),
        .imm8 = value,
    };

    return operand;
}

static inline Operand imm16(u16 value)
{
    Operand operand =
    {
        .type = OperandType::Immediate,
        .size = static_cast<u32>(OperandSize::Bits_16),
        .imm16 = value,
    };

    return operand;
}

static inline Operand imm32(u32 value)
{
    Operand operand =
    {
        .type = OperandType::Immediate,
        .size = static_cast<u32>(OperandSize::Bits_32),
        .imm32 = value,
    };

    return operand;
}

static inline Operand imm64(u64 value)
{
    Operand operand =
    {
        .type = OperandType::Immediate,
        .size = static_cast<u32>(OperandSize::Bits_64),
        .imm64 = value,
    };

    return operand;
}

static inline Operand imm_auto(u64 value)
{
    if (fits_into(value, sizeof(u8), false))
    {
        return imm8((u8)value);
    }
    if (fits_into(value, sizeof(u16), false))
    {
        return imm16((u16)value);
    }
    if (fits_into(value, sizeof(u32), false))
    {
        return imm32((u32)value);
    }
    
    return imm64(value);
}

const u8  stack_fill_value_8  = 0xcc;
const u16 stack_fill_value_16 = 0xcccc;
const u32 stack_fill_value_32 = 0xcccccc;
const u64 stack_fill_value_64 = 0xcccccccccccc;

const OperandSize def_rel_size = OperandSize::Bits_32;

static inline Operand rel(Label* label)
{
    assert(label->label_size == def_rel_size);
    Operand relative = { .type = OperandType::Relative, .size = (u32)label->label_size, .relative = label, };
    return relative;
}

//static inline Operand rel8(u8 value)
//{
//    Operand operand = {
//        .type = OperandType::Relative,
//        .size = static_cast<u32>(OperandSize::Bits_8),
//        .rel8 = value,
//    };
//    return operand;
//}
//
//static inline Operand rel16(u16 value)
//{
//    Operand operand = {
//        .type = OperandType::Relative,
//        .size = static_cast<u32>(OperandSize::Bits_16),
//        .rel16 = value,
//    };
//    return operand;
//}
//
//static inline Operand rel32(u32 value)
//{
//    Operand operand = {
//        .type = OperandType::Relative,
//        .size = static_cast<u32>(OperandSize::Bits_32),
//        .rel32 = value,
//    };
//    return operand;
//}
//
//static inline Operand rel64(u64 value)
//{
//    Operand operand = {
//        .type = OperandType::Relative,
//        .size = static_cast<u32>(OperandSize::Bits_64),
//        .rel64 = value,
//    };
//    return operand;
//}
//

static inline Operand rip_rel(s64 address)
{
    return {
        .type = OperandType::RIP_Relative,
        .size = static_cast<u32>(OperandSize::Bits_64),
        .rip_rel{.offset = address},
    };
}

static inline Register get_stack_register(void)
{
    switch (calling_convention)
    {
        case CallingConvention::MSVC:
            return reg.rsp.reg;
        case CallingConvention::SystemV:
            return reg.rbp.reg;
            break;
        default:
            RNS_NOT_IMPLEMENTED;
            return Register::A;
    }
}

static inline Operand stack(s32 offset, s64 size)
{
    assert(size > 0);
    assert(fits_into(size, sizeof(u32), true));
    Operand op = {
        .type = OperandType::MemoryIndirect,
        .size = (u32)size,
        .indirect =
        {
            .displacement = offset,
            .reg = get_stack_register(),
        },
    };
    return op;
}

template<typename Size, typename Reg>
static inline Operand get_reg(Size reg_size_, Reg reg_index_)
{
    u8 reg_size = static_cast<u8>(reg_size_);
    u8 reg_index = static_cast<u8>(reg_index_);
    assert(reg_index <= (u8)Register::r15);
    assert(reg_size == 1 || reg_size == 2 || reg_size == 4 || reg_size == 8);

    static u8 jump_table[9] = { 0xff, 3, 2, 0xff, 1, 0xff, 0xff, 0xff, 0 };

    u8 size_index = jump_table[reg_size];
    assert(reg_size == 1 ? size_index == 3 : true);
    assert(reg_size == 2 ? size_index == 2 : true);
    assert(reg_size == 4 ? size_index == 1 : true);
    assert(reg_size == 8 ? size_index == 0 : true);
    Operand operand = reg.arr[size_index][reg_index];
    return operand;
}

void* RIP_value_pointer(Program* program, Value* value)
{
    assert(value->operand.type == OperandType::RIP_Relative);
    return program->data.ptr + value->operand.rip_rel.offset;
}

static inline Value* global_value(GlobalBuffer* global_buffer, ValueBuffer* value_buffer, Descriptor* descriptor)
{
    auto value_size = descriptor_size(descriptor);
    auto* data_section_allocation = global_buffer->allocate_bytes(value_size);
    s64 offset_in_data_section = data_section_allocation - global_buffer->ptr;
    
    auto* value = value_buffer->allocate();
    *value = {
        .descriptor = descriptor,
        .operand = {
            .type = OperandType::RIP_Relative,
            .size = static_cast<u32>(value_size),
            .rip_rel {.offset = offset_in_data_section },
        },
    };

    return value;
}

Value* global_value_C_string(ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, Program* program, const char* str)
{
    auto length = strlen(str) + 1;

    Descriptor* descriptor = descriptor_buffer->allocate();
    *descriptor = {
        .type = DescriptorType::FixedSizeArray,
        .fixed_size_array = {.data = &descriptor_u8, .len = (s64)length },
    };

    Value* string_value = global_value(&program->data, value_buffer, descriptor);
    memcpy(RIP_value_pointer(program, string_value), str, length);

    return string_value;
}

static inline Value* reg_value(ValueBuffer* value_buffer, Register reg, const Descriptor* descriptor)
{
    s64 size = descriptor_size(descriptor);
    assert(
        size == sizeof(s8) ||
        size == sizeof(s16) ||
        size == sizeof(s32) ||
        size == sizeof(s64)
    );

    Value* result = value_buffer->allocate();
    *result = {
        .descriptor = (Descriptor*)descriptor,
        .operand = get_reg(size, reg),
    };

    return (result);
}

Value* arg_value(ValueBuffer* value_buffer, DescriptorFunction* function, Descriptor* arg_descriptor, u32 arg_index)
{
    auto size = descriptor_size(arg_descriptor);
    assert(size <= 8);

    if (arg_index < rns_array_length(parameter_registers))
    {
        function->arg_list.append(*reg_value(value_buffer, parameter_registers[arg_index], arg_descriptor));
    }
    else
    {
        switch (calling_convention)
        {
            case CallingConvention::MSVC:
            {
                auto offset = arg_index * 8;
                auto operand = stack(offset, size);

                function->arg_list.append({
                    .descriptor = arg_descriptor,
                    .operand = operand,
                });
                break;
            }
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    assert(arg_index == function->arg_list.len - 1);
    return &function->arg_list.ptr[arg_index];
}


static inline Value* s32_value(ValueBuffer* value_buffer, s32 v)
{
    Value* result = value_buffer->allocate();
    *result = {
        .descriptor = (Descriptor*)&descriptor_s32,
        .operand = imm32(v),
    };
    return (result);
}
static inline Value* s64_value(ValueBuffer* value_buffer, s64 v)
{
    Value* result = value_buffer->allocate();
    *result = {
        .descriptor = (Descriptor*)&descriptor_s64,
        .operand = imm64(v),
    };
    return (result);
}


Value* value_size(ValueBuffer* value_buffer, Value* value)
{
    return s64_value(value_buffer, descriptor_size(value->descriptor));
}

bool memory_range_equal_to_c_string(const void* range_start, const void* range_end, const char* string)
{
    s64 length = ((char*)range_end) - ((char*)range_start);
    s64 string_length = strlen(string);
    if (string_length != length) return false;
    return memcmp(range_start, string, string_length) == 0;
}

Descriptor* C_parse_type(DescriptorBuffer* descriptor_buffer, const char* range_start, const char* range_end)
{
    Descriptor* descriptor = 0;

    const char* start = range_start;
    for (const char* ch = range_start; ch <= range_end; ++ch)
    {
        if (!(*ch == ' ' || *ch == '*' || ch == range_end))
        {
            continue;
        }

        if (start != ch)
        {
            if (memory_range_equal_to_c_string(start, ch, "char") || memory_range_equal_to_c_string(start, ch, "s8"))
            {
                descriptor = (Descriptor*)&descriptor_s8;
            }
            else if (memory_range_equal_to_c_string(start, ch, "int") || memory_range_equal_to_c_string(start, ch, "s32"))
            {
                descriptor = (Descriptor*)&descriptor_s32;
            }
            else if (memory_range_equal_to_c_string(start, ch, "void*") || memory_range_equal_to_c_string(start, ch, "s64"))
            {
                descriptor = (Descriptor*)&descriptor_s64;
            }
            else if (memory_range_equal_to_c_string(start, ch, "void"))
            {
                descriptor = (Descriptor*)&descriptor_void;
            }
            else if (memory_range_equal_to_c_string(start, ch, "const"))
            {
                // TODO support const values?
            }
            else
            {
                assert(!"Unsupported argument type");
            }
        }

        if (*ch == '*')
        {
            assert(descriptor);
            Descriptor* previous_descriptor = descriptor;
            descriptor = descriptor_buffer->allocate();
            *descriptor = {
                .type = DescriptorType::Pointer,
                .pointer_to = previous_descriptor,
            };
        }
        start = ch + 1;
    }

    return descriptor;
}

Value* C_function_return_value(DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, const char* forward_declaration)
{
    const char* ch = strchr(forward_declaration, '(');
    assert(ch);
    --ch;

    // skip whitespace before (
    for (; *ch == ' '; --ch)
    {
    }
    for (; (*ch >= 'a' && *ch <= 'z') || (*ch >= 'A' && *ch <= 'Z') || (*ch >= '0' && *ch <= '9') || *ch == '_'; --ch)
    {
        // skip whitespace before function name
        for (; *ch == ' '; --ch) { }
    }
    ++ch;

    Descriptor* descriptor = C_parse_type(descriptor_buffer, forward_declaration, ch);
    assert(descriptor);

    switch (descriptor->type)
    {
        case DescriptorType::Void:
            return &void_value;
        case DescriptorType::Function:
        case DescriptorType::Integer:
        case DescriptorType::Pointer:
        {
            Value* return_value = reg_value(value_buffer, Register::A, descriptor);
            return return_value;
        }
        case DescriptorType::TaggedUnion:
        case DescriptorType::FixedSizeArray:
        case DescriptorType::Struct:
        default:
        {
            assert(!"Unsupported return type");
        }
    }

    return nullptr;
}



Descriptor* C_function_descriptor(Allocator* allocator, DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, const char* forward_declaration)
{
    Descriptor* descriptor = descriptor_buffer->allocate();
    *descriptor = {
        .type = DescriptorType::Function,
        .function = {
            .arg_list = ValueBuffer::create(allocator, 16),
        }
    };


    descriptor->function.return_value = C_function_return_value(descriptor_buffer, value_buffer, forward_declaration);

    const char* ch = strchr(forward_declaration, '(');
    assert(ch);
    ch++;

    const char* start = ch;
    Descriptor* arg_descriptor = nullptr;
    for (auto arg_index = 0; *ch; ++ch)
    {
        if (*ch == ',' || *ch == ')')
        {
            if (start != ch)
            {
                arg_descriptor = C_parse_type(descriptor_buffer, start, ch);
                assert(arg_descriptor);

                if (arg_index == 0 && arg_descriptor->type == DescriptorType::Void)
                {
                    assert(*ch == ')');
                    break;
                }

                arg_value(value_buffer, &descriptor->function, arg_descriptor, arg_index);
                ++arg_index;
            }
            start = ch + 1;
        }
    }

    return descriptor;
}

Value* C_function_value(Allocator* allocator, DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, const char* forward_declaration, u64 fn)
{
    Value* value = value_buffer->allocate();
    *value = {
        .descriptor = C_function_descriptor(allocator, descriptor_buffer, value_buffer, forward_declaration),
        .operand = imm64(fn),
    };

    return value;
}

Operand import_symbol(Allocator* general_allocator, Program* program, const char* library_name, const char* symbol_name)
{
    ImportLibrary* library = nullptr;

    for (auto i = 0; i < program->import_libraries.len; i++)
    {
        ImportLibrary* lib = &program->import_libraries.ptr[i];
        if (_stricmp(lib->name, library_name) == 0)
        {
            library = lib;
            break;
        }
    }

    if (!library)
    {
        library = program->import_libraries.append({
            .symbols = ImportSymbolBuffer::create(general_allocator, 16),
            .name = library_name,
            .name_RVA = 0xcccccccc,
            .RVA = 0xcccccccc,
            .image_thunk_RVA = 0xcccccccc,
            });
    }

    ImportSymbol* symbol = nullptr;

    for (auto i = 0; i < library->symbols.len; i++)
    {
        auto* sym = &library->symbols.ptr[i];
        if (strcmp(sym->name, symbol_name) == 0)
        {
            symbol = sym;
        }
    }

    if (!symbol)
    {
        library->symbols.append({ .name = symbol_name, .name_RVA = 0xccccccc, .offset_in_data = 0 });
    }

    Operand operand = {
        .type = OperandType::Import_RIP_Relative,
        .size = 8,
        .import_rip_rel = {
            .library_name = library_name,
            .symbol_name = symbol_name
         }
    };

    return operand;
}

Value* C_function_import(Allocator* allocator, DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, Program* program, const char* library_name, const char* forward_declaration)
{
    const char* symbol_name_end = strchr(forward_declaration, '(');
    assert(symbol_name_end);
    const char* symbol_name_start = symbol_name_end;

    while (symbol_name_start != forward_declaration && !isspace(*symbol_name_start))
    {
        --symbol_name_start;
    }
    ++symbol_name_start;
    
    u64 length = symbol_name_end - symbol_name_start;
    char* symbol_name = reinterpret_cast<char*>(allocator->allocate(length + 1).ptr);
    memcpy(symbol_name, symbol_name_start, length);
    symbol_name[length] = 0;

    Value* result = value_buffer->allocate();
    *result = {
        .descriptor = C_function_descriptor(allocator, descriptor_buffer, value_buffer, forward_declaration),
        .operand = import_symbol(allocator, program, library_name, symbol_name),
    };

    return result;
}

ImportSymbol* find_import(Program* program, const char* library_name, const char* symbol_name)
{
    for (auto i = 0; i < program->import_libraries.len; i++)
    {
        auto* lib = &program->import_libraries.ptr[i];

        if (_stricmp(lib->name, library_name) != 0)
        {
            continue;
        }

        for (auto i = 0; i < lib->symbols.len; i++)
        {
            auto* symbol = &lib->symbols.ptr[i];
            if (strcmp(symbol->name, symbol_name) == 0)
            {
                return symbol;
            }
        }
    }

    return nullptr;
}

#include "x86_64_encoding.h"

bool find_encoding(Instruction instruction, u32* encoding_index, u32* combination_index)
{
    u32 encoding_count = instruction.mnemonic.encoding_count;
    const InstructionEncoding* encodings = instruction.mnemonic.encodings;

    for (u32 encoding_i = 0; encoding_i < encoding_count; encoding_i++)
    {
        InstructionEncoding encoding = encodings[encoding_i];
        u32 combination_count = rns_array_length(encoding.operand_combinations);

        for (u32 combination_i = 0; combination_i < combination_count; combination_i++)
        {
            OperandCombination combination = encoding.operand_combinations[combination_i];
            bool matched = true;

            for (u32 operand_i = 0; operand_i < max_operand_count; operand_i++)
            {
                Operand operand = instruction.operands[operand_i];
                OperandEncoding operand_encoding = combination.operand_encodings[operand_i];

                switch (operand.type)
                {
                    case OperandType::None:
                        if (operand_encoding.type == OperandEncodingType::None)
                        {
                            continue;
                        }
                        break;
                    case OperandType::Register:
                        if (operand_encoding.type == OperandEncodingType::Register && (u32)operand_encoding.size == operand.size)
                        {
                            continue;
                        }
                        if (operand_encoding.type == OperandEncodingType::Register_A && operand.reg == Register::A && (u32)operand_encoding.size == operand.size)
                        {
                            continue;
                        }
                        if (operand_encoding.type == OperandEncodingType::RegisterOrMemory && (u32)operand_encoding.size == operand.size)
                        {
                            continue;
                        }
                        break;
                    case OperandType::Immediate:
                        if (operand_encoding.type == OperandEncodingType::Immediate && (u32)operand_encoding.size == operand.size)
                        {
                            continue;
                        }
                        break;
                    case OperandType::MemoryIndirect:
                        if (operand_encoding.type == OperandEncodingType::RegisterOrMemory)
                        {
                            continue;
                        }
                        if (operand_encoding.type == OperandEncodingType::Memory)
                        {
                            continue;
                        }
                        break;
                    case OperandType::Relative:
                        if (operand_encoding.type == OperandEncodingType::Relative && (u32)operand_encoding.size == operand.size)
                        {
                            continue;
                        }
                        if (operand_encoding.type == OperandEncodingType::Memory)
                        {
                            continue;
                        }
                        break;
                    case OperandType::RIP_Relative:
                        if (operand_encoding.type == OperandEncodingType::RegisterOrMemory)
                        {
                            continue;
                        }
                        if (operand_encoding.type == OperandEncodingType::Memory)
                        {
                            continue;
                        }
                        break;
                    case OperandType::Import_RIP_Relative:
                        if (operand_encoding.type == OperandEncodingType::RegisterOrMemory)
                        {
                            continue;
                        }
                        if (operand_encoding.type == OperandEncodingType::Memory)
                        {
                            continue;
                        }
                        break;
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }

                matched = false;
                break;
            }

            if (matched)
            {
                *encoding_index = encoding_i;
                *combination_index = combination_i;
                return true;
            }
        }
    }
    return false;
}



ExecutionBuffer eb = {};


bool typecheck(const Descriptor* a, const Descriptor* b)
{
    if (a->type != b->type)
    {
        return false;
    }

    switch (a->type)
    {
        case DescriptorType::Function:
            if (a->function.arg_list.len != b->function.arg_list.len)
            {
                return false;
            }
            if (!typecheck(a->function.return_value->descriptor, b->function.return_value->descriptor))
            {
                return false;
            }

            for (s64 i = 0; i < a->function.arg_list.len; i++)
            {
                if (!typecheck(a->function.arg_list.ptr[i].descriptor, b->function.arg_list.ptr[i].descriptor))
                {
                    return false;
                }
            }

            return true;
        case DescriptorType::FixedSizeArray:
            return typecheck(a->fixed_size_array.data, b->fixed_size_array.data) && a->fixed_size_array.len == b->fixed_size_array.len;
        case DescriptorType::Pointer:
            if (a->pointer_to->type == DescriptorType::FixedSizeArray && typecheck(a->pointer_to->fixed_size_array.data, b->pointer_to))
            {
                return true;
            }
            if (b->pointer_to->type == DescriptorType::FixedSizeArray && typecheck(b->pointer_to->fixed_size_array.data, a->pointer_to))
            {
                return true;
            }
            if (a->pointer_to->type == DescriptorType::Void || b->pointer_to->type == DescriptorType::Void)
            {
                return true;
            }
            return typecheck(a->pointer_to, b->pointer_to);
        case DescriptorType::Struct: case DescriptorType::TaggedUnion:
            return a == b;
        default:
            return descriptor_size(a) == descriptor_size(b);
    }
}

bool typecheck_values(Value* a, Value* b)
{
    assert(a);
    assert(b);
    return typecheck(a->descriptor, b->descriptor);
}

void encode_instruction(BufferU8* buffer, FunctionBuilder* fn_builder, Instruction instruction)
{
    assert(fn_builder->program);
    u64 instruction_start_offset = (u64)(buffer->ptr + buffer->len);

    if (instruction.label)
    {
        Label* label = instruction.label;
        assert(label->label_size == OperandSize::Bits_32);
        assert(!label->target);
        label->target = instruction_start_offset;

        for (u32 i = 0; i < label->location_count; i++)
        {
            auto* location = &label->locations[i];
            u8* from = (u8*)location->from_offset;
            u8* target = (u8*)label->target;
            auto difference = target - from;
            assert(fits_into(difference, sizeof(s32), true));
            assert(difference >= 0);
            s32* patch_target = (s32*)location->patch_target;
            *patch_target = static_cast<s32>(difference);
        }
        return;
    }

    u32 encoding_index;
    u32 combination_index;

    if (!find_encoding(instruction, &encoding_index, &combination_index))
    {
        printf("Instruction pushed in %s:%d:%s", instruction.context.filename, instruction.context.line_number, instruction.context.function_name);
        assert(!"Couldn't find encoding");
        return;
    }

    InstructionEncoding encoding = instruction.mnemonic.encodings[encoding_index];
    OperandCombination combination = encoding.operand_combinations[combination_index];

    u8 rex_byte = static_cast<u8>(combination.rex_byte);
    bool memory_encoding = false;

    bool r_m_encoding = false;
    for (u32 i = 0; i < rns_array_length(instruction.operands); i++)
    {
        Operand op = instruction.operands[i];
        OperandEncoding op_encoding = combination.operand_encodings[i];
        if (op.type == OperandType::Register && (u8)op.reg & Register_N_Flag)
        {
            if (encoding.options.type == InstructionOptionType::Digit)
            {
                rex_byte |= (u8)Rex::R;
            }
            else if (encoding.options.type == InstructionOptionType::OpCodePlusReg)
            {
                rex_byte |= (u8)Rex::B;
            }
        }
        else if (op_encoding.type == OperandEncodingType::RegisterOrMemory)
        {
            r_m_encoding = true;
        }
        else if (op_encoding.type == OperandEncodingType::Memory)
        {
            memory_encoding = true;
        }
    }

    u8 reg_code;
    u8 op_code[max_op_code_bytes] = { encoding.op_code[0], encoding.op_code[1], encoding.op_code[2], encoding.op_code[3] };

    if (encoding.options.type == InstructionOptionType::OpCodePlusReg)
    {
        u8 plus_reg_op_code = op_code[0];
        for (u32 i = 1; i < max_op_code_bytes; i++)
        {
            assert(op_code[i] == 0);
        }
        reg_code = static_cast<u8>(instruction.operands[0].reg);
        bool d = plus_reg_op_code & 0b10;
        bool s = plus_reg_op_code & 0b1;
        plus_reg_op_code = (plus_reg_op_code & 0b11111000) | (reg_code & 0b111);
        op_code[0] = plus_reg_op_code;
    }

    // MOD RM
    bool need_sib = false;
    u8 sib_byte = 0;
    bool is_digit = encoding.options.type == InstructionOptionType::Digit;
    bool is_reg = encoding.options.type == InstructionOptionType::Reg;
    bool need_mod_rm = is_digit || is_reg || r_m_encoding || memory_encoding;

    u8 register_or_digit = 0;
    u8 r_m = 0;
    u8 mod = 0;
    u8 mod_r_m = 0;
    bool encoding_stack_operand = false;

    if (need_mod_rm)
    {
        for (u32 oi = 0; oi < max_operand_count; oi++)
        {
            Operand operand = instruction.operands[oi];
            switch (operand.type)
            {
                case OperandType::Register:
                    if ((u8)operand.reg & Register_N_Flag)
                    {
                        rex_byte |= static_cast<u8>(Rex::R);
                    }
                    switch (oi)
                    {
                        case 0:
                            mod = static_cast<u8>(Mod::Register);
                            r_m = static_cast<u8>(operand.reg);
                            reg_code = static_cast<u8>(operand.reg);
                            if (is_reg)
                            {
                                register_or_digit = static_cast<u8>(operand.reg);
                            }
                            break;
                        case 1:
                            if (is_reg)
                            {
                                register_or_digit = static_cast<u8>(operand.reg);
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case OperandType::MemoryIndirect:
                {
                    mod = static_cast<u8>(Mod::Displacement_32);
                    r_m = static_cast<u8>(operand.indirect.reg);
                    encoding_stack_operand = need_sib = operand.indirect.reg == get_stack_register();

                    if (need_sib)
                    {
                        sib_byte = ((u8)SIB::Scale_1 << 6) | (r_m << 3) | (r_m);
                    }
                    break;
                }
                case OperandType::RIP_Relative: case OperandType::Import_RIP_Relative:
                {
                    r_m = 0b101;
                    mod = 0;
                    break;
                }
                default:
                    break;
            }
        }

        if (is_digit)
        {
            register_or_digit = encoding.options.digit;
        }

        mod_r_m = (
            (mod << 6) |
            ((register_or_digit & 0b111) << 3) |
            ((r_m & 0b111))
            );

    }

    if (rex_byte)
    {
        buffer->append(rex_byte);
    }
    else if ((instruction.operands[0].type == OperandType::Register && instruction.operands[0].size == (u32)OperandSize::Bits_16) || (instruction.operands[1].type == OperandType::Register && instruction.operands[1].size == (u32)OperandSize::Bits_16) || encoding.options.explicit_byte_size == static_cast<u8>(OperandSize::Bits_16))
    {
        buffer->append(OperandSizeOverride);
    }

    for (u32 i = 0; i < max_op_code_bytes; i++)
    {
        u8 op_code_byte = op_code[i];
        if (op_code_byte)
        {
            buffer->append(op_code_byte);
        }
    }

    if (need_mod_rm)
    {
        buffer->append(mod_r_m);
    }
    // SIB
    if (need_sib)
    {
        buffer->append(sib_byte);
    }

    // DISPLACEMENT
    if (need_mod_rm && mod != static_cast<u8>(Mod::Register))
    {
        for (u32 oi = 0; oi < max_operand_count; oi++)
        {
            Operand op = instruction.operands[oi];
            switch (op.type)
            {
                case OperandType::MemoryIndirect:
                {
                    switch ((Mod)mod)
                    {
                        case Mod::Displacement_8:
                            buffer->append((s8)op.indirect.displacement);
                            break;
                        case Mod::Displacement_32:
                        {
                            s32 displacement = op.indirect.displacement;
                            if (encoding_stack_operand)
                            {
                                if (displacement < 0)
                                {
                                    displacement += static_cast<s32>(fn_builder->stack_offset);
                                }
                                else
                                {
                                    if (displacement >= fn_builder->max_call_parameter_stack_size)
                                    {
                                        s32 return_address_size = 8;
                                        displacement += static_cast<s32>(fn_builder->stack_offset) + return_address_size;
                                    }
                                }
                            }
                            switch ((Mod)mod)
                            {
                                case Mod::Displacement_32:
                                {
                                    buffer->append(displacement);
                                } break;
                                case Mod::Displacement_8:
                                {
                                    buffer->append((s8)displacement);
                                } break;
                                case Mod::NoDisplacement:
                                    break;
                                default:
                                {
                                    RNS_UNREACHABLE;
                                } break;
                            }
                        } break;
                        default:
                            break;
                    }
                } break;
                case OperandType::Import_RIP_Relative:
                {
                    Program* program = fn_builder->program;
                    s64 next_instruction_RVA = program->code_base_RVA + buffer->len + sizeof(s32);

                    bool match_found = false;

                    for (auto i = 0; i < program->import_libraries.len; i++)
                    {
                        auto* lib = &program->import_libraries.ptr[i];
                        if (_stricmp(lib->name, op.import_rip_rel.library_name) != 0)
                        {
                            continue;
                        }

                        for (auto i = 0; i < lib->symbols.len; i++)
                        {
                            auto* fn = &lib->symbols.ptr[i];

                            if (strcmp(fn->name, op.import_rip_rel.symbol_name) == 0)
                            {
                                s64 diff = program->data_base_RVA + fn->offset_in_data - next_instruction_RVA;
                                assert(fits_into(diff, sizeof(s32), true));
                                s32 displacement = (s32)diff;

                                buffer->append(displacement);

                                match_found = true;
                                break;
                            }
                        }
                    }

                    assert(match_found);
                } break;
                case OperandType::RIP_Relative:
                {
                    Program* program = fn_builder->program;
                    s64 next_instruction_RVA = program->code_base_RVA + buffer->len + sizeof(s32);
                    s64 operand_RVA = program->data_base_RVA + op.rip_rel.offset;
                    s64 diff = operand_RVA - next_instruction_RVA;
                    assert(fits_into(diff, sizeof(s32), true));
                    s32 displacement = (s32)diff;
                    buffer->append(displacement);
                    break;
                }
                default:
                    break;
            }
        }
    }

    // IMMEDIATE
    for (u32 operand_i = 0; operand_i < max_operand_count; operand_i++)
    {
        Operand operand = instruction.operands[operand_i];
        if (operand.type == OperandType::Immediate)
        {
            switch ((OperandSize)operand.size)
            {
                case OperandSize::Bits_8:
                    buffer->append(operand.imm8);
                    break;
                case OperandSize::Bits_16:
                    buffer->append( operand.imm16);
                    break;
                case OperandSize::Bits_32:
                    buffer->append( operand.imm32);
                    break;
                case OperandSize::Bits_64:
                    buffer->append( operand.imm64);
                    break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }
        }
        else if (operand.type == OperandType::Relative)
        {
            if (operand.relative->target)
            {
                auto label_size = operand.relative->label_size;
                assert(label_size == OperandSize::Bits_32);
                u8* from = buffer->ptr + buffer->len + static_cast<u8>(label_size);
                u8* target = (u8*)(operand.relative->target);
                auto difference = target - from;

                switch (label_size)
                {
                case OperandSize::Bits_8:
                    assert(fits_into(difference, sizeof(s8), true));
                    buffer->append((s8)difference);
                    break;
                case OperandSize::Bits_16:
                    assert(fits_into(difference, sizeof(s16), true));
                    buffer->append((s16)difference);
                    break;
                case OperandSize::Bits_32:
                    assert(fits_into(difference, sizeof(s32), true));
                    buffer->append((s32)difference);
                    break;
                case OperandSize::Bits_64:
                    buffer->append(difference);
                    break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
                }
            }
            else
            {
                assert(operand.relative->location_count < max_label_location_count);
                u64 patch_target = (u64)(buffer->ptr + buffer->len);

                switch ((OperandSize)operand.size)
                {
                    case OperandSize::Bits_8:
                        buffer->append(stack_fill_value_8);
                        break;
                    case OperandSize::Bits_16:
                        buffer->append(stack_fill_value_16);
                        break;
                    case OperandSize::Bits_32:
                        buffer->append(stack_fill_value_32);
                        break;
                    case OperandSize::Bits_64:
                        buffer->append(stack_fill_value_64);
                        break;
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }
                operand.relative->locations[operand.relative->location_count] = {
                    .patch_target = patch_target,
                    .from_offset = patch_target + operand.size,
                };
                operand.relative->location_count++;
            }
        }
    }
}

FunctionBuilder* fn_begin(Allocator* allocator, ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, Program* program)
{
    Descriptor* descriptor = descriptor_buffer->allocate();
    *descriptor =
    {
        .type = DescriptorType::Function,
        .function = {
            .arg_list = ValueBuffer::create(allocator, 16),
        }
    };

    FunctionBuilder* fn_builder = program->functions.allocate();
    *fn_builder =
    {
        .instruction_buffer = InstructionBuffer::create(allocator, max_instruction_count),
        .descriptor = descriptor,
        .program = program,
        .prologue_label = label(allocator, OperandSize::Bits_32),
        .epilogue_label = label(allocator, OperandSize::Bits_32),
    };

    fn_builder->value = value_buffer->allocate();
    *(fn_builder->value) = 
    {
        .descriptor = descriptor,
        .operand = rel(fn_builder->prologue_label),
    };

    return fn_builder;
}

#define TestEncodingFn(_name_) bool _name_(Allocator* general_allocator, Allocator* virtual_allocator, ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, Program* program)
typedef TestEncodingFn(FnTestEncoding);

#define TEST_ENCODING 0
#if TEST_ENCODING
#define INSTR(...) { __VA_ARGS__ }
#define EXPECTED(...) __VA_ARGS__

static inline void print_chunk_of_bytes_in_hex(u8* buffer, usize size, const char* text)
{
    if (text)
    {
        printf("%s", text);
    }

    for (usize i = 0; i < size; i++)
    {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
}

static inline bool test_buffer(ExecutionBuffer* eb, u8* test_case, s64 case_size, const char* str)
{
    bool success = true;
    for (u32 i = 0; i < case_size; i++)
    {
        u8 got = executable->code.ptr[i];
        u8 expected = test_case[i];
        if (got != expected)
        {
            printf("[Index %u] Expected 0x%02X. Found 0x%02X.\n", i, expected, got);
            success = false;
        }
    }

    s32 chars = printf("[TEST] %s", str);
    while (chars < 40)
    {
        putc(' ', stdout);
        chars++;
    }
    if (success)
    {
        printf("[OK]\n");
    }
    else
    {
        printf("[FAILED]\n");
        print_chunk_of_bytes_in_hex(test_case, case_size, "Expected:\t");
        print_chunk_of_bytes_in_hex(executable->code.ptr, executable->code.len, "Result:\t\t");
    }

    return success;
}

static bool test_instruction(ExecutionBuffer* eb, const char* test_name, Instruction instruction, u8* expected_bytes, u8 expected_byte_count)
{
    const u32 buffer_size = 64;
    FunctionBuilder* fn_builder = {
        .eb = eb,
    };
    s64 size = 1024;
    fn_builder->executable->code.ptr = reinterpret_cast<u8*>(RNS::virtual_alloc(nullptr, size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 }));
    fn_builder->executable->code.len = 0;
    fn_builder->executable->code.cap = size;

    encode_instruction(fn_builder, instruction);

    ExecutionBuffer expected;
    expected.ptr = reinterpret_cast<u8*>(RNS::virtual_alloc(nullptr, size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 }));
    expected.len = 0;
    expected.cap = size;
    expected.append(expected_bytes, expected_byte_count);
    return test_buffer(fn_builder->eb, expected.ptr, expected.len, test_name);
}

#define TEST(eb, test_name, _instr, _test_bytes)\
    u8 expected_bytes_ ## test_name [] = { _test_bytes };\
    fn_begin(allocator, value_buffer, descriptor_buffer, &result, &eb);\
    test_instruction(&eb, #test_name, _instr, expected_bytes_ ## test_name, rns_array_length(expected_bytes_ ## test_name )

void test_main(Allocator* allocator, ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer)
{
    Value* result = nullptr;

    TEST(eb, add_ax_imm16,              INSTR(add, { reg.ax, imm16(0xffff) }), EXPECTED(0x66, 0x05, 0xff, 0xff)));
    TEST(eb, add_al_imm8,               INSTR(add, { reg.al, imm8(0xff) }), EXPECTED(0x04, UINT8_MAX)));
    TEST(eb, add_eax_imm32,             INSTR(add, { reg.eax, imm32(0xffffffff) }), EXPECTED(0x05, 0xff, 0xff, 0xff, 0xff)));
    TEST(eb, add_rax_imm32,             INSTR(add, { reg.rax, imm32(0xffffffff) }), EXPECTED(0x48, 0x05, 0xff, 0xff, 0xff, 0xff)));
    TEST(eb, add_rm8_imm8,              INSTR(add, { reg.bl, imm8(0xff) }), EXPECTED(0x80, 0xc3, 0xff)));
    TEST(eb, add_rm16_imm16,            INSTR(add, { reg.bx, imm16(0xffff) }), EXPECTED(0x66, 0x81, 0xc3, 0xff, 0xff)));
    TEST(eb, add_rm32_imm32,            INSTR(add, { reg.ebx, imm32(0xffffffff) }), EXPECTED(0x81, 0xc3, 0xff, 0xff, 0xff, 0xff)));
    TEST(eb, add_rm64_imm32,            INSTR(add, { reg.rbx, imm32(0xffffffff) }), EXPECTED(0x48, 0x81, 0xc3, 0xff, 0xff, 0xff, 0xff)));
    TEST(eb, call_r64,                  INSTR(call, { reg.rax }), EXPECTED(0xff, 0xd0)));
    TEST(eb, mov_bl_cl,                 INSTR(mov, { reg.bl, reg.cl }), EXPECTED(0x88, 0xcb)));
    TEST(eb, mov_bx_cx,                 INSTR(mov, { reg.bx, reg.cx }), EXPECTED(0x66, 0x89, 0xcb)));
    TEST(eb, mov_ebx_ecx,               INSTR(mov, { reg.ebx, reg.ecx }), EXPECTED(0x89, 0xcb)));
    TEST(eb, mov_rbx_rcx,               INSTR(mov, { reg.rbx, reg.rcx }), EXPECTED(0x48, 0x89, 0xcb)));
    TEST(eb, mov_al_imm8,               INSTR(mov, { reg.al, imm8(0xff) }), EXPECTED(0xb0, UINT8_MAX)));
    TEST(eb, mov_ax_imm16,              INSTR(mov, { reg.ax, imm16(0xffff) }), EXPECTED(0x66, 0xb8, 0xff, 0xff)));
    TEST(eb, mov_eax_imm32,             INSTR(mov, { reg.eax, imm32(0xffffffff) }), EXPECTED(0xb8, 0xff, 0xff)));
    TEST(eb, mov_rax_imm32,             INSTR(mov, { reg.rax, imm32(0xffffffff) }), EXPECTED(0x48, 0xc7, 0xc0, 0xff, 0xff, 0xff, 0xff)));
    TEST(eb, mov_rax_imm64,             INSTR(mov, { reg.rax, imm64(0xffffffffffffffff) }), EXPECTED(0x48, 0xb8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff)));
    TEST(eb, mov_r8_imm8,               INSTR(mov, { reg.bl, imm8(0xff) }), EXPECTED(0xb3, 0xff)));
    TEST(eb, mov_r16_imm16,             INSTR(mov, { reg.bx, imm16(0xffff) }), EXPECTED(0x66, 0xbb, 0xff, 0xff)));
    TEST(eb, mov_r32_imm32,             INSTR(mov, { reg.ebx, imm32(0xffffffff) }), EXPECTED(0xbb, 0xff, 0xff, 0xff, 0xff)));
    TEST(eb, mov_r64_imm64,             INSTR(mov, { reg.rbx, imm64(0xffffffffffffffff) }), EXPECTED(0x48, 0xbb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff)));
    TEST(eb, mov_rm64_imm32,            INSTR(mov, { reg.rbx, imm32(0xffffffff) }), EXPECTED(0x48, 0xc7, 0xc3, 0xff, 0xff, 0xff, 0xff)));
    TEST(eb, pop_r64,                  INSTR(pop, { reg.rbp }), EXPECTED(0x5d)));
    TEST(eb, push_r64, INSTR(push, { reg.rbp }), EXPECTED(0x55)));
    TEST(eb, push_r9, INSTR(push, { reg.r9 }), EXPECTED(0x41, 0x51)));

    // @TODO: Find out how we can test this right
#if todo_test
    TEST(eb, mov_qword_ptr_r64_offset_r64,     INSTR(mov, { stack(-8, 8), reg.rdi }), EXPECTED(0x48, 0x89, 0x7d, 0xf8)));
    TEST(eb, mov_rax_qword_ptr_r64_offset_r64, INSTR(mov, { reg.rax, stack(-8, 8)}), EXPECTED(0x48, 0x8b, 0x45, 0xf8)));
#endif
}
#endif

Value* stack_reserve(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, const Descriptor* descriptor)
{
    auto size = descriptor_size(descriptor);
    Operand reserved;

    switch (calling_convention)
    {
        // @TODO: we need to test this in SystemV
        case CallingConvention::SystemV:
        case CallingConvention::MSVC:
            fn_builder->stack_offset += size;
            reserved = stack((s32)-fn_builder->stack_offset, (u32)size);
            break;
        default:
            RNS_NOT_IMPLEMENTED;
            break;
    }

    Value* result = value_buffer->allocate();
    *result = {
        .descriptor = (Descriptor*)descriptor,
        .operand = reserved,
    }; 

    return (result);
}

bool is_memory_operand(Operand operand)
{
    return operand.type == OperandType::MemoryIndirect || operand.type == OperandType::RIP_Relative;
}

void move_value(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, const Value* a, const Value* b)
{
    s64 a_size = descriptor_size(a->descriptor);
    s64 b_size = descriptor_size(b->descriptor);

    // @Info: The operands are the same, don't move the values around
    if (memcmp(a, b, sizeof(*a) == 0))
    {
        return;
    }

    if (is_memory_operand(a->operand) && is_memory_operand(b->operand))
    {
        Value* reg_a = reg_value(value_buffer, Register::A, a->descriptor);
        move_value(value_buffer, fn_builder, reg_a, b);
        move_value(value_buffer, fn_builder, a, reg_a);
        return;
    }
   
#if 1
    if (a_size != b_size)
    {
        if (!(b->operand.type == OperandType::Immediate && b_size == sizeof(s32) && a_size == sizeof(s64)))
        {
            assert(!"Mismatched operand size when moving");
        }
    }
#else
#endif

    if ((a->operand.type != OperandType::Register && b->operand.type == OperandType::Immediate && b_size == (u32)OperandSize::Bits_64) || (a->operand.type == OperandType::MemoryIndirect && b->operand.type == OperandType::MemoryIndirect))
    {
        Value* reg_a = reg_value(value_buffer, Register::A, a->descriptor);
        fn_builder->instruction_buffer.append({ mov, { reg_a->operand, b->operand } }, METACONTEXT);
        fn_builder->instruction_buffer.append({ mov, { a->operand, reg_a->operand } }, METACONTEXT);
    }
    else
    {
        fn_builder->instruction_buffer.append({ mov, { a->operand, b->operand } }, METACONTEXT);
    }
}

Value* Stack(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Descriptor* descriptor, Value* value)
{
    // @Info: This function doesn't actually reserve stack memory, it just creates the stack operand which in the later mov is stack-allocating the variable
    Value* stack_value = stack_reserve(value_buffer, fn_builder, descriptor);
    move_value(value_buffer, fn_builder, stack_value, value);
    return stack_value;
}

Value* fn_reflect(ValueBuffer* value_buffer,  FunctionBuilder* fn_builder, Descriptor* descriptor)
{
    Value* result = stack_reserve(value_buffer, fn_builder, &descriptor_struct_reflection);
    assert(descriptor->type == DescriptorType::Struct);
    move_value(value_buffer, fn_builder, result, (s64_value(value_buffer, descriptor->struct_.arg_count)));

    return (result);
}

void fn_ensure_frozen(DescriptorFunction* function)
{
    if (function->frozen)
    {
        return;
    }
    if (!function->return_value)
    {
        function->return_value = &void_value;
    }
    function->frozen = true;
}

void fn_freeze(FunctionBuilder* fn_builder)
{
    fn_ensure_frozen(&fn_builder->descriptor->function);
}

bool fn_is_frozen(FunctionBuilder* fn_builder)
{
    return fn_builder->descriptor->function.frozen;
}

void fn_encode(BufferU8* buffer, FunctionBuilder* fn_builder)
{
    switch (calling_convention)
    {
        case CallingConvention::MSVC:
        {
            encode_instruction(buffer, fn_builder, { .label = fn_builder->prologue_label });
            assert(fits_into(fn_builder->stack_offset, sizeof(s32), true));
            s32 stack_size = static_cast<s32>(fn_builder->stack_offset);
            if (stack_size >= INT8_MIN && stack_size <= INT8_MAX)
            {
                encode_instruction(buffer, fn_builder, { sub, { reg.rsp, imm8((s8)stack_size) } });
            }
            else
            {
                encode_instruction(buffer, fn_builder, { sub, { reg.rsp, imm32(stack_size) } });
            }

            for (auto i = 0; i < fn_builder->instruction_buffer.len; i++)
            {
                Instruction instruction = fn_builder->instruction_buffer.ptr[i];
                encode_instruction(buffer, fn_builder, instruction);
            }

            encode_instruction(buffer, fn_builder, { .label = fn_builder->epilogue_label });

            assert(fn_builder->stack_offset >= INT32_MIN && fn_builder->stack_offset <= INT32_MAX);
            encode_instruction(buffer, fn_builder, { add, { reg.rsp, imm32((s32)fn_builder->stack_offset) } });
            encode_instruction(buffer, fn_builder, { ret });
        } break;
        
        default:
            RNS_NOT_IMPLEMENTED;
            break;
    }

    encode_instruction(buffer, fn_builder, { int3 });
}

void fn_end(FunctionBuilder* fn_builder)
{
    switch (calling_convention)
    {
        case CallingConvention::MSVC:
        {
            s8 alignment = 0x8;
            fn_builder->stack_offset += fn_builder->max_call_parameter_stack_size;
            fn_builder->stack_offset = align(fn_builder->stack_offset, 16) + alignment;
        } break;
        case CallingConvention::SystemV:
            RNS_NOT_IMPLEMENTED;
            break;
        default:
            RNS_NOT_IMPLEMENTED;
            break;
    }

    fn_freeze(fn_builder);
}

u64 estimate_max_code_size(Program* program)
{
    u64 total_instruction_count = 0;

    for (auto i = 0; i < program->functions.len; i++)
    {
        auto* fn = &program->functions.ptr[i];
        total_instruction_count += fn->instruction_buffer.len ? fn->instruction_buffer.len : 5;
    }

    const u64 max_bytes_per_instruction = 15;
    return total_instruction_count * max_bytes_per_instruction;
}

JIT jit_end(Allocator* virtual_allocator, Program* program)
{
    u64 code_buffer_size = estimate_max_code_size(program);
    JIT result = {
        .code = ExecutionBuffer::create(virtual_allocator, code_buffer_size),
        .data = program->data,
    };

    program->code_base_RVA = (s64)result.code.ptr;
    program->data_base_RVA = (s64)result.data.ptr;

    for (auto i = 0; i < program->import_libraries.len; i++)
    {
        auto* lib = &program->import_libraries.ptr[i];
        HINSTANCE dll_handle = LoadLibraryA(lib->name);
        assert(dll_handle);

        for (auto i = 0; i < lib->symbols.len; i++)
        {
            auto* fn = &lib->symbols.ptr[i];
            auto fn_address = (u64)GetProcAddress(dll_handle, fn->name);
            assert(fn_address);

            s64 offset = program->data.len;
            program->data.append(fn_address);
            assert(fits_into(offset, sizeof(s32), true));
            fn->offset_in_data = (s32)offset;
        }
    }

    for (auto i = 0; i < program->functions.len; i++)
    {
        auto* function = &program->functions.ptr[i];
        fn_encode(&result.code, function);
    }

    return result;
}

Value* fn_arg(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Descriptor* arg_descriptor)
{
    assert(!fn_is_frozen(fn_builder));
    auto arg_index = fn_builder->next_arg;
    fn_builder->next_arg++;
    assert(arg_index < max_arg_count);

    auto* function = &fn_builder->descriptor->function;
    auto* result = arg_value(value_buffer, function, arg_descriptor, arg_index);

    return result;
};

void fn_return(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Value* to_return, bool no_jump)
{
    assert(to_return);
    DescriptorFunction* function = &fn_builder->descriptor->function;
    if (function->return_value)
    {
        assert(typecheck(function->return_value->descriptor, to_return->descriptor));
    }
    else
    {
        assert(!fn_is_frozen(fn_builder));

        if (to_return->descriptor->type != DescriptorType::Void)
        {
            function->return_value = reg_value(value_buffer, Register::A, to_return->descriptor);
        }
        else
        {
            function->return_value = &void_value;
        }
    }

    if (to_return->descriptor->type != DescriptorType::Void)
    {
        if (memcmp(&function->return_value->operand, &to_return->operand, sizeof(to_return->operand)) != 0)
        {
            move_value(value_buffer, fn_builder, function->return_value, to_return);
        }
    }

    if (!no_jump)
    {
        fn_builder->instruction_buffer.append({ jmp, { rel(fn_builder->epilogue_label) } }, METACONTEXT);
    }
}

u64 helper_value_as_function(Value* value)
{
    assert(value);
    assert(value->operand.type == OperandType::Relative);
    assert(value->operand.size == static_cast<u8>(OperandSize::Bits_32));
    assert(value->operand.relative->target);
    return value->operand.relative->target;
}

Value* pointer_value(ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, FunctionBuilder* fn_builder, Value* value)
{
    // TODO: support registers
    // TODO: support immediates
    assert(value->operand.type == OperandType::MemoryIndirect || value->operand.type == OperandType::RIP_Relative);
    Descriptor* result_descriptor = descriptor_pointer_to(descriptor_buffer, value->descriptor);
    Value* reg_a = reg_value(value_buffer, Register::A, result_descriptor);
    fn_builder->instruction_buffer.append({ lea, {reg_a->operand, value->operand} }, METACONTEXT);
    Value* result = stack_reserve(value_buffer, fn_builder, result_descriptor);
    move_value(value_buffer, fn_builder, result, reg_a);

    return (result);
}

#define value_as_function(_value_, _type_) ((_type_*)helper_value_as_function(_value_))

Value* call_function_overload(DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Value* fn, Value** arg_list, s64 arg_count)
{
    DescriptorFunction* descriptor = &fn->descriptor->function;
    assert(fn->descriptor->type == DescriptorType::Function);
    assert(descriptor->arg_list.len == arg_count);

    fn_ensure_frozen(descriptor);

    for (s64 i = 0; i < arg_count; i++)
    {
        assert(typecheck_values(&descriptor->arg_list.ptr[i], arg_list[i]));
        move_value(value_buffer, fn_builder, &descriptor->arg_list.ptr[i], arg_list[i]);
    }

    s64 parameter_stack_size = max((s64)4, arg_count) * 8;

    // @TODO: support this for function that accepts arguments
    s64 return_size = descriptor_size(descriptor->return_value->descriptor);
    if (return_size > 8 && return_size != 0)
    {
        parameter_stack_size += return_size;
        Descriptor* return_pointer_descriptor = descriptor_pointer_to(descriptor_buffer, descriptor->return_value->descriptor);
        Value* reg_c = reg_value(value_buffer, Register::C, return_pointer_descriptor);
        fn_builder->instruction_buffer.append({ lea, { reg_c->operand, descriptor->return_value->operand } }, METACONTEXT);
    }

    fn_builder->max_call_parameter_stack_size = max(fn_builder->max_call_parameter_stack_size, parameter_stack_size);

    if (fn->operand.type == OperandType::Relative && fn->operand.size == static_cast<u8>(OperandSize::Bits_32))
    {
        fn_builder->instruction_buffer.append({ call, { fn->operand } }, METACONTEXT);
    }
    else
    {
        Value* reg_a = reg_value(value_buffer, Register::A, fn->descriptor);
        move_value(value_buffer, fn_builder, reg_a, fn);
        fn_builder->instruction_buffer.append({ call, { reg_a->operand } }, METACONTEXT);
    }

    if (return_size <= 8 && return_size != 0)
    {
        Value* result = stack_reserve(value_buffer, fn_builder, descriptor->return_value->descriptor);
        move_value(value_buffer, fn_builder, result, descriptor->return_value);
        return (result);
    }

    return (descriptor->return_value);
}

Value* call_function_value(DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Value* fn, Value** arg_list, s64 arg_count)
{
    assert(fn);
    for (Value* fn_overload = fn; fn_overload; fn_overload = fn_overload->descriptor->function.next_overload)
    {
        DescriptorFunction* descriptor = &fn_overload->descriptor->function;
        if (arg_count != descriptor->arg_list.len)
        {
            continue;
        }

        bool match = true;

        for (s64 arg_index = 0; arg_index < arg_count; arg_index++)
        {
            Value* demanded_arg_value = arg_list[arg_index];
            Value* available_arg_value = &descriptor->arg_list.ptr[arg_index];

            if (!typecheck_values(available_arg_value, demanded_arg_value))
            {
                match = false;
                break;
            }
        }
        if (match)
        {
            return call_function_overload(descriptor_buffer, value_buffer, fn_builder, fn_overload, arg_list, arg_count);
        }
    }

    assert(!"No matching overload found");
    return NULL;
}

Label* make_if(Allocator* allocator, FunctionBuilder* fn_builder, Value* condition)
{
    Label* lbl = label(allocator, OperandSize::Bits_32);
    fn_builder->instruction_buffer.append({ cmp, { condition->operand, imm32(0) } }, METACONTEXT);
    fn_builder->instruction_buffer.append({ jz, { rel(lbl) } }, METACONTEXT);

    return lbl;
}

Label* if_begin(Allocator* allocator, FunctionBuilder* fn_builder, Value* condition)
{
    return make_if(allocator, fn_builder, condition);
}

void if_end(FunctionBuilder* fn_builder, Label* lbl)
{
    fn_builder->instruction_buffer.append({ .label = lbl }, METACONTEXT);
}

struct LoopBuilder
{
    Label* start;
    Label* end;
    bool done;
};

LoopBuilder loop_start(Allocator* allocator, FunctionBuilder* fn_builder)
{
    Label* start = label(allocator, OperandSize::Bits_32);
    fn_builder->instruction_buffer.append({ .label = start }, METACONTEXT);
    return { .start = start, .end = label(allocator, OperandSize::Bits_32), .done = false };
}

void loop_end(FunctionBuilder* fn_builder, LoopBuilder* loop)
{
    fn_builder->instruction_buffer.append({ jmp, { rel(loop->start)} }, METACONTEXT);
    fn_builder->instruction_buffer.append({ .label = loop->end }, METACONTEXT);
    loop->done = true;
}

void loop_continue(FunctionBuilder* fn_builder, LoopBuilder* loop)
{
    fn_builder->instruction_buffer.append({ jmp, { rel(loop->start) } }, METACONTEXT);
}

void loop_break(FunctionBuilder* fn_builder, LoopBuilder* loop)
{
    fn_builder->instruction_buffer.append({ jmp, { rel(loop->end) } }, METACONTEXT);
}

StructBuilder struct_begin(void)
{
    return {};
}

DescriptorStructField* struct_add_field(StructBuilderFieldBuffer* struct_builder_field_buffer, StructBuilder* struct_builder, const Descriptor* descriptor, const char* name)
{
    StructBuilderField* field = struct_builder_field_buffer->allocate();

    u32 size = (u32)descriptor_size(descriptor);
    struct_builder->offset = align(struct_builder->offset, size);

    field->descriptor.name = name;
    field->descriptor.descriptor = descriptor;
    field->descriptor.offset = struct_builder->offset;


    field->next = struct_builder->field_list;
    struct_builder->field_list = field;

    struct_builder->offset += size;
    struct_builder->arg_count++;

    return &field->descriptor;
}

Descriptor* struct_end(Allocator* allocator, DescriptorBuffer* descriptor_buffer, StructBuilder* struct_builder)
{
    assert(struct_builder->arg_count);

    Descriptor* result = descriptor_buffer->allocate();
    assert(result);

    DescriptorStructField* field_list = new(allocator)DescriptorStructField[struct_builder->arg_count];
    assert(field_list);

    u64 index = struct_builder->arg_count - 1;
    for (StructBuilderField* field = struct_builder->field_list; field; field = field->next, index--)
    {
        field_list[index] = field->descriptor;
    }

    result->type = DescriptorType::Struct;
    result->struct_ = {
        .field_list = field_list,
        .arg_count = struct_builder->arg_count,
    };

    return result;
}

Value* ensure_memory(Allocator* allocator, Value* value)
{
    Operand operand = value->operand;
    if (operand.type == OperandType::MemoryIndirect)
    {
        return value;
    }
    
    Value* result = new(allocator)Value;
    if (value->descriptor->type != DescriptorType::Pointer)
        assert(!"Not implemented");
    if (value->operand.type != OperandType::Register)
        assert(!"Not implemented");

    *result = {
        .descriptor = value->descriptor->pointer_to,
        .operand = {
            .type = OperandType::MemoryIndirect,
            .indirect = {
                .displacement = 0,
                .reg = value->operand.reg,
            }
        },
    };

    return result;
}

Value* struct_get_field(Allocator* allocator, Value* raw_value, const char* name)
{
    Value* struct_value = ensure_memory(allocator, raw_value);
    Descriptor* descriptor = struct_value->descriptor;
    assert(descriptor->type == DescriptorType::Struct);

    for (s64 i = 0; i < descriptor->struct_.arg_count; ++i)
    {
        DescriptorStructField* field = &descriptor->struct_.field_list[i];
        if (strcmp(field->name, name) == 0)
        {
            Value* result = new(allocator) Value;
            // support more operands
            Operand operand = struct_value->operand;
            assert(operand.type == OperandType::MemoryIndirect);
            operand.indirect.displacement += (s32)field->offset;
            *result = {
                .descriptor = (Descriptor*)field->descriptor,
                .operand = operand,
            };

            return (result);
        }
    }

    assert(!"Couldn't find a field with specified name");
    return nullptr;
}

enum class CompareOp : u8
{
    Equal,
    Less,
    Greater,
};

Value* compare(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, CompareOp compare_op, Value* a_value, Value* b_value)
{
    assert(typecheck_values(a_value, b_value));

    Value* temp_b = stack_reserve(value_buffer, fn_builder, b_value->descriptor);
    move_value(value_buffer, fn_builder, temp_b, b_value);

    Value* reg_a = reg_value(value_buffer, Register::A, a_value->descriptor);
    move_value(value_buffer, fn_builder, reg_a, a_value);

    fn_builder->instruction_buffer.append({ cmp, { reg_a->operand, temp_b->operand } }, METACONTEXT);

    // @TODO: use xor
    reg_a = reg_value(value_buffer, Register::A, &descriptor_s64);
    move_value(value_buffer, fn_builder, reg_a, s64_value(value_buffer, 0));

    switch (compare_op)
    {
        case CompareOp::Equal:
            fn_builder->instruction_buffer.append({ setz, { reg.al }}, METACONTEXT);
            break;
        case CompareOp::Less:
            fn_builder->instruction_buffer.append({ setl, { reg.al }}, METACONTEXT);
            break;
        case CompareOp::Greater:
            fn_builder->instruction_buffer.append({ setg, { reg.al } }, METACONTEXT);
            break;
        default:
            RNS_NOT_IMPLEMENTED;
            break;
    }

    Value* result = stack_reserve(value_buffer, fn_builder, (Descriptor*)&descriptor_s64);
    move_value(value_buffer, fn_builder, result, reg_a);

    return (result);
}

Value* rns_arithmetic(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Mnemonic arithmetic_instruction, Value* a, Value* b)
{
    if (!(a->descriptor->type == DescriptorType::Pointer && b->descriptor->type == DescriptorType::Integer && b->descriptor->integer.size == 8))
    {
        assert(typecheck_values(a, b));
        assert(a->descriptor->type == DescriptorType::Integer);
    }
    assert_not_register(a, Register::A);
    assert_not_register(b, Register::A);

    Value* temp_b = stack_reserve(value_buffer, fn_builder, b->descriptor);
    move_value(value_buffer, fn_builder, temp_b, b);
    Value* reg_a = reg_value(value_buffer, Register::A, a->descriptor);
    move_value(value_buffer, fn_builder, reg_a, a);

    fn_builder->instruction_buffer.append({ arithmetic_instruction, {reg_a->operand, temp_b->operand }}, METACONTEXT);

    Value* temporary_value = stack_reserve(value_buffer, fn_builder, a->descriptor);
    move_value(value_buffer, fn_builder, temporary_value, reg_a);

    return (temporary_value);
}

static inline Value* rns_add(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Value* a, Value* b)
{
    return rns_arithmetic(value_buffer, fn_builder, add, a, b);
}

static inline Value* rns_sub(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Value* a, Value* b)
{
    return rns_arithmetic(value_buffer, fn_builder, sub, a, b);
}

Value* rns_multiply_signed(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Value* a, Value* b)
{
    assert(typecheck_values(a, b));
    assert(a->descriptor->type == DescriptorType::Integer);

    assert_not_register(a, Register::A);
    assert_not_register(b, Register::A);

    Value* reg_a = reg_value(value_buffer, Register::A, b->descriptor);
    Value* b_temp = stack_reserve(value_buffer, fn_builder, b->descriptor);
    move_value(value_buffer, fn_builder, reg_a, b);
    move_value(value_buffer, fn_builder, b_temp, reg_a);

    reg_a = reg_value(value_buffer, Register::A, a->descriptor);
    move_value(value_buffer, fn_builder, reg_a, a);

    fn_builder->instruction_buffer.append({ imul, { reg_a->operand, b_temp->operand } }, METACONTEXT);

    Value* temporary_value = stack_reserve(value_buffer, fn_builder, a->descriptor);
    move_value(value_buffer, fn_builder, temporary_value, reg_a);

    return (temporary_value);
}

Value* rns_signed_div(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Value* a, Value* b)
{
    assert(typecheck_values(a, b));
    assert(a->descriptor->type == DescriptorType::Integer);
    assert_not_register(a, Register::A);
    assert_not_register(b, Register::A);

    // Signed division stores the remainder in the D register
    Value* rdx_temp = stack_reserve(value_buffer, fn_builder, &descriptor_s64);
    Value* reg_rdx = reg_value(value_buffer, Register::D, &descriptor_s64);
    move_value(value_buffer, fn_builder, rdx_temp, reg_rdx);

    Value* reg_a = reg_value(value_buffer, Register::A, a->descriptor);
    move_value(value_buffer, fn_builder, reg_a, a);

    Value* divisor = stack_reserve(value_buffer, fn_builder, b->descriptor);
    move_value(value_buffer, fn_builder, divisor, b);
    switch (descriptor_size(a->descriptor))
    {
        case (s64)OperandSize::Bits_16:
            fn_builder->instruction_buffer.append({ cwd }, METACONTEXT);
            break;
        case (s64)OperandSize::Bits_32:
            fn_builder->instruction_buffer.append({ cdq }, METACONTEXT);
            break;
        case (s64)OperandSize::Bits_64:
            fn_builder->instruction_buffer.append({ cqo }, METACONTEXT);
            break;
        default:
            RNS_NOT_IMPLEMENTED;
    }

    fn_builder->instruction_buffer.append({ idiv, {divisor->operand}}, METACONTEXT);

    Value* temporary_value = stack_reserve(value_buffer, fn_builder, a->descriptor);
    move_value(value_buffer, fn_builder, temporary_value, reg_a);

    // Restore RDX
    move_value(value_buffer, fn_builder, reg_rdx, rdx_temp);

    return (temporary_value);
}


TestEncodingFn(test_basic_conditional)
{
    FunctionBuilder* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    Value* arg_value = fn_arg(value_buffer, fn_builder, &descriptor_s32);
    Value* if_condition = compare(value_buffer, fn_builder, CompareOp::Equal, arg_value, s32_value(value_buffer, 0));
    Label* conditional = if_begin(general_allocator, fn_builder, if_condition);
    fn_return(value_buffer, fn_builder, s32_value(value_buffer, 0), false);
    if_end(fn_builder, conditional);
    fn_return(value_buffer, fn_builder, s32_value(value_buffer, 1), false);
    fn_end(fn_builder);
    jit_end(virtual_allocator, program);

    auto* fn = value_as_function(fn_builder->value, Ret_S32_Args_s32);
    auto result = fn(0);
    if (result != 0)
    {
        return false;
    }
    result = fn(1);
    if (result != 1)
    {
        return false;
    }
    result = fn(-1);
    if (result != 1)
    {
        return false;
    }

    return true;
}

TestEncodingFn(test_partial_application)
{
    s64 test_n = 42;
    FunctionBuilder* id_fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    Value* arg_value = fn_arg(value_buffer, id_fn_builder, &descriptor_s64);
    fn_return(value_buffer, id_fn_builder, arg_value, false);
    fn_end(id_fn_builder);

    FunctionBuilder* partial_fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    Value* call_args[] = { s64_value(value_buffer, test_n) };
    Value* call_value = call_function_value(descriptor_buffer, value_buffer, partial_fn_builder, id_fn_builder->value, call_args, rns_array_length(call_args));
    fn_return(value_buffer, partial_fn_builder, call_value, false);
    fn_end(partial_fn_builder);

    jit_end(virtual_allocator, program);

    auto* value = value_as_function(partial_fn_builder->value, RetS64ParamVoid);
    s64 result = value();
    bool fn_result = (result == test_n);
    if (!fn_result)
    {
        printf("Result %lld\n", result);
        printf("Expected %lld\n", test_n);
    }
    return fn_result;
}

TestEncodingFn(test_call_puts)
{
    const char* message = "Hello world!\n";
    Value message_value = {
        .descriptor = descriptor_pointer_to(descriptor_buffer, &descriptor_s8),
        .operand = imm64((u64)(message)),
    };
    Value* args[] = { &message_value };

    Value* puts_value = C_function_value(general_allocator, descriptor_buffer, value_buffer, "int puts(const char*)", (u64)puts);
    assert(puts_value);

    FunctionBuilder* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);

    (void)call_function_value(descriptor_buffer, value_buffer, fn_builder, puts_value, args, rns_array_length(args));

    fn_return(value_buffer, fn_builder, &void_value, false);
    fn_end(fn_builder);
    jit_end(virtual_allocator, program);

    value_as_function(fn_builder->value, VoidRetVoid)();
    printf("Should see a hello world\n");

    return true;
}

TestEncodingFn(test_rns_add_sub)
{
    s64 a = 15123;
    s64 b = 6;
    s64 sub = 4;

    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    Value* a_value = fn_arg(value_buffer, fn_builder, &descriptor_s64);
    Value* b_value = fn_arg(value_buffer, fn_builder, &descriptor_s64);

    auto* return_result = rns_add(value_buffer, fn_builder, rns_sub(value_buffer, fn_builder, a_value, s64_value(value_buffer, sub)), b_value);
    fn_return(value_buffer, fn_builder, return_result, false);
    fn_end(fn_builder);
    jit_end(virtual_allocator, program);

    s64 result = value_as_function(fn_builder->value , RetS64_ParamS64_S64)(a, b);

    s64 expected = a - sub + b;
    bool fn_result = result == expected;
    if (!return_result)
    {
        printf("Result: %lld\n", result);
        printf("Expected: %lld\n", expected);
    }
    return fn_result;
}

TestEncodingFn(test_multiply)
{
    s32 a = -5;
    s32 b = 6;

    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    Value* a_value = fn_arg(value_buffer, fn_builder, &descriptor_s32);

    auto* return_result = rns_multiply_signed(value_buffer, fn_builder, a_value, s32_value(value_buffer, b));
    fn_return(value_buffer, fn_builder, return_result, false);
    fn_end(fn_builder);
    jit_end(virtual_allocator, program);

    auto result = value_as_function(fn_builder->value , RetS32_ParamS32_S32)(a, b);
    auto expected = a * b;
    auto fn_result = (result == (a * b));
    if (!fn_result)
    {
        printf("Result: %d\n", result);
        printf("Expected: %d\n", a * b);
    }

    return fn_result;
}

TestEncodingFn(test_divide)
{
    s32 a = 40;
    s32 b = 5;
    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    Value* a_value = fn_arg(value_buffer, fn_builder, &descriptor_s32);
    Value* b_value = fn_arg(value_buffer, fn_builder, &descriptor_s32);

    auto* return_result = rns_signed_div(value_buffer, fn_builder, a_value, b_value);
    fn_return(value_buffer, fn_builder, return_result, false);
    fn_end(fn_builder);
    jit_end(virtual_allocator, program);

    auto result = value_as_function(fn_builder->value, RetS32_ParamS32_S32)(a, b);
    s32 expected = a / b;

    auto fn_result = (result == expected);

    if (!fn_result)
    {
        printf("Result: %d\n", result);
        printf("Expected: %d\n", expected);
    }

    return fn_result;
}

TestEncodingFn(test_array_loop)
{
    s32 arr[] = {1, 2, 3};
    u32 len = rns_array_length(arr);

    Descriptor array_descriptor = 
    {
        .type = DescriptorType::FixedSizeArray,
        .fixed_size_array =
        {
            .data = &descriptor_s32,
            .len = len,
        },
    };

    Descriptor array_pointer_descriptor =
    {
        .type = DescriptorType::Pointer,
        .pointer_to = &array_descriptor,
    };

    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    {
        auto* arr_arg = fn_arg(value_buffer, fn_builder, &array_pointer_descriptor);
        auto* index = Stack(value_buffer, fn_builder, &descriptor_s32, s32_value(value_buffer, 0));
        auto* temp = Stack(value_buffer, fn_builder, &array_pointer_descriptor, arr_arg);

        u32 element_size = static_cast<u32>(descriptor_size(array_pointer_descriptor.pointer_to->fixed_size_array.data));
        {
            auto loop = loop_start(general_allocator, fn_builder);
            auto length = array_pointer_descriptor.pointer_to->fixed_size_array.len;

            // if (i > array_length)
            {
                auto* condition = compare(value_buffer, fn_builder, CompareOp::Greater, index, s32_value(value_buffer, static_cast<s32>(length-1)));
                auto* if_label = if_begin(general_allocator, fn_builder, condition);
                loop_break(fn_builder, &loop);
                if_end(fn_builder, if_label);
            }

            Value* reg_a = reg_value(value_buffer, Register::A, temp->descriptor);
            move_value(value_buffer, fn_builder, reg_a, temp);

            Operand pointer = {
                .type = OperandType::MemoryIndirect,
                .size = element_size,
                .indirect = {
                    .displacement = 0,
                    .reg = reg.rax.reg,
                },
            };

            fn_builder->instruction_buffer.append({ inc, { pointer } }, METACONTEXT);
            fn_builder->instruction_buffer.append({ add, { temp->operand, imm32(element_size) } }, METACONTEXT);
            fn_builder->instruction_buffer.append({ inc, {index->operand} }, METACONTEXT);
            loop_end(fn_builder, &loop);
        }
        fn_end(fn_builder);
        jit_end(virtual_allocator, program);
    }

    value_as_function(fn_builder->value, RetVoid_Param_P_S32)(arr);

    return (arr[0] == 2) &&
        (arr[1] == 3) &&
        (arr[2] == 4);
}

TestEncodingFn(test_structs)
{
    StructBuilder struct_builder = struct_begin();

    // @TODO: this should go into the function/program builder (more like like the program builder)
    StructBuilderFieldBuffer struct_builder_field_buffer = StructBuilderFieldBuffer::create(general_allocator, 64);
    DescriptorStructField* width_field = struct_add_field (&struct_builder_field_buffer, &struct_builder, &descriptor_s32, "width");
    DescriptorStructField* height_field = struct_add_field(&struct_builder_field_buffer, &struct_builder, &descriptor_s32, "height");
    DescriptorStructField* dummy_field = struct_add_field (&struct_builder_field_buffer, &struct_builder, &descriptor_s32, "dummy");

    Descriptor* size_struct_descriptor = struct_end(general_allocator, descriptor_buffer, &struct_builder);
    Descriptor* size_struct_pointer_desc = descriptor_pointer_to(descriptor_buffer, size_struct_descriptor);

    // Compute the area (width * height)
    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    {
        auto* size_struct = fn_arg(value_buffer, fn_builder, size_struct_pointer_desc);
        auto* width = struct_get_field(general_allocator, size_struct, "width");
        auto* height = struct_get_field(general_allocator, size_struct, "height");
        auto* mul_result = rns_multiply_signed(value_buffer, fn_builder, width, height);
        fn_return(value_buffer, fn_builder, mul_result, false);
        fn_end(fn_builder);
        jit_end(virtual_allocator, program);
    }

    struct { s32 width; s32 height; s32 dummy; } size = { 10, 42 };
    auto result = value_as_function(fn_builder->value, RetS32_Param_VoidP)(&size);
    auto expected = size.width * size.height;
    bool return_result = result == expected;
    if (!return_result)
    {
        return false;
    }
    auto size_of_the_struct = sizeof(size);
    auto size_of_descriptor = descriptor_size(size_struct_descriptor);
    if (size_of_the_struct != size_of_descriptor)
    {
        return false;
    }
    return true;
}

TestEncodingFn(tests_arguments_on_the_stack)
{
    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* arg0 = fn_arg(value_buffer, fn_builder, &descriptor_s64);
    auto* arg1 = fn_arg(value_buffer, fn_builder, &descriptor_s64);
    auto* arg2 = fn_arg(value_buffer, fn_builder, &descriptor_s64);
    auto* arg3 = fn_arg(value_buffer, fn_builder, &descriptor_s64);
    auto* arg4 = fn_arg(value_buffer, fn_builder, &descriptor_s64);
    auto* arg5 = fn_arg(value_buffer, fn_builder, &descriptor_s64);
    auto* arg6 = fn_arg(value_buffer, fn_builder, &descriptor_s64);
    auto* arg7 = fn_arg(value_buffer, fn_builder, &descriptor_s64);

    fn_return(value_buffer, fn_builder, arg5, false);
    fn_end(fn_builder);
    jit_end(virtual_allocator, program);

#ifndef def_num
#define def_num(a) s64 v ## a = a
    def_num(0);
    def_num(1);
    def_num(2);
    def_num(3);
    def_num(4);
    def_num(5);
    def_num(6);
    def_num(7);
#undef def_num
#endif
    s64 result = value_as_function(fn_builder->value, RetS64_ALotOfS64Args)(v0, v1, v2, v3, v4, v5, v6, v7); return (result == v5);
}

TestEncodingFn(test_type_checker)
{
    bool result = typecheck(&descriptor_s32, &descriptor_s32);
    result = result && !typecheck(&descriptor_s32, &descriptor_s16);
    result = result && !typecheck(&descriptor_s64, descriptor_pointer_to(descriptor_buffer, &descriptor_s64));
    result = result && typecheck(descriptor_pointer_to(descriptor_buffer, &descriptor_s64), descriptor_pointer_to(descriptor_buffer, &descriptor_void));
    result = result && !typecheck(descriptor_array_of(descriptor_buffer, &descriptor_s32, 2), descriptor_array_of(descriptor_buffer, &descriptor_s32, 3));
    if (!result)
    {
        return false;
    }

    // @TODO: this should go into the function/program builder (more like like the program builder)
    StructBuilderFieldBuffer struct_builder_field_buffer = StructBuilderFieldBuffer::create(general_allocator, 64);

    StructBuilder struct_buildera = struct_begin();
    struct_add_field(&struct_builder_field_buffer, &struct_buildera, &descriptor_s32, "foo1");
    Descriptor* a = struct_end(general_allocator, descriptor_buffer, &struct_buildera);
    StructBuilder struct_builderb = struct_begin();
    struct_add_field(&struct_builder_field_buffer, &struct_builderb, &descriptor_s32, "foo1");
    Descriptor* b = struct_end(general_allocator, descriptor_buffer, &struct_builderb);

    result = typecheck(a, a) && !typecheck(a, b);
    if (!result)
    {
        return false;
    }

    auto* fn_builder_a = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* a_arg0 = fn_arg(value_buffer, fn_builder_a, &descriptor_s32);
    fn_end(fn_builder_a);
    auto* fn_builder_b = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* b_arg0 = fn_arg(value_buffer, fn_builder_b, &descriptor_s32);
    fn_end(fn_builder_b);
    auto* fn_builder_c = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* c_arg0 = fn_arg(value_buffer, fn_builder_c, &descriptor_s32);
    auto* c_arg1 = fn_arg(value_buffer, fn_builder_c, &descriptor_s32);
    fn_end(fn_builder_c);
    auto* fn_builder_d = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* d_arg0 = fn_arg(value_buffer, fn_builder_d, &descriptor_s64);
    fn_end(fn_builder_d);
    auto* fn_builder_e = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* e_arg0 = fn_arg(value_buffer, fn_builder_e, &descriptor_s32);
    fn_return(value_buffer, fn_builder_e, s32_value(value_buffer, 0), false);
    fn_end(fn_builder_e);
    jit_end(virtual_allocator, program);

    result = result && typecheck_values(fn_builder_a->value,  fn_builder_b->value);
    result = result && !typecheck_values(fn_builder_a->value, fn_builder_c->value);
    result = result && !typecheck_values(fn_builder_a->value, fn_builder_d->value);
    result = result && !typecheck_values(fn_builder_d->value, fn_builder_c->value);
    result = result && !typecheck_values(fn_builder_d->value, fn_builder_e->value);
    return result;
}

TestEncodingFn(test_fibonacci)
{
    auto fib_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    {
        auto* n = fn_arg(value_buffer, fib_builder, &descriptor_s64);
        {
            auto* condition = compare(value_buffer, fib_builder, CompareOp::Equal, n, s64_value(value_buffer, 0));
            auto* if_lbl = if_begin(general_allocator, fib_builder, condition);
            fn_return(value_buffer, fib_builder, s64_value(value_buffer, 0), false);
            if_end(fib_builder, if_lbl);
        }
        {
            auto* condition = compare(value_buffer, fib_builder, CompareOp::Equal, n, s64_value(value_buffer, 1));
            auto* if_lbl = if_begin(general_allocator, fib_builder, condition);
            fn_return(value_buffer, fib_builder, s64_value(value_buffer, 1), false);
            if_end(fib_builder, if_lbl);
        }

        auto* n_minus_one = rns_sub(value_buffer, fib_builder, n, s64_value(value_buffer, 1));
        auto* n_minus_two = rns_sub(value_buffer, fib_builder, n, s64_value(value_buffer, 2));

        auto* f_minus_one = call_function_value(descriptor_buffer, value_buffer, fib_builder, fib_builder->value, &n_minus_one, 1);
        auto* f_minus_two = call_function_value(descriptor_buffer, value_buffer, fib_builder, fib_builder->value, &n_minus_two, 1);

        auto* result = rns_add(value_buffer, fib_builder, f_minus_one, f_minus_two);
        fn_return(value_buffer, fib_builder, result, false);
        fn_end(fib_builder);
        jit_end(virtual_allocator, program);
    }

    auto* fib = value_as_function(fib_builder->value, RetS64_ParamS64);
    bool result = true;
    result = result && fib(0) == 0;
    result = result && fib(1) == 1;
    result = result && fib(2) == 1;
    result = result && fib(3) == 2;
    result = result && fib(6) == 8;

    return result;
}

Value* make_identity(Allocator* general_allocator, ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, Program* program, Descriptor* descriptor)
{
    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* arg = fn_arg(value_buffer, fn_builder, descriptor);
    fn_return(value_buffer, fn_builder, arg, false);
    fn_end(fn_builder);

    return fn_builder->value;
}
Value* make_add_two(Allocator* general_allocator, ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, Program* program, Descriptor* descriptor)
{
    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* arg = fn_arg(value_buffer, fn_builder, descriptor);
    auto* result = rns_add(value_buffer, fn_builder, arg, s64_value(value_buffer, 2));
    fn_return(value_buffer, fn_builder, result, false);
    fn_end(fn_builder);

    return fn_builder->value;
}

TestEncodingFn(test_basic_parametric_polymorphism)
{
    Value* id_s64 = make_identity(general_allocator, value_buffer, descriptor_buffer, program, &descriptor_s64);
    Value* id_s32 = make_identity(general_allocator, value_buffer, descriptor_buffer, program, &descriptor_s32);
    Value* add_two_s64 = make_add_two(general_allocator, value_buffer, descriptor_buffer, program, &descriptor_s64);

    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    {
        Value* value_s64 = s64_value(value_buffer, 0);
        Value* value_s32 = s32_value(value_buffer, 0);
        call_function_value(descriptor_buffer, value_buffer, fn_builder, id_s64, &value_s64, 1);
        call_function_value(descriptor_buffer, value_buffer, fn_builder, id_s32, &value_s32, 1);
        call_function_value(descriptor_buffer, value_buffer, fn_builder, add_two_s64, &value_s64, 1);
        fn_end(fn_builder);
        jit_end(virtual_allocator, program);
    }

    return true;
}

auto size_of_descriptor(ValueBuffer* value_buffer, Descriptor* descriptor)
{
    return s64_value(value_buffer, descriptor_size(descriptor));
}

auto size_of_value(ValueBuffer* value_buffer, Value* value)
{
    return value_size(value_buffer, value);
}

auto reflect_descriptor(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Descriptor* descriptor)
{
    return fn_reflect(value_buffer, fn_builder, descriptor);
}

TestEncodingFn(test_sizeof_values)
{
    Value* sizeof_s32 = size_of_value(value_buffer, s32_value(value_buffer, 0));
    bool result = sizeof_s32;
    result = result && sizeof_s32->operand.type == OperandType::Immediate && sizeof_s32->operand.size == (u32)OperandSize::Bits_64;
    result = result && sizeof_s32->operand.imm64 == 4;
    return result;
}

TestEncodingFn(test_sizeof_types)
{
    Value* sizeof_s32 = size_of_descriptor(value_buffer, &descriptor_s32);
    bool result = sizeof_s32;
    result = result && sizeof_s32->operand.type == OperandType::Immediate && sizeof_s32->operand.size == (u32)OperandSize::Bits_64;
    result = result && sizeof_s32->operand.imm64 == 4;
    return result;
}

TestEncodingFn(test_struct_reflection)
{
    StructBuilder struct_builder = struct_begin();
    StructBuilderFieldBuffer b = StructBuilderFieldBuffer::create(general_allocator, 64);
    DescriptorStructField* width_field = struct_add_field (&b, &struct_builder, &descriptor_s32, "x");
    DescriptorStructField* height_field = struct_add_field(&b, &struct_builder, &descriptor_s32, "y");
    Descriptor* point_struct_descriptor = struct_end(general_allocator, descriptor_buffer, &struct_builder);

    auto* fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);

    Value* arg_count = fn_reflect(value_buffer, fn_builder, point_struct_descriptor);
    auto* struct_ = Stack(value_buffer, fn_builder, &descriptor_struct_reflection, arg_count);
    auto* field = struct_get_field(general_allocator, struct_, "field_count");
    fn_return(value_buffer, fn_builder, field, false);
    fn_end(fn_builder);
    jit_end(virtual_allocator, program);

    s32 size = value_as_function(fn_builder->value, RetS32ParamVoid)();
    bool fn_result = size == 2;
    return fn_result;
}

// @AdhocPolymorphism
TestEncodingFn(test_adhoc_polymorphism)
{
    auto sizeof_s32 = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    {
        auto* s32_arg = fn_arg(value_buffer, sizeof_s32, &descriptor_s32);
        fn_return(value_buffer, sizeof_s32, s64_value(value_buffer, sizeof(s32)), false);
        fn_end(sizeof_s32);
    }
    auto sizeof_s64 = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    {
        auto* s64_arg = fn_arg(value_buffer, sizeof_s64, &descriptor_s64);
        fn_return(value_buffer, sizeof_s64, s64_value(value_buffer, sizeof(s64)), false);
        fn_end(sizeof_s64);
    }

    sizeof_s32->descriptor->function.next_overload = sizeof_s64->value;

    auto checker_fn = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    {
        auto* zero_value_1 = s64_value(value_buffer, 0);
        auto* zero_value_2 = s32_value(value_buffer, 0);

        auto* x = call_function_value(descriptor_buffer, value_buffer, checker_fn, sizeof_s32->value, &zero_value_1, 1);
        auto* y = call_function_value(descriptor_buffer, value_buffer, checker_fn, sizeof_s32->value, &zero_value_2, 1);

        auto* addition = rns_add(value_buffer, checker_fn, x, y);
        fn_return(value_buffer, checker_fn, addition, false);
        fn_end(checker_fn);
    }

    jit_end(virtual_allocator, program);

    auto* sizeofs64_fn = value_as_function(sizeof_s64->value, RetS64ParamVoid);
    auto size_result = sizeofs64_fn();
    bool fn_result = size_result == sizeof(s64);
    if (!fn_result)
    {
        return false;
    }

    auto* checker_function = value_as_function(checker_fn->value, RetS64ParamVoid);
    auto checker_result = checker_function();
    fn_result = checker_result == sizeof(s64) + sizeof(s32);
    return fn_result;
}

TestEncodingFn(test_rip_addressing_mode)
{
    s32 a = 32, b = 10;
    // @TODO: consider refactoring (abstracting) this in a more robust way (don't hardcode)
    Value* global_a = global_value(&program->data, value_buffer, &descriptor_s32);
    {
        if (global_a->operand.type != OperandType::RIP_Relative)
        {
            return false;
        }

        auto* address = (s32*)RIP_value_pointer(program, global_a);
        *address = a;
    }
    Value* global_b = global_value(&program->data, value_buffer, &descriptor_s32);
    {
        if (global_b->operand.type != OperandType::RIP_Relative)
        {
            return false;
        }

        auto* address = (s32*)RIP_value_pointer(program, global_b);
        *address = b;
    }

    auto checker_fn = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    {
        auto* addition = rns_add(value_buffer, checker_fn, global_a, global_b);
        fn_return(value_buffer, checker_fn, addition, false);
        fn_end(checker_fn);
    }
    jit_end(virtual_allocator, program);

    auto* checker = value_as_function(checker_fn->value, RetS32ParamVoid);
    auto result = checker();
    auto expected = a + b;

    bool fn_result = result == expected;

    return fn_result;
}

Value* cast_to_tag(Allocator* general_allocator, DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, FunctionBuilder* fn_builder, const char* name, Value* value)
{
    assert(value);
    assert(value->descriptor->type == DescriptorType::Pointer);
    const Descriptor* descriptor = value->descriptor->pointer_to;

    // @TODO:
    assert(value->operand.type == OperandType::Register);
    Value* tag_value = value_buffer->allocate();
    *tag_value = {
        .descriptor = &descriptor_s64,
        .operand = {
            .type = OperandType::MemoryIndirect,
            .size = static_cast<u32>(descriptor_size(&descriptor_s64)),
            .indirect = {
                .displacement = 0,
                .reg = value->operand.reg,
            },
        }
    };

    s64 struct_count = descriptor->tagged_union.struct_count;

    for (s64 i = 0; i < struct_count; i++)
    {
        DescriptorStruct* struct_ = &descriptor->tagged_union.struct_list[i];
        if (strcmp(struct_->name, name) == 0)
        {
            Descriptor* constructor_descriptor = descriptor_buffer->allocate();
            *constructor_descriptor = {
                .type = DescriptorType::Struct,
                .struct_ = *struct_,
            };

            Descriptor* pointer_descriptor = descriptor_pointer_to(descriptor_buffer, constructor_descriptor);
            Value* result_overload = value_buffer->allocate();
            *result_overload = {
                .descriptor = pointer_descriptor,
                .operand = reg.rbx,
            };
            move_value(value_buffer, fn_builder, result_overload, (s64_value(value_buffer, 0)));

            Value* comparison = compare(value_buffer, fn_builder, CompareOp::Equal, (tag_value), s64_value(value_buffer, i));

            Value* result_value = (result_overload);

            {
                auto* if_block = if_begin(general_allocator, fn_builder, comparison);
                move_value(value_buffer, fn_builder, result_overload, value);
                auto* addition = rns_add(value_buffer, fn_builder, (result_overload), s64_value(value_buffer, sizeof(s64)));
                move_value(value_buffer, fn_builder, result_overload, addition);
                if_end(fn_builder, if_block);
            }

            return result_value;
        }
    }

    assert(!"Could not find specified name in the tagged union");
    return NULL;
}

TestEncodingFn(test_tagged_unions)
{
    DescriptorStructField some_fields[] = {
        {.name = "value", .descriptor = &descriptor_s64, .offset = 0, },
    };

    DescriptorStruct constructors[] = {
        {.name = "None", .field_list = nullptr, .arg_count = 0, },
        {.name = "Some", .field_list = some_fields, .arg_count = rns_array_length(some_fields), },
    };

    Descriptor option_s64_descriptor = {
        .type = DescriptorType::TaggedUnion,
        .tagged_union = {
            .struct_list = constructors,
            .struct_count = rns_array_length(constructors),
        }
    };

    auto with_default_value = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* option_value = fn_arg(value_buffer, with_default_value, descriptor_pointer_to(descriptor_buffer, &option_s64_descriptor));
    auto* default_value = fn_arg(value_buffer, with_default_value, &descriptor_s64);

    {
        Value* some = cast_to_tag(general_allocator, descriptor_buffer, value_buffer, with_default_value, "Some", option_value);
        auto* some_if_block = if_begin(general_allocator, with_default_value, some);
        auto* value = struct_get_field(general_allocator, some, "value");
        fn_return(value_buffer, with_default_value, value, false);
        if_end(with_default_value, some_if_block);
    }

    fn_return(value_buffer, with_default_value, default_value, false);
    fn_end(with_default_value);
    jit_end(virtual_allocator, program);

    auto* with_default = value_as_function(with_default_value->value, RetS64ParamVoidStar_s64);
    struct { s64 tag; s64 maybe_value; } test_none = {0};
    struct { s64 tag; s64 maybe_value; } test_some = {1, 21};
    bool result = with_default(&test_none, 42) == 42;
    result = result && with_default(&test_some, 42) == 21;

    return result;
}

TestEncodingFn(test_hello_world_lea)
{
    Value* puts_value = C_function_value(general_allocator, descriptor_buffer, value_buffer, "int puts(const char*)", (u64)puts);
    
    Descriptor message_descriptor = {
        .type = DescriptorType::FixedSizeArray,
        .fixed_size_array = {
            .data = &descriptor_s8,
            .len = 4,
         }
    };

    u8 hi[] = { 'H', 'i', '!', 0 };
    s32 hi_s32 = *(s32*)hi;
    auto hello_world = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    Value* message_value = stack_reserve(value_buffer, hello_world, &message_descriptor);
    move_value(value_buffer, hello_world, message_value, s32_value(value_buffer, hi_s32));
    auto* message_pointer_value = pointer_value(value_buffer, descriptor_buffer, hello_world, message_value);
    call_function_value(descriptor_buffer, value_buffer, hello_world, puts_value, &message_pointer_value, 1);
    fn_end(hello_world);
    jit_end(virtual_allocator, program);

    printf("It should print hello world\n");
    value_as_function(hello_world->value, RetVoidParamVoid)();

    return true;
}

typedef struct LargeStruct
{
    u64 x, y;
} LargeStruct;

LargeStruct test()
{
    return { 42, 84 };
}

TestEncodingFn(test_large_size_return)
{
    StructBuilder struct_builder = struct_begin();
    {
        auto field_buffer = StructBuilderFieldBuffer::create(general_allocator, 64);
        struct_add_field(&field_buffer, &struct_builder, &descriptor_u64, "x");
        struct_add_field(&field_buffer, &struct_builder, &descriptor_u64, "y");
    }
    Descriptor* point_struct_descriptor = struct_end(general_allocator, descriptor_buffer, &struct_builder);

    Value* return_overload = value_buffer->allocate();
    *return_overload =
    {
        .descriptor = point_struct_descriptor,
        .operand = stack(0, descriptor_size(point_struct_descriptor)),
    };

     Descriptor* c_test_fn_descriptor = descriptor_buffer->allocate();
    *c_test_fn_descriptor = 
    {
        .type = DescriptorType::Function,
        .function = {
            .return_value = return_overload,
        },
    };

    Value* c_test_fn_overload = value_buffer->allocate();
    *c_test_fn_overload = 
    {
        .descriptor = c_test_fn_descriptor,
        .operand = {
            .type = OperandType::Immediate,
            .size = static_cast<u32>(OperandSize::Bits_64),
            .imm64 = (u64)(test),
        },
    };

    Value* c_test_fn_value = (c_test_fn_overload);

    auto checker_fn = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    Value* test_result = call_function_value(descriptor_buffer, value_buffer, checker_fn, c_test_fn_value, NULL, 0);
    Value* x = struct_get_field(general_allocator, test_result, "x");
    fn_return(value_buffer, checker_fn, x, false);
    fn_end(checker_fn);
    jit_end(virtual_allocator, program);

    RetS64ParamVoid* checker_value_fn = value_as_function(checker_fn->value, RetS64ParamVoid);
    s64 result = checker_value_fn();
    bool fn_result = result == 42;

    return fn_result;
}

TestEncodingFn(test_shared_library)
{
    auto* GetStdHandle_value = C_function_import(general_allocator, descriptor_buffer, value_buffer, program, "kernel32.dll", "s64 GetStdHandle(s32)");

    auto checker_fn = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    {
        auto* arg = s32_value(value_buffer, STD_INPUT_HANDLE);
        auto* call = call_function_value(descriptor_buffer, value_buffer, checker_fn, GetStdHandle_value, &arg, 1);
        fn_return(value_buffer, checker_fn, call, false);
        fn_end(checker_fn);
    }
   
    jit_end(virtual_allocator, program);

    auto our_get_std_handle = value_as_function(checker_fn->value, RetS64ParamVoid);
    auto result = our_get_std_handle();
    auto expected = (s64)(GetStdHandle(STD_INPUT_HANDLE));
    bool fn_result = result == expected;

    return fn_result;
}



using RNS::Allocator;
using RNS::DebugAllocator;

#define file_offset_to_rva(_section_header_)\
    (static_cast<DWORD>(file_buffer.len) - (_section_header_)->PointerToRawData + (_section_header_)->VirtualAddress)



struct PE32Constants
{
    u16 file_alignment;
    u16 section_alignment;
    u8 min_windows_version_vista;
} PE32 = {
    .file_alignment = 0x200,
    .section_alignment = 0x1000,
    .min_windows_version_vista = 6,
};

enum class PE32DirectoryIndex
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
    Bound,
    IAT,
    DelayImport,
    CLR,
};

struct Encoded_RDATA_Section
{
    BufferU8 buffer;
    s32 IAT_RVA;
    s32 IAT_size;
    s32 import_directory_RVA;
    s32 import_directory_size;
};

Encoded_RDATA_Section encode_rdata_section(Allocator* allocator, Program* program, IMAGE_SECTION_HEADER* header)
{
    #define get_rva() (s32)(header->VirtualAddress + buffer->len)

    s64 expected_encoded_size = 0;
    program->data_base_RVA = header->VirtualAddress;

    for (auto i = 0; i < program->import_libraries.len; i++)
    {
        auto* lib = &program->import_libraries.ptr[i];
        expected_encoded_size += align(strlen(lib->name) + 1, 2);

        for (auto i = 0; i < lib->symbols.len; i++)
        {
            auto* sym = &lib->symbols.ptr[i];

            {
                expected_encoded_size += sizeof(s16);
                expected_encoded_size += align(strlen(sym->name) + 1, 2);
            }

            {
                expected_encoded_size += sizeof(u64);
            }

            {
                expected_encoded_size += sizeof(u64);
            }

            {
                expected_encoded_size += sizeof(IMAGE_IMPORT_DESCRIPTOR);
            }
        }

        expected_encoded_size += sizeof(u64);
        expected_encoded_size += sizeof(u64);
        expected_encoded_size += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }

    auto global_data_size = align(program->data.len, 16);
    expected_encoded_size += global_data_size;

    Encoded_RDATA_Section result = {
        .buffer = BufferU8::create(allocator, expected_encoded_size),
    };

    auto* buffer = &result.buffer;

    auto* global_data = buffer->allocate_bytes(global_data_size);
    memcpy(global_data, program->data.ptr, program->data.len);

    for (auto i = 0; i < program->import_libraries.len; i++)
    {
        ImportLibrary* lib = &program->import_libraries.ptr[i];
        
        for (auto i = 0; i < lib->symbols.len; i++)
        {
            auto* symbol = &lib->symbols.ptr[i];

            symbol->name_RVA = get_rva();
            buffer->append((s16)0);
            auto name_size = strlen(symbol->name) + 1;
            auto aligned_name_size = align(name_size, 2);
            memcpy(buffer->allocate_bytes(aligned_name_size), symbol->name, name_size);
        }
    }

    result.IAT_RVA = get_rva();
    for (auto i = 0; i < program->import_libraries.len; i++)
    {
        auto* lib = &program->import_libraries.ptr[i];
        lib->RVA  = get_rva();
        
        for (auto i = 0; i < lib->symbols.len; i++)
        {
            auto* symbol = &lib->symbols.ptr[i];

            symbol->offset_in_data = get_rva() - header->VirtualAddress;
            buffer->append((u64)symbol->offset_in_data);
        }

        buffer->append((u64)0);
    }
    result.IAT_size = (s32)buffer->len;

    // Image thunks
    for (auto i = 0; i < program->import_libraries.len; i++)
    {
        ImportLibrary* lib = &program->import_libraries.ptr[i];
        lib->image_thunk_RVA = get_rva();
        
        for (auto i = 0; i < lib->symbols.len; i++)
        {
            auto* symbol = &lib->symbols.ptr[i];
            buffer->append((u64)symbol->name_RVA);
        }

        buffer->append((u64)0);
    }

    // Library names
    for (auto i = 0; i < program->import_libraries.len; i++)
    {
        ImportLibrary* lib = &program->import_libraries.ptr[i];
        lib->name_RVA = get_rva();
        auto name_size = strlen(lib->name) + 1;
        auto aligned_name_size = align(name_size, 2);

        memcpy(buffer->allocate_bytes(aligned_name_size), lib->name, name_size);
    }

    // Import directory
    result.import_directory_RVA = get_rva();

    for (auto i = 0; i < program->import_libraries.len; i++)
    {
        ImportLibrary* lib = &program->import_libraries.ptr[i];

        buffer->append(IMAGE_IMPORT_DESCRIPTOR{
                .OriginalFirstThunk = lib->image_thunk_RVA,
                .Name = lib->name_RVA,
                .FirstThunk = lib->RVA
            });
    }

    result.import_directory_size = get_rva() - result.import_directory_RVA;

    buffer->append(IMAGE_IMPORT_DESCRIPTOR{});

    assert(buffer->len <= expected_encoded_size);
    assert(fits_into(buffer->len, sizeof(s32), true));

    header->Misc.VirtualSize = (u32)buffer->len;
    header->SizeOfRawData = (u32)align(buffer->len, PE32.file_alignment);

    return result;
}

struct EncodedTextSection
{
    BufferU8 buffer;
    u32 entry_point_RVA;
};

EncodedTextSection encode_text_section(Allocator* general_allocator, Program* program, IMAGE_SECTION_HEADER* header)
{
    u64 max_code_size = estimate_max_code_size(program);
    max_code_size = align(max_code_size, PE32.file_alignment);

    EncodedTextSection result = {
        .buffer = BufferU8::create(general_allocator, max_code_size),
    };

    BufferU8* buffer = &result.buffer;

    program->code_base_RVA = header->VirtualAddress;

    for (auto i = 0; i < program->functions.len; i++)
    {
        auto* fn = &program->functions.ptr[i];
        if (fn == program->entry_point)
        {
            result.entry_point_RVA = get_rva();
        }

        fn_encode(&result.buffer, fn);
    }

    assert(fits_into(buffer->len, sizeof(u32), false));
    header->Misc.VirtualSize = (u32)buffer->len;
    header->SizeOfRawData = (u32)align(buffer->len, PE32.file_alignment);

    return result;
#undef get_rva
}

void write_executable(Allocator* general_allocator, Program* program, const char* exe_name)
{
    IMAGE_SECTION_HEADER sections[] =
    {
        {
            .Name = ".rdata",
            .Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,
        },
        {
            .Name = ".text",
            .Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE,
        },
        {},
    };
    static_assert(rns_array_length(sections) == 3);

    auto file_size_of_headers = sizeof(IMAGE_DOS_HEADER) + sizeof(s32) + sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER64) + sizeof(sections);
    file_size_of_headers = align(file_size_of_headers, PE32.file_alignment);
    auto virtual_size_of_headers = align(file_size_of_headers, PE32.section_alignment);

    auto* rdata_section_header = &sections[0];
    rdata_section_header->PointerToRawData = (u32)file_size_of_headers;
    rdata_section_header->VirtualAddress = (u32)virtual_size_of_headers;

    Encoded_RDATA_Section encoded_rdata_section = encode_rdata_section(general_allocator, program, rdata_section_header);

    auto rdata_section_buffer = encoded_rdata_section.buffer;

    IMAGE_SECTION_HEADER* text_section_header = &sections[1];
    text_section_header->PointerToRawData = rdata_section_header->PointerToRawData + rdata_section_header->SizeOfRawData;
    text_section_header->VirtualAddress = rdata_section_header->VirtualAddress + (u32)align(rdata_section_header->SizeOfRawData, PE32.section_alignment);

    EncodedTextSection encoded_text_section = encode_text_section(general_allocator, program, text_section_header);
    Buffer text_section_buffer = encoded_text_section.buffer;

    auto virtual_size_of_image = text_section_header->VirtualAddress + align(text_section_header->SizeOfRawData, PE32.section_alignment);
    u64 max_file_buffer = file_size_of_headers + rdata_section_header->SizeOfRawData + text_section_header->SizeOfRawData;

    FileBuffer file_buffer = FileBuffer::create(general_allocator, max_file_buffer);

    file_buffer.append(IMAGE_DOS_HEADER {
        .e_magic = IMAGE_DOS_SIGNATURE,
        .e_lfanew = sizeof(IMAGE_DOS_HEADER),
        });

    file_buffer.append((u32)IMAGE_NT_SIGNATURE);

    file_buffer.append(IMAGE_FILE_HEADER{
        .Machine = IMAGE_FILE_MACHINE_AMD64,
        .NumberOfSections = rns_array_length(sections) - 1,
        .TimeDateStamp = (DWORD)time(NULL),
        .SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64),
        .Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE,
        });


    auto* optional_header = file_buffer.append(IMAGE_OPTIONAL_HEADER64{
        .Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC,
        .SizeOfCode = text_section_header->SizeOfRawData,
        .SizeOfInitializedData = rdata_section_header->SizeOfRawData,
        .AddressOfEntryPoint = encoded_text_section.entry_point_RVA,
        .BaseOfCode = text_section_header->VirtualAddress,
        .ImageBase = 0x0000000140000000,
        .SectionAlignment = PE32.section_alignment,
        .FileAlignment = PE32.file_alignment,
        .MajorOperatingSystemVersion = PE32.min_windows_version_vista,
        .MajorSubsystemVersion = PE32.min_windows_version_vista,
        .SizeOfImage = (u32)virtual_size_of_image,
        .SizeOfHeaders = (u32)file_size_of_headers,
        .CheckSum = 0,
        .Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI,
        .DllCharacteristics = IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA | IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE,
        .SizeOfStackReserve = 0x100000,
        .SizeOfStackCommit = 0x1000,
        .SizeOfHeapReserve = 0x100000,
        .SizeOfHeapCommit = 0x1000,
        .NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES,
        });

    optional_header->DataDirectory[(u32)PE32DirectoryIndex::IAT].VirtualAddress = encoded_rdata_section.IAT_RVA;
    optional_header->DataDirectory[(u32)PE32DirectoryIndex::IAT].Size = encoded_rdata_section.IAT_size;
    optional_header->DataDirectory[(u32)PE32DirectoryIndex::Import].VirtualAddress = encoded_rdata_section.import_directory_RVA;
    optional_header->DataDirectory[(u32)PE32DirectoryIndex::Import].Size = encoded_rdata_section.import_directory_size;

    for (auto i = 0; i < rns_array_length(sections); i++)
    {
        file_buffer.append(sections[i]);
    }

    file_buffer.len = rdata_section_header->PointerToRawData;
    auto* dst = file_buffer.allocate_bytes(rdata_section_buffer.len);
    memcpy(dst, rdata_section_buffer.ptr, rdata_section_buffer.len);

    // useless statement
    file_buffer.len = (u64)rdata_section_header->PointerToRawData + (u64)rdata_section_header->SizeOfRawData;

    // .text section
    file_buffer.len = text_section_header->PointerToRawData;

    auto* text_section_code_buffer = file_buffer.allocate_bytes(text_section_buffer.len);
    memcpy(text_section_code_buffer, text_section_buffer.ptr, text_section_buffer.len);

    file_buffer.len = text_section_header->PointerToRawData + text_section_header->SizeOfRawData;

    {
        auto file_handle = CreateFileA(exe_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        assert(file_handle != INVALID_HANDLE_VALUE);

        DWORD bytes_written = 0;
        WriteFile(file_handle, file_buffer.ptr, (DWORD)file_buffer.len, &bytes_written, 0);
        CloseHandle(file_handle);
    }
}

TestEncodingFn(test_write_minimal_executable)
{
    Value* ExitProcess_value = C_function_import(general_allocator, descriptor_buffer, value_buffer, program, "kernel32.dll", "s64 ExitProcess(s32)");

    auto* my_exit = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    auto* exit_code_value = s32_value(value_buffer, 42);
    call_function_value(descriptor_buffer, value_buffer, my_exit, ExitProcess_value, &exit_code_value, 1);
    fn_end(my_exit);

    auto* main = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    program->entry_point = main;
    auto* call_value = call_function_value(descriptor_buffer, value_buffer, main, my_exit->value, nullptr, 0);
    fn_end(main);

    write_executable(general_allocator, program, "minimal.exe");

    return true;
}

TestEncodingFn(test_write_hello_world_executable)
{
    Value* GetStdHandle_value = C_function_import(general_allocator, descriptor_buffer, value_buffer, program, "kernel32.dll", "s64 GetStdHandle(s32)");
    Value* STDOUT_HANDLE_value = s32_value(value_buffer, STD_OUTPUT_HANDLE);
    Value* ExitProcess_value = C_function_import(general_allocator, descriptor_buffer, value_buffer, program, "kernel32.dll", "s64 ExitProcess(s32)");
    Value* WriteFile_value = C_function_import(general_allocator, descriptor_buffer, value_buffer, program, "kernel32.dll", "s32 WriteFile(s64, void*, s32, void*, s64)");

    auto* main_fn = fn_begin(general_allocator, value_buffer, descriptor_buffer, program);
    program->entry_point = main_fn;
    auto* handle = call_function_value(descriptor_buffer, value_buffer, main_fn, GetStdHandle_value, &STDOUT_HANDLE_value, 1);
    auto* bytes_written = Stack(value_buffer, main_fn, &descriptor_s32, s32_value(value_buffer, 0));
    auto* bytes_written_ptr = pointer_value(value_buffer, descriptor_buffer, main_fn, bytes_written);
    auto* message_bytes = global_value_C_string(value_buffer, descriptor_buffer, program, "Hello world!");
    auto* message_ptr = pointer_value(value_buffer, descriptor_buffer, main_fn, message_bytes);

    Value* args[] = { handle, message_ptr, s32_value(value_buffer, static_cast<s32>(message_bytes->descriptor->fixed_size_array.len)), bytes_written_ptr, s64_value(value_buffer, 0) };
    call_function_value(descriptor_buffer, value_buffer, main_fn, WriteFile_value, args, rns_array_length(args));
    auto* exit_code = s32_value(value_buffer, 0);
    call_function_value(descriptor_buffer, value_buffer, main_fn, ExitProcess_value, &exit_code, 1);
    fn_end(main_fn);

    write_executable(general_allocator, program, "hello_world.exe");

    return true;
}

struct TestFunctionArgs
{
    Allocator* general_allocator;
    Allocator* virtual_allocator;
    ValueBuffer* value_buffer;
    DescriptorBuffer* descriptor_buffer;
    Program* program;
};

struct TestCase
{
    FnTestEncoding* fn;
    const char* name;
};

struct TestSuite
{
    TestFunctionArgs args;
    TestCase* test_cases;
    u32 test_case_count;
    u32 passed_test_case_count;

    static TestSuite create(TestFunctionArgs args, TestCase* test_cases, u32 test_case_count)
    {
        TestSuite ts = {
            .args = args,
            .test_cases = test_cases,
            .test_case_count = test_case_count,
        };
        return ts;
    }

    bool run()
    {
        u32 i;
        for (i = 0; i < test_case_count; i++)
        {
            GlobalBuffer global_buffer = GlobalBuffer::create(args.general_allocator, 1024);
            Program program = { .data = global_buffer, .import_libraries = ImportLibraryBuffer::create(args.general_allocator, 16), .entry_point = {}, };
            args.program = &program;
            args.program->functions = FunctionBuilderBuffer::create(args.general_allocator, test_case_count * 2);

            auto& test_case = test_cases[i];

            auto result = test_case.fn(args.general_allocator, args.virtual_allocator, args.value_buffer, args.descriptor_buffer, args.program);

            const char* test_result_msg = result ? "OK" : "FAILED";
            passed_test_case_count += result;
            printf("%s [%s]\n", test_case.name, test_result_msg);
        }

        auto tests_run = i + bool(test_case_count != i);
        printf("Tests passed: %u. Tests failed: %u. Tests run: %u. Total tests: %u\n", passed_test_case_count, tests_run - passed_test_case_count, tests_run, test_case_count);

        return passed_test_case_count == test_case_count;
    }
};

s32 x86_64_test_main(s32 argc, char* argv[])
{
    DebugAllocator test_allocator = {};
    DebugAllocator virtual_allocator = {};

    s64 test_allocator_size = 1024*1024*1024;

    void* address = RNS::virtual_alloc(nullptr, test_allocator_size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 });
    test_allocator.pool = { (u8*)address, 0, test_allocator_size };
    void* execution_address = RNS::virtual_alloc(nullptr, test_allocator_size, { .commit = 1, .reserve = 1, .execute = 1, .read = 1, .write = 1 });
    virtual_allocator = virtual_allocator.create(execution_address, test_allocator_size);


    s64 value_count = 10000;
    s64 descriptor_count = 1000;
    ValueBuffer value_buffer = ValueBuffer::create(&test_allocator, value_count);
    DescriptorBuffer descriptor_buffer = DescriptorBuffer::create(&test_allocator, descriptor_count);

#if TEST_ENCODING
    test_main(&general_allocator, &value_buffer, &descriptor_buffer);
#else
    TestFunctionArgs args = {
        .general_allocator = &test_allocator,
        .virtual_allocator = &virtual_allocator,
        .value_buffer = &value_buffer,
        .descriptor_buffer = &descriptor_buffer,
    };

#define def_test_case(_x_) { _x_, #_x_ }

    TestCase test_cases[] =
    {
        def_test_case(test_basic_conditional),
        def_test_case(test_partial_application),
        def_test_case(test_call_puts),
        def_test_case(test_rns_add_sub),
        def_test_case(test_multiply),
        def_test_case(test_divide),
        def_test_case(test_array_loop),
        def_test_case(test_structs),
        def_test_case(tests_arguments_on_the_stack),
        def_test_case(test_type_checker),
        def_test_case(test_fibonacci),
        def_test_case(test_basic_parametric_polymorphism),
        def_test_case(test_sizeof_values),
        def_test_case(test_sizeof_types),
        def_test_case(test_struct_reflection),
        def_test_case(test_adhoc_polymorphism),
        def_test_case(test_rip_addressing_mode),
        def_test_case(test_tagged_unions),
        def_test_case(test_hello_world_lea),
        def_test_case(test_large_size_return),
        def_test_case(test_shared_library),
        def_test_case(test_write_minimal_executable),
        def_test_case(test_write_hello_world_executable),
    };

    auto test_suite = TestSuite::create(args, test_cases, rns_array_length(test_cases));
    s32 exit_code = test_suite.run() ? 0 : -1;
    if (exit_code)
    {
        return exit_code;
    }
    return exit_code;
#endif
}


using WASMInstruction = WASMBC::InstructionStruct;
using WASMInstructionBuffer = Buffer<WASMInstruction>;

struct WASM_JIT
{
    WASMInstructionBuffer* wasm_instructions;
    s64 instruction_index;
    struct
    {
        IDBuffer wasm_indices;
        IDBuffer mc_indices;
    } stack;
    

    static WASM_JIT create(Allocator* allocator, WASMInstructionBuffer* wasm_instructions)
    {
        s64 starting_instruction = 0;
        assert(wasm_instructions);
        assert(wasm_instructions->len);
        auto* first_instruction = &wasm_instructions->ptr[0];

        if (first_instruction->id == WASMBC::Instruction::global_get && first_instruction->value == WASMBC::__stack_pointer)
        {
            for (s64 i = 0; i < wasm_instructions->len; i++)
            {
                auto& instr = wasm_instructions->ptr[i];
                if (instr.id == WASMBC::Instruction::i32_sub)
                {
                    starting_instruction = i + 2;
                    break;
                }
            }

            if (starting_instruction == 0)
            {
                RNS_UNREACHABLE;
            }
        }

        WASM_JIT result = {
            .wasm_instructions = wasm_instructions,
            .instruction_index = starting_instruction,
            .stack = {
                .wasm_indices = IDBuffer::create(allocator, 1024),
                .mc_indices = IDBuffer::create(allocator, 1024),
            },
        };

        return result;
    }

    WASMInstruction* next_instruction()
    {
        if (instruction_index < wasm_instructions->len)
        {
            auto* instruction = &wasm_instructions->ptr[instruction_index];
            instruction_index++;
            return instruction;
        }

        return nullptr;
    }

    WASMInstruction* previous_instruction()
    {
        instruction_index--;
        auto* instruction = &wasm_instructions->ptr[instruction_index];
        return instruction;
    }

    // @Info: this doesn't increment the "instruction pointer" aka instruction index_ref iterator
    WASMInstruction* peek_next_instruction()
    {
        WASMInstruction* instruction = nullptr;
        if (instruction_index + 1 < wasm_instructions->len)
        {
            instruction = &wasm_instructions->ptr[instruction_index];
        }
        assert(instruction);
        return instruction;
    }

    void append_stack_index(ID wasm_index, ID mc_index)
    {
        stack.wasm_indices.append(wasm_index);
        stack.mc_indices.append(mc_index);
    }

    ID find_stack_index(ID stack_offset)
    {
        for (auto i = 0; i < stack.wasm_indices.len; i++)
        {
            auto index = stack.wasm_indices.ptr[i];
            if (index == stack_offset)
            {
                auto result = stack.mc_indices.ptr[i];
                return result;
            }
        }

        RNS_UNREACHABLE;
        return WASMBC::no_value;
    }
};

struct RegisterAllocator
{
    WASMBC::WASM_ID wasm_registers[register_count_per_size];
    Value* mc_registers[register_count_per_size];

    Value* allocate(ValueBuffer* value_buffer, WASMBC::WASM_ID wasm_id, Descriptor* descriptor)
    {
        auto size = descriptor_size(descriptor);

        for (auto i = 0; i < rns_array_length(wasm_registers); i++)
        {
            auto& index = wasm_registers[i];
            if (index == WASMBC::no_value)
            {
                auto* reg_allocated = reg_value(value_buffer, static_cast<Register>(i), descriptor);
                wasm_registers[i] = wasm_id;
                mc_registers[i] = reg_allocated;
                return reg_allocated;
            }
        }

        RNS_UNREACHABLE;
        return nullptr;
    }

    Value* find_register(WASMBC::WASM_ID wasm_id)
    {
        for (auto i = 0; i < rns_array_length(wasm_registers); i++)
        {
            auto index = wasm_registers[i];
            if (index == wasm_id)
            {
                return mc_registers[i];
            }
        }

        RNS_UNREACHABLE;
        return nullptr;
    }

    Value* free_register(WASMBC::WASM_ID wasm_id)
    {
        for (auto i = 0; i < rns_array_length(wasm_registers); i++)
        {
            auto& index_ref = wasm_registers[i];
            if (index_ref == wasm_id)
            {
                index_ref = WASMBC::no_value;
                auto& mc_ref = mc_registers[i];
                auto* popped_value = mc_ref;
                mc_ref = nullptr;
                return popped_value;
            }
        }

        RNS_UNREACHABLE;
        return nullptr;
    }
};

void jit_wasm(WASMBC::InstructionBuffer* wasm_instructions, WASMBC::WASM_ID stack_pointer_id)
{
    RNS_PROFILE_FUNCTION();
    DebugAllocator general_allocator = {};
    DebugAllocator virtual_allocator = {};

    s64 test_allocator_size = 1024 * 1024;

    void* address = RNS::virtual_alloc(nullptr, test_allocator_size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 });
    general_allocator.pool = { (u8*)address, 0, test_allocator_size };
    void* execution_address = RNS::virtual_alloc(nullptr, test_allocator_size, { .commit = 1, .reserve = 1, .execute = 1, .read = 1, .write = 1 });
    virtual_allocator = virtual_allocator.create(execution_address, test_allocator_size);


    s64 value_count = 10000;
    s64 descriptor_count = 1000;
    ValueBuffer value_buffer = ValueBuffer::create(&general_allocator, value_count);
    DescriptorBuffer descriptor_buffer = DescriptorBuffer::create(&general_allocator, descriptor_count);

    /* PREAMBLE END */

    GlobalBuffer global_buffer = GlobalBuffer::create(&general_allocator, 1024);
    Program program = { .data = global_buffer, .import_libraries = ImportLibraryBuffer::create(&general_allocator, 16), .entry_point = {}, };
    program.functions = FunctionBuilderBuffer::create(&general_allocator, 16);
    WASM_JIT wasm_jit = WASM_JIT::create(&general_allocator, wasm_instructions);

    auto* main = fn_begin(&general_allocator, &value_buffer, &descriptor_buffer, &program);

    RegisterAllocator register_allocator; 
    memset(register_allocator.wasm_registers, WASMBC::no_value, sizeof(register_allocator.wasm_registers));
    memset(register_allocator.mc_registers, 0, sizeof(register_allocator.mc_registers));

    for (WASMInstruction* instruction = wasm_jit.next_instruction(); instruction; instruction = wasm_jit.next_instruction())
    {
        auto instr_id = instruction->id;
        
        switch (instr_id)
        {
            case WASMBC::Instruction::i32_const:
            {
                u32 i32_const_value = instruction->value;
                // @TODO: consider unsigned numbers
                Descriptor* descriptor = &descriptor_s32;
                Value* i32_const_mc_value = s32_value(&value_buffer, i32_const_value);

                instruction = wasm_jit.next_instruction();
                instr_id = instruction->id;

                switch (instr_id)
                {
                    case WASMBC::Instruction::local_set:
                    {
                        auto vr_index = instruction->value;

                        auto* next_instruction_peeked = wasm_jit.peek_next_instruction();
                        assert(next_instruction_peeked);

                        if (next_instruction_peeked->id == WASMBC::Instruction::local_get && next_instruction_peeked->value == stack_pointer_id)
                        {
                            // @Info: consume the local_get stack_pointer_id
                            wasm_jit.next_instruction();
                            instruction = wasm_jit.next_instruction();
                            instr_id = instruction->id;

                            switch (instr_id)
                            {
                                case WASMBC::Instruction::local_get:
                                {
                                    if (instruction->value == vr_index)
                                    {
                                        instruction = wasm_jit.next_instruction();
                                        instr_id = instruction->id;

                                        switch (instr_id)
                                        {
                                            case WASMBC::Instruction::i32_store:
                                            {
                                                auto stack_offset = instruction->value;
                                                auto* i32_const_stack = Stack(&value_buffer, main, descriptor, i32_const_mc_value);
                                                auto mc_index = value_buffer.get_id(i32_const_stack);
                                                wasm_jit.append_stack_index(stack_offset, mc_index);
                                            } break;
                                            default:
                                                RNS_NOT_IMPLEMENTED;
                                                break;
                                        }
                                    }
                                    else
                                    {
                                        RNS_UNREACHABLE;
                                    }
                                } break;
                                default:
                                    RNS_NOT_IMPLEMENTED;
                                    break;
                            }
                        }
                        // @Info: do a normal set in a virtual register (aka for this one: physical register if possible)
                        else
                        {
                            RNS_NOT_IMPLEMENTED;
                        }
                    } break;
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }
            } break;
            case WASMBC::Instruction::local_get:
            {
                u32 value = instruction->value;
                // stack operation
                if (value == stack_pointer_id)
                {
                    instruction = wasm_jit.next_instruction();
                    instr_id = instruction->id;

                    switch (instr_id)
                    {
                        case WASMBC::Instruction::local_get:
                        {
                            auto vr_index = instruction->value;
                            auto* value_in_reg = register_allocator.find_register(vr_index);
                            assert(value_in_reg);

                            instruction = wasm_jit.next_instruction();
                            instr_id = instruction->id;

                            switch (instr_id)
                            {
                                case WASMBC::Instruction::i32_store:
                                {
                                    auto stack_offset = instruction->value;
                                    auto* i32_store = Stack(&value_buffer, main, value_in_reg->descriptor, value_in_reg);
                                    auto mc_index = value_buffer.get_id(i32_store);
                                    wasm_jit.append_stack_index(stack_offset, mc_index);
                                    register_allocator.free_register(vr_index);
                                } break;
                                default:
                                    RNS_NOT_IMPLEMENTED;
                                    break;
                            }
                        } break;
                        case WASMBC::Instruction::i32_load:
                        {
                            auto wasm_stack_offset = instruction->value;
                            auto mc_stack_value_index = wasm_jit.find_stack_index(wasm_stack_offset);
                            auto* mc_stack_value = value_buffer.get(mc_stack_value_index);

                            instruction = wasm_jit.next_instruction();
                            instr_id = instruction->id;

                            switch (instr_id)
                            {
                                case WASMBC::Instruction::local_set:
                                {
                                    auto wasm_new_reg_index = instruction->value;
                                    auto* mc_reg_value = register_allocator.allocate(&value_buffer, wasm_new_reg_index, mc_stack_value->descriptor);
                                    assert(mc_reg_value);
                                    move_value(&value_buffer, main, mc_reg_value, mc_stack_value);

                                    // @Info: if we are loading a value into a register, it's most definitely because we are going to operate with it, then continue scanning

                                    auto* next_instruction = wasm_jit.peek_next_instruction();
                                    switch (next_instruction->id)
                                    {
                                        case WASMBC::Instruction::local_get:
                                            break;
                                        case WASMBC::Instruction::i32_const:
                                        {
                                            // consume the instruction
                                            instruction = wasm_jit.next_instruction();
                                            instr_id = instruction->id;

                                            auto i32_const_wasm_value = instruction->value;
                                            auto* i32_const_mc_value = s32_value(&value_buffer, i32_const_wasm_value);

                                            instruction = wasm_jit.next_instruction();
                                            instr_id = instruction->id;

                                            switch (instr_id)
                                            {
                                                case WASMBC::Instruction::local_set:
                                                {
                                                    // @Info: skip instruction, for now we don't need to store the constant in a register
                                                    auto i32_const_wasm_index = instruction->value;
                                                    
                                                    instruction = wasm_jit.next_instruction();
                                                    instr_id = instruction->id;

                                                    switch (instr_id)
                                                    {
                                                        case WASMBC::Instruction::local_get:
                                                        {
                                                            auto wasm_first_operand_index = instruction->value;
                                                            Value* mc_first_operand_value = nullptr;
                                                            // @TODO: we should consider doing a special kind of constant map between the constant value and the wasm index
                                                            // so we can pick them up and push to them there in a way they are only stored in memory or register when necessary.
                                                            // Should it also be necessary to pop/delete them from the buffer?
                                                            if (wasm_first_operand_index == i32_const_wasm_index)
                                                            {
                                                                mc_first_operand_value = i32_const_mc_value;
                                                            }
                                                            else
                                                            {
                                                                mc_first_operand_value = register_allocator.find_register(wasm_first_operand_index);
                                                            }
                                                            assert(mc_first_operand_value);

                                                            instruction = wasm_jit.next_instruction();
                                                            instr_id = instruction->id;

                                                            switch (instr_id)
                                                            {
                                                                case WASMBC::Instruction::local_get:
                                                                {
                                                                    auto wasm_second_operand_index = instruction->value;
                                                                    Value* mc_second_operand_value = nullptr;
                                                                    if (wasm_second_operand_index == i32_const_wasm_index)
                                                                    {
                                                                        mc_second_operand_value = i32_const_mc_value;
                                                                    }
                                                                    else
                                                                    {
                                                                        mc_second_operand_value = register_allocator.find_register(wasm_second_operand_index);
                                                                    }
                                                                    assert(mc_second_operand_value);

                                                                    instruction = wasm_jit.next_instruction();
                                                                    instr_id = instruction->id;

                                                                    switch (instr_id)
                                                                    {
                                                                        case WASMBC::Instruction::i32_add:
                                                                        {
                                                                            main->instruction_buffer.append({ add, {mc_first_operand_value->operand, mc_second_operand_value->operand } }, METACONTEXT);
                                                                            assert(mc_first_operand_value->operand.type == OperandType::Register);
                                                                            auto* mc_result_value = mc_first_operand_value;

                                                                            instruction = wasm_jit.next_instruction();
                                                                            instr_id = instruction->id;

                                                                            switch (instr_id)
                                                                            {
                                                                                case WASMBC::Instruction::local_set:
                                                                                {
                                                                                    auto wasm_result_index = instruction->value;

                                                                                    instruction = wasm_jit.next_instruction();
                                                                                    instr_id = instruction->id;

                                                                                    switch (instr_id)
                                                                                    {
                                                                                        case WASMBC::Instruction::local_get:
                                                                                        {
                                                                                            auto wasm_first_operand_index = instruction->value;
                                                                                            assert(wasm_first_operand_index == stack_pointer_id);

                                                                                            instruction = wasm_jit.next_instruction();
                                                                                            instr_id = instruction->id;
                                                                                            switch (instr_id)
                                                                                            {
                                                                                                case WASMBC::Instruction::local_get:
                                                                                                {
                                                                                                    auto wasm_second_operand_index = instruction->value;
                                                                                                    assert(wasm_second_operand_index == wasm_result_index);

                                                                                                    instruction = wasm_jit.next_instruction();
                                                                                                    instr_id = instruction->id;

                                                                                                    switch (instr_id)
                                                                                                    {
                                                                                                        case WASMBC::Instruction::i32_store:
                                                                                                        {
                                                                                                            auto wasm_stack_offset = instruction->value;
                                                                                                            auto* store_value = Stack(&value_buffer, main, mc_result_value->descriptor, mc_result_value);
                                                                                                            auto mc_index = value_buffer.get_id(store_value);
                                                                                                            wasm_jit.append_stack_index(wasm_stack_offset, mc_index);
                                                                                                            assert(wasm_result_index == wasm_second_operand_index);
                                                                                                            // @Info: this variable is the virtual register for the load, which is to be the same physical register
                                                                                                            // than the binop result.
                                                                                                            // @TODO: improve this, it's awful
                                                                                                            register_allocator.free_register(wasm_new_reg_index);
                                                                                                        } break;
                                                                                                        default:
                                                                                                            RNS_NOT_IMPLEMENTED;
                                                                                                            break;
                                                                                                    }
                                                                                                } break;
                                                                                                default:
                                                                                                    RNS_NOT_IMPLEMENTED;
                                                                                                    break;
                                                                                            }
                                                                                        } break;
                                                                                        default:
                                                                                            RNS_NOT_IMPLEMENTED;
                                                                                            break;
                                                                                    }
                                                                                } break;
                                                                                default:
                                                                                    RNS_NOT_IMPLEMENTED;
                                                                                    break;
                                                                            }
                                                                        } break;
                                                                        default:
                                                                            RNS_NOT_IMPLEMENTED;
                                                                            break;
                                                                    }
                                                                } break;
                                                                default:
                                                                    RNS_NOT_IMPLEMENTED;
                                                                    break;
                                                            }
                                                        } break;
                                                        default:
                                                            RNS_NOT_IMPLEMENTED;
                                                            break;
                                                    }
                                                } break;
                                                default:
                                                    RNS_NOT_IMPLEMENTED;
                                                    break;
                                            }
                                        } break;
                                        default:
                                            RNS_NOT_IMPLEMENTED;
                                            break;
                                    }
                                } break;
                                default:
                                    RNS_NOT_IMPLEMENTED;
                                    break;
                            }
                        } break;
                        default:
                            RNS_NOT_IMPLEMENTED;
                            break;
                    }
                }
                else
                {
                    // @Info: if a binary operation occurs, this index_ref will be invalidated and the new SSA index will take his place with the result updated,
                    // which is the standard behavior for x86_64 basic bin ops
                    u32 wasm_first_index = instruction->value;
                    auto* mc_first_reg_value = register_allocator.find_register(wasm_first_index);
                    assert(mc_first_reg_value);

                    instruction = wasm_jit.next_instruction();
                    instr_id = instruction->id;

                    switch (instr_id)
                    {
                        case WASMBC::Instruction::local_get:
                        {
                            u32 wasm_second_index = instruction->value;
                            auto* mc_second_reg_value = register_allocator.find_register(wasm_second_index);

                            instruction = wasm_jit.next_instruction();
                            instr_id = instruction->id;

                            switch (instr_id)
                            {
                                case WASMBC::Instruction::i32_add:
                                {
                                    // @Info: We do a raw add since the one implemented stack-allocates a lot
                                    main->instruction_buffer.append({ add, {mc_first_reg_value->operand, mc_second_reg_value->operand }}, METACONTEXT);
                                    auto* reg_mc_result_value = mc_first_reg_value;

                                    instruction = wasm_jit.next_instruction();
                                    instr_id = instruction->id;

                                    switch (instr_id)
                                    {
                                        case WASMBC::Instruction::local_set:
                                        {
                                            auto wasm_result_reg_index = instruction->value;

                                            auto* next_instruction = wasm_jit.peek_next_instruction();
                                            assert(next_instruction);

                                            if (next_instruction->id == WASMBC::Instruction::local_get) 
                                            {
                                                instruction = wasm_jit.next_instruction();

                                                if (instruction->value == wasm_result_reg_index)
                                                {
                                                    instruction = wasm_jit.next_instruction();
                                                    instr_id = instruction->id;

                                                    switch (instr_id)
                                                    {
                                                        case WASMBC::Instruction::ret:
                                                        {
                                                            // @TODO: improve this
                                                            bool no_jump_from_context = true;
                                                            fn_return(&value_buffer, main, reg_mc_result_value, true);
                                                        } break;
                                                        default:
                                                            RNS_NOT_IMPLEMENTED;
                                                            break;
                                                    }
                                                }
                                                else
                                                {
                                                    RNS_NOT_IMPLEMENTED;
                                                }
                                            }
                                            else
                                            {
                                                RNS_NOT_IMPLEMENTED;
                                            }
                                        } break;
                                        default:
                                            RNS_NOT_IMPLEMENTED;
                                            break;
                                    }
                                } break;
                                default:
                                    RNS_NOT_IMPLEMENTED;
                                    break;
                            }
                        } break;
                        default:
                            RNS_NOT_IMPLEMENTED;
                            break;
                    }
                }
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    fn_end(main);
    auto jit = jit_end(&virtual_allocator, &program);

    auto main_fn = value_as_function(main->value, RetS32ParamVoid);
    auto result = main_fn();

    auto a = 5; auto b = a + 4; auto expected = b + a;
    assert(result == expected);
}
