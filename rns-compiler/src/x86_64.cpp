#include <RNS/types.h>
#include <RNS/compiler.h>
#include <RNS/os.h>
#include <RNS/data_structures.h>

#include <stdio.h>
#include <immintrin.h>

using namespace RNS;

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
    RIP_relative,
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
    u64 address;
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
    auto* label = new Label({
        .label_size = size,
        .locations = new(allocator) LabelLocation[max_label_location_count],
        .location_count = 0,
        });

    return label;
}

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
        //union
        //{
        //    u8  rel8;
        //    u16 rel16;
        //    u32 rel32;
        //    struct
        //    {
        //        u32 first;
        //        u16 second;
        //    } rel48;
        //    u64 rel64;
        //    struct
        //    {
        //        u64 first;
        //        u16 second;
        //    } rel80;
        //};
        Label* relative;
        Register reg;
        OperandMemoryIndirect indirect;
        OperandRIPRelative rip_rel;
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
const u32 max_arg_count = 16;
struct DescriptorFunction
{
    Value* arg_list;
    s64 arg_count;
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
    s64 field_count;
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
    s64 field_count = descriptor->field_count;
    assert(field_count);
    s64 alignment = 0;
    s64 raw_size = 0;

    for (s64 i = 0; i < field_count; i++)
    {
        DescriptorStructField* field = &descriptor->field_list[i];
        s64 field_size = descriptor_size(field->descriptor);
        alignment = max(alignment, field_size);
        bool is_last_field = i == field_count - 1;
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


template <typename T>
struct Buffer
{
    T* ptr;
    s64 len;
    s64 cap;
    Allocator* allocator;

    static Buffer<T> create(Allocator* allocator, s64 count)
    {
        Buffer<T> buffer = {
            .ptr = new(allocator) T[count],
            .len = 0,
            .cap = count,
            .allocator = allocator,
        };
        return buffer;
    }

    T* allocate(s64 count)
    {
        assert(len + count < cap);
        auto result = &ptr[len];
        len += count;
        return result;
    }

    T* allocate()
    {
        return allocate(1);
    }

    void append(T element)
    {
        assert(len + 1 < cap);
        ptr[len++] = element;
    }
};

struct StackPatch
{
    s32* location;
    u32 size;
};

struct StructBuilderField
{
    DescriptorStructField descriptor;
    struct StructBuilderField* next;
};

struct StructBuilder
{
    s64 offset;
    s64 field_count;
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
    Register_Or_Memory,
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
};



using DescriptorBuffer = Buffer<Descriptor>;
using ValueBuffer = Buffer<Value>;
using StructBuilderFieldBuffer = Buffer<StructBuilderField>;
using InstructionBuffer = Buffer<Instruction>;

#define MAX_DISPLACEMENT_COUNT 128

struct ExecutionBuffer : public Buffer<u8>
{
    static ExecutionBuffer create(Allocator* allocator, s64 count)
    {
        assert(allocator);
        assert(count > 0);
        ExecutionBuffer buffer;
        buffer.ptr = new(allocator) u8[count];
        buffer.len = 0;
        buffer.cap = count;
        buffer.allocator = allocator;

        assert(buffer.ptr);

        return buffer;
    }

    void* allocate_bytes(s64 bytes)
    {
        assert(len + bytes < cap);
        auto* result =  &ptr[len];
        len += bytes;
        return result;
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

using GlobalBuffer = ExecutionBuffer;

const u32 max_instruction_count = 4096;
struct FunctionBuilder
{
    InstructionBuffer instruction_buffer;
    Descriptor* descriptor;
    ExecutionBuffer* eb;
    StackPatch* stack_displacements;
    s64 stack_displacement_count;
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
    s64 field_count;
} DescriptorStructReflection;

DescriptorStructField struct_reflection_fields[] =
{
    { .name = "field_count", .descriptor = &descriptor_s64, .offset = 0, },
};

Descriptor descriptor_struct_reflection = {
    .type = DescriptorType::Struct,
    .struct_ = {
        .field_list = struct_reflection_fields,
        .field_count = rns_array_length(struct_reflection_fields),
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

static inline Operand rip_rel(u64 address)
{
    return {
        .type = OperandType::RIP_relative,
        .size = static_cast<u32>(OperandSize::Bits_64),
        .rip_rel{.address = address},
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
    assert(size > 0 && size <= UINT32_MAX);
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

static inline Value* global_value(GlobalBuffer* global_buffer, ValueBuffer* value_buffer, Descriptor* descriptor)
{
    auto value_size = descriptor_size(descriptor);
    auto* global_allocated_blob = global_buffer->allocate_bytes(value_size);

    auto* value = value_buffer->allocate();
    *value = {
        .descriptor = descriptor,
        .operand = {
            .type = OperandType::RIP_relative,
            .size = static_cast<u32>(value_size),
            .imm64 = (u64)global_allocated_blob,
        },
    };

    return value;
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

Descriptor * C_parse_type(DescriptorBuffer* descriptor_buffer, const char *range_start, const char *range_end
) {
  Descriptor *descriptor = 0;

  const char *start = range_start;
  for(const char *ch = range_start; ch <= range_end; ++ch) {
    if (!(*ch == ' ' || *ch == '*' || ch == range_end)) continue;
    if (start != ch) {
      if (memory_range_equal_to_c_string(start, ch, "char")) {
        descriptor = (Descriptor*)&descriptor_s8;
      } else if (memory_range_equal_to_c_string(start, ch, "int")) {
        descriptor = (Descriptor*)&descriptor_s32;
      } else if (memory_range_equal_to_c_string(start, ch, "void")) {
        descriptor = (Descriptor*)&descriptor_void;
      } else if (memory_range_equal_to_c_string(start, ch, "const")) {
        // TODO support const values?
      } else {
        assert(!"Unsupported argument type");
      }
    }
    if (*ch == '*') {
      assert(descriptor);
      Descriptor *previous_descriptor = descriptor;
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

Value* C_function_return_value(DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, const char *forward_declaration) {
    const char *ch = strchr(forward_declaration, '(');
    assert(ch);
    --ch;

    // skip whitespace before (
    for(; *ch == ' '; --ch);
    for(;
      (*ch >= 'a' && *ch <= 'z') ||
      (*ch >= 'A' && *ch <= 'Z') ||
      (*ch >= '0' && *ch <= '9') ||
      *ch == '_';
      --ch
    )
    // skip whitespace before function name
    for(; *ch == ' '; --ch);
    ++ch;
    Descriptor *descriptor = C_parse_type(descriptor_buffer, forward_declaration, ch);
    assert(descriptor);
    switch(descriptor->type) {
      case DescriptorType::Void: {
        return &void_value;
      }
      case DescriptorType::Function:
      case DescriptorType::Integer:
      case DescriptorType::Pointer: {
          Value* return_value = value_buffer->allocate();
        *return_value = {
          .descriptor = descriptor,
          .operand = reg.eax,
        };
        return return_value;
      }
      case DescriptorType::TaggedUnion:
      case DescriptorType::FixedSizeArray:
      case DescriptorType::Struct:
      default: {
        assert(!"Unsupported return type");
      }
    }
    return 0;
}

Value* C_function_value(DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, const char* forward_declaration, u64 fn) {
    Descriptor* descriptor = descriptor_buffer->allocate();
    *descriptor = {
      .type = DescriptorType::Function,
      .function = {0},
    };
    Value* result = value_buffer->allocate();
    *result = {
      .descriptor = descriptor,
      .operand = imm64(fn),
    };

    result->descriptor->function.return_value = C_function_return_value(descriptor_buffer, value_buffer, forward_declaration);
    const char* ch = strchr(forward_declaration, '(');
    assert(ch);
    ch++;

    const char* start = ch;
    Descriptor* argument_descriptor = 0;
    for (; *ch; ++ch) {
        if (*ch == ',' || *ch == ')') {
            if (start != ch) {
                argument_descriptor = C_parse_type(descriptor_buffer, start, ch);
                assert(argument_descriptor);
            }
            start = ch + 1;
        }
    }

    if (argument_descriptor && argument_descriptor->type != DescriptorType::Void) {
        Value* arg = value_buffer->allocate();
        arg->descriptor = argument_descriptor;
        // FIXME should not use a hardcoded register here
        arg->operand = reg.rcx;

        result->descriptor->function.arg_list = arg;
        result->descriptor->function.arg_count = 1;
    }

    return (result);
}

using InstructionBlock = Buffer<Instruction>;

struct Program
{
    /*
    * ImportLibrary array
    * Label array
    * patch array
    * startup functions
    * entry point
    * functions (array of function_builders?)
    * program memory
    */
};

struct Scope
{
    /*
    * id
    * flags
    * parent
    * scope_map
    * macros???
    * statement matcher
    */
};

struct Function
{
    InstructionBlock block;
    s64 stack_reserved;
    bool frozen;
    u32 max_call_parameters_stack_size;
};

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
                        if (operand_encoding.type == OperandEncodingType::Register_Or_Memory && (u32)operand_encoding.size == operand.size)
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
                        if (operand_encoding.type == OperandEncodingType::Register_Or_Memory)
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
                    case OperandType::RIP_relative:
                        if (operand_encoding.type == OperandEncodingType::Register_Or_Memory)
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
            if (a->function.arg_count != b->function.arg_count)
            {
                return false;
            }
            if (!typecheck(a->function.return_value->descriptor, b->function.return_value->descriptor))
            {
                return false;
            }

            for (s64 i = 0; i < a->function.arg_count; i++)
            {
                if (!typecheck(a->function.arg_list[i].descriptor, b->function.arg_list[i].descriptor))
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

void encode_instruction(FunctionBuilder* fn_builder, Instruction instruction)
{
    assert(fn_builder->eb);
    u64 instruction_start_offset = (u64)(fn_builder->eb->ptr + fn_builder->eb->len);

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
            assert(difference >= INT32_MIN && difference <= INT32_MAX);
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
        else if (op_encoding.type == OperandEncodingType::Register_Or_Memory)
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

    u64 offset_of_displacement = 0;
    u32 stack_size = 0;
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
                    need_sib = operand.indirect.reg == get_stack_register();

                    if (need_sib)
                    {
                        sib_byte = ((u8)SIB::Scale_1 << 6) | (r_m << 3) | (r_m);
                    }
                    break;
                }
                case OperandType::RIP_relative:
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
        fn_builder->eb->append(rex_byte);
    }
    else if ((instruction.operands[0].type == OperandType::Register && instruction.operands[0].size == (u32)OperandSize::Bits_16) || (instruction.operands[1].type == OperandType::Register && instruction.operands[1].size == (u32)OperandSize::Bits_16) || encoding.options.explicit_byte_size == static_cast<u8>(OperandSize::Bits_16))
    {
        fn_builder->eb->append(OperandSizeOverride);
    }

    for (u32 i = 0; i < max_op_code_bytes; i++)
    {
        u8 op_code_byte = op_code[i];
        if (op_code_byte)
        {
            fn_builder->eb->append(op_code_byte);
        }
    }

    if (need_mod_rm)
    {
        fn_builder->eb->append(mod_r_m);
    }
    // SIB
    if (need_sib)
    {
        fn_builder->eb->append(sib_byte);
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
                    switch ((Mod)mod)
                    {
                        case Mod::Displacement_8:
                            fn_builder->eb->append((s8)op.indirect.displacement);
                            break;
                        case Mod::Displacement_32:
                            if (need_sib)
                            {
                                offset_of_displacement = fn_builder->eb->len;
                                stack_size = static_cast<u8>(op.size);
                                assert(fn_builder->stack_displacement_count < MAX_DISPLACEMENT_COUNT);
                                s32* location = (s32*)(fn_builder->eb->ptr + offset_of_displacement);
                                fn_builder->stack_displacements[fn_builder->stack_displacement_count] = {
                                    .location = location,
                                    .size = stack_size,
                                };
                                fn_builder->stack_displacement_count++;
                            }
                            fn_builder->eb->append(op.indirect.displacement);
                            break;
                        default:
                            break;
                    }
                    break;
                case OperandType::RIP_relative:
                {
                    u64 start_address = (u64)fn_builder->eb->ptr;
                    u64 end_address = start_address + fn_builder->eb->cap;
                    u64 next_instruction_address = start_address + fn_builder->eb->len + sizeof(s32);
                    s64 displacement = op.rip_rel.address - next_instruction_address;
                    assert(displacement < INT32_MAX && displacement > INT32_MIN);
                    fn_builder->eb->append((s32)displacement);
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
                    fn_builder->eb->append(operand.imm8);
                    break;
                case OperandSize::Bits_16:
                    fn_builder->eb->append( operand.imm16);
                    break;
                case OperandSize::Bits_32:
                    fn_builder->eb->append( operand.imm32);
                    break;
                case OperandSize::Bits_64:
                    fn_builder->eb->append( operand.imm64);
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
                u8* from = fn_builder->eb->ptr + fn_builder->eb->len + static_cast<u8>(label_size);
                u8* target = (u8*)(operand.relative->target);
                auto difference = target - from;

                switch (label_size)
                {
                case OperandSize::Bits_8:
                    assert(difference >= INT8_MIN && difference <= INT8_MAX);
                    fn_builder->eb->append((s8)difference);
                    break;
                case OperandSize::Bits_16:
                    assert(difference >= INT16_MIN && difference <= INT16_MAX);
                    fn_builder->eb->append((s16)difference);
                    break;
                case OperandSize::Bits_32:
                    assert(difference >= INT32_MIN && difference <= INT32_MAX);
                    fn_builder->eb->append((s32)difference);
                    break;
                case OperandSize::Bits_64:
                    fn_builder->eb->append(difference);
                    break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
                }
            }
            else
            {
                assert(operand.relative->location_count < max_label_location_count);
                u64 patch_target = (u64)(fn_builder->eb->ptr + fn_builder->eb->len);

                switch ((OperandSize)operand.size)
                {
                    case OperandSize::Bits_8:
                        fn_builder->eb->append(stack_fill_value_8);
                        break;
                    case OperandSize::Bits_16:
                        fn_builder->eb->append(stack_fill_value_16);
                        break;
                    case OperandSize::Bits_32:
                        fn_builder->eb->append(stack_fill_value_32);
                        break;
                    case OperandSize::Bits_64:
                        fn_builder->eb->append(stack_fill_value_64);
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

FunctionBuilder fn_begin(Allocator* allocator, ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, ExecutionBuffer* execution_buffer)
{
    Descriptor* descriptor = descriptor_buffer->allocate();
    *descriptor =
    {
        .type = DescriptorType::Function,
        .function = {
            .arg_list = value_buffer->allocate(max_arg_count),
        }
    };

    // @Test @NoExecutionNeeded
    if (!execution_buffer)
    {
        execution_buffer = new(allocator) ExecutionBuffer;
        *execution_buffer = ExecutionBuffer::create(allocator, 1024);
    }

    assert(execution_buffer);

    FunctionBuilder fn_builder =
    {
        .instruction_buffer = InstructionBuffer::create(allocator, max_instruction_count),
        .descriptor = descriptor,
        .eb = execution_buffer,
        .stack_displacements = new(allocator) StackPatch[MAX_DISPLACEMENT_COUNT],
        .epilogue_label = label(allocator, OperandSize::Bits_32),
    };

    assert(fn_builder.stack_displacements);

    u64 code_address = u64(fn_builder.eb->ptr + fn_builder.eb->len);
    
    fn_builder.value = value_buffer->allocate();
    Label* fn_rel_address = label(allocator, OperandSize::Bits_32);
    fn_rel_address->target = code_address,
    *fn_builder.value = 
    {
        .descriptor = descriptor,
        .operand = rel(fn_rel_address),
    };

    // @Volatile @ArgCount

    return fn_builder;
}

#define TestEncodingFn(_name_) bool _name_(Allocator* general_allocator, ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, ExecutionBuffer* execution_buffer)
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
        u8 got = eb->ptr[i];
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
        print_chunk_of_bytes_in_hex(eb->ptr, eb->len, "Result:\t\t");
    }

    return success;
}

static bool test_instruction(ExecutionBuffer* eb, const char* test_name, Instruction instruction, u8* expected_bytes, u8 expected_byte_count)
{
    const u32 buffer_size = 64;
    FunctionBuilder fn_builder = {
        .eb = eb,
    };
    s64 size = 1024;
    fn_builder.eb->ptr = reinterpret_cast<u8*>(RNS::virtual_alloc(nullptr, size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 }));
    fn_builder.eb->len = 0;
    fn_builder.eb->cap = size;

    encode_instruction(&fn_builder, instruction);

    ExecutionBuffer expected;
    expected.ptr = reinterpret_cast<u8*>(RNS::virtual_alloc(nullptr, size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 }));
    expected.len = 0;
    expected.cap = size;
    expected.append(expected_bytes, expected_byte_count);
    return test_buffer(fn_builder.eb, expected.ptr, expected.len, test_name);
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
    return operand.type == OperandType::MemoryIndirect || operand.type == OperandType::RIP_relative;
}

void move_value(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, const Value* a, const Value* b)
{
    s64 a_size = descriptor_size(a->descriptor);
    s64 b_size = descriptor_size(b->descriptor);

    if (is_memory_operand(a->operand) && is_memory_operand(b->operand))
    {
        Value* reg_a = reg_value(value_buffer, Register::A, a->descriptor);
        move_value(value_buffer, fn_builder, reg_a, b);
        move_value(value_buffer, fn_builder, a, reg_a);
        return;
    }
   
#if 1
    // @TODO: Maybe this is wrong?
    if (a_size != b_size)
    {
        if (!(b->operand.type == OperandType::Immediate && b_size == sizeof(s32) && a_size == sizeof(s64)))
        {
            assert(!"Mismatched operand size when moving");
        }
    }
    // This doesn't work
#else

    if (b_size == 1 && (a_size >= 2 && a_size <= 8))
    {
        assert(a->operand.type == OperandType::Register);
        Value* zero = s64_value(value_buffer, 0);
        zero->descriptor = a->descriptor;

        move_value(value_buffer, fn_builder, a, zero);
        fn_builder->instruction_buffer.append({ mov, {a->operand, b->operand} });

        // @TODO: use movsx
        //fn_builder->instruction_buffer.append({ movsx, {a->operand, b->operand} });

        return;
    }

    if (a->operand.type == OperandType::Register && b->operand.type == OperandType::Immediate)
    {
        bool operand_immediate_0 = false;
        switch (b->operand.size)
        {
            case 1:
                operand_immediate_0 = b->operand.imm8 == 0;
                break;
            case 2:
                operand_immediate_0 = b->operand.imm16 == 0;
                break;
            case 4:
                operand_immediate_0 = b->operand.imm32 == 0;
                break;
            case 8:
                operand_immediate_0 = b->operand.imm64 == 0;
                break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        if (operand_immediate_0)
        {
            fn_builder->instruction_buffer.append({ xor_, {a->operand, a->operand} });
            return;
        }
    }
#endif

    if ((b_size == (u32)OperandSize::Bits_64 && b->operand.type == OperandType::Immediate && a->operand.type != OperandType::Register) || (a->operand.type == OperandType::MemoryIndirect && b->operand.type == OperandType::MemoryIndirect))
    {
        Value* reg_a = reg_value(value_buffer, Register::A, a->descriptor);
        fn_builder->instruction_buffer.append({ mov, { reg_a->operand, b->operand } });
        fn_builder->instruction_buffer.append({ mov, { a->operand, reg_a->operand } });
    }
    else
    {
        fn_builder->instruction_buffer.append({ mov, { a->operand, b->operand } });
    }
}

Value* Stack(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Descriptor* descriptor, Value* value)
{
    Value* stack_value = stack_reserve(value_buffer, fn_builder, descriptor);
    move_value(value_buffer, fn_builder, stack_value, value);
    return stack_value;
}

Value* fn_reflect(ValueBuffer* value_buffer,  FunctionBuilder* fn_builder, Descriptor* descriptor)
{
    Value* result = stack_reserve(value_buffer, fn_builder, &descriptor_struct_reflection);
    assert(descriptor->type == DescriptorType::Struct);
    move_value(value_buffer, fn_builder, result, (s64_value(value_buffer, descriptor->struct_.field_count)));

    return (result);
}


void fn_update_result(FunctionBuilder* fn_builder)
{
    fn_builder->descriptor->function.arg_count = fn_builder->next_arg;
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

void fn_end(FunctionBuilder* fn_builder)
{
    //assert(!fn_is_frozen(fn_builder));
    // @MSVC
    s32 stack_size;

    switch (calling_convention)
    {
        case CallingConvention::MSVC:
        {
            s8 alignment = 0x8;
            fn_builder->stack_offset += fn_builder->max_call_parameter_stack_size;
            stack_size = (s32)align(fn_builder->stack_offset, 16) + alignment;
            if (stack_size >= INT8_MIN && stack_size <= INT8_MAX)
            {
                encode_instruction(fn_builder, { sub, { reg.rsp, imm8((s8)stack_size) } });
            }
            else
            {
                encode_instruction(fn_builder, { sub, { reg.rsp, imm32(stack_size) } });
            }
        } break;
        case CallingConvention::SystemV:
            encode_instruction(fn_builder, { push, { reg.rbp } });
            encode_instruction(fn_builder, { mov, { reg.rbp, reg.rsp } });
            break;
        default:
            RNS_NOT_IMPLEMENTED;
            break;
    }

    u64* instruction_index_offset = new u64[fn_builder->instruction_buffer.len];

    for (auto i = 0; i < fn_builder->instruction_buffer.len; i++)
    {
        instruction_index_offset[i] = (u64)(fn_builder->eb->ptr + fn_builder->eb->len);
        auto instruction = fn_builder->instruction_buffer.ptr[i];
        encode_instruction(fn_builder, instruction);
    }

    switch (calling_convention)
    {
        case CallingConvention::MSVC:
        {
            for (s64 i = 0; i < fn_builder->stack_displacement_count; i++)
            {
                StackPatch* patch = &fn_builder->stack_displacements[i];
                s32 displacement = *patch->location;
                if (displacement < 0)
                {
                    *patch->location = stack_size + displacement;
                }
                else if (displacement >= (s32)fn_builder->max_call_parameter_stack_size)
                {
                    s8 return_address_size = 8;
                    *patch->location = stack_size + displacement + return_address_size;
                }
            }

            encode_instruction(fn_builder, { .label = fn_builder->epilogue_label });

            encode_instruction(fn_builder, { add, { reg.rsp, imm32(stack_size) } });
            encode_instruction(fn_builder, { ret });
        } break;
        case CallingConvention::SystemV:
        {

            // @TODO: maybe resolve max call parameter stack size
            encode_instruction(fn_builder, { pop, { reg.rbp } });
            encode_instruction(fn_builder, { ret });
        } break;
        default:
        {
            RNS_NOT_IMPLEMENTED;
        } break;
    }

    fn_freeze(fn_builder);
}

Value* fn_arg(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Descriptor* arg_descriptor)
{
    assert(!fn_is_frozen(fn_builder));
    assert(descriptor_size(arg_descriptor) <= 8);
    DescriptorFunction* function = &fn_builder->descriptor->function;
    s64 arg_index = fn_builder->next_arg++;
    assert(arg_index < max_arg_count);
    if (arg_index < rns_array_length(parameter_registers))
    {
        function->arg_list[arg_index] = *reg_value(value_buffer, parameter_registers[arg_index], arg_descriptor);
    }
    else
    {
        switch (calling_convention)
        {
            case CallingConvention::MSVC:
            {
                s32 return_address_size = 8;
                s64 arg_size = descriptor_size(arg_descriptor);
                s64 offset = arg_index * 8;

                function->arg_list[arg_index] = {
                    .descriptor = (Descriptor*)arg_descriptor,
                    .operand = stack((s32)offset, (s32)arg_size),
                };
                break;
            }
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
    }

    fn_update_result(fn_builder);

    return (&function->arg_list[arg_index]);
};

void fn_return(ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Value* to_return)
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
        move_value(value_buffer, fn_builder, function->return_value, to_return);
    }

    fn_builder->instruction_buffer.append({ jmp, { rel(fn_builder->epilogue_label) } });

    (void)fn_update_result(fn_builder);
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
    assert(value->operand.type == OperandType::MemoryIndirect || value->operand.type == OperandType::RIP_relative);
    Descriptor* result_descriptor = descriptor_pointer_to(descriptor_buffer, value->descriptor);
    Value* reg_a = reg_value(value_buffer, Register::A, result_descriptor);
    fn_builder->instruction_buffer.append({
        lea, {reg_a->operand, value->operand}
    });
    Value* result = stack_reserve(value_buffer, fn_builder, result_descriptor);
    move_value(value_buffer, fn_builder, result, reg_a);

    return (result);
}

#define value_as_function(_value_, _type_) ((_type_*)helper_value_as_function(_value_))

Value* call_function_overload(DescriptorBuffer* descriptor_buffer, ValueBuffer* value_buffer, FunctionBuilder* fn_builder, Value* fn, Value** arg_list, s64 arg_count)
{
    DescriptorFunction* descriptor = &fn->descriptor->function;
    assert(fn->descriptor->type == DescriptorType::Function);
    assert(descriptor->arg_count == arg_count);

    fn_ensure_frozen(descriptor);

    for (s64 i = 0; i < arg_count; i++)
    {
        assert(typecheck_values(&descriptor->arg_list[i], arg_list[i]));
        move_value(value_buffer, fn_builder, &descriptor->arg_list[i], arg_list[i]);
    }

    s64 parameter_stack_size = max((s64)4, arg_count) * 8;

    // @TODO: support this for function that accepts arguments
    s64 return_size = descriptor_size(descriptor->return_value->descriptor);
    if (return_size > 8)
    {
        parameter_stack_size += return_size;
        Descriptor* return_pointer_descriptor = descriptor_pointer_to(descriptor_buffer, descriptor->return_value->descriptor);
        Value* reg_c = reg_value(value_buffer, Register::C, return_pointer_descriptor);
        fn_builder->instruction_buffer.append({ lea, { reg_c->operand, descriptor->return_value->operand } });
    }

    fn_builder->max_call_parameter_stack_size = max(fn_builder->max_call_parameter_stack_size, parameter_stack_size);

    if (fn->operand.type == OperandType::Relative && fn->operand.size == static_cast<u8>(OperandSize::Bits_32))
    {
        fn_builder->instruction_buffer.append({ call, { fn->operand } });
    }
    else
    {
        Value* reg_a = reg_value(value_buffer, Register::A, fn->descriptor);
        move_value(value_buffer, fn_builder, reg_a, fn);
        fn_builder->instruction_buffer.append({ call, { reg_a->operand } });
    }

    if (return_size <= 8)
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
        bool match = true;

        for (s64 arg_index = 0; arg_index < arg_count; arg_index++)
        {
            Value* demanded_arg_value = arg_list[arg_index];
            Value* available_arg_value = &descriptor->arg_list[arg_index];

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
    fn_builder->instruction_buffer.append({ cmp, { condition->operand, imm32(0) } });
    fn_builder->instruction_buffer.append({ jz, { rel(lbl) } });

    return lbl;
}

Label* if_begin(Allocator* allocator, FunctionBuilder* fn_builder, Value* condition)
{
    return make_if(allocator, fn_builder, condition);
}

void if_end(FunctionBuilder* fn_builder, Label* lbl)
{
    fn_builder->instruction_buffer.append({ .label = lbl });
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
    fn_builder->instruction_buffer.append({ .label = start });
    return { .start = start, .end = label(allocator, OperandSize::Bits_32), .done = false };
}

void loop_end(FunctionBuilder* fn_builder, LoopBuilder* loop)
{
    fn_builder->instruction_buffer.append({ jmp, { rel(loop->start)} });
    fn_builder->instruction_buffer.append({ .label = loop->end });
    loop->done = true;
}

void loop_continue(FunctionBuilder* fn_builder, LoopBuilder* loop)
{
    fn_builder->instruction_buffer.append({ jmp, { rel(loop->start) } });
}

void loop_break(FunctionBuilder* fn_builder, LoopBuilder* loop)
{
    fn_builder->instruction_buffer.append({ jmp, { rel(loop->end) } });
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
    struct_builder->field_count++;

    return &field->descriptor;
}

Descriptor* struct_end(Allocator* allocator, DescriptorBuffer* descriptor_buffer, StructBuilder* struct_builder)
{
    assert(struct_builder->field_count);

    Descriptor* result = descriptor_buffer->allocate();
    assert(result);

    DescriptorStructField* field_list = new(allocator)DescriptorStructField[struct_builder->field_count];
    assert(field_list);

    u64 index = struct_builder->field_count - 1;
    for (StructBuilderField* field = struct_builder->field_list; field; field = field->next, index--)
    {
        field_list[index] = field->descriptor;
    }

    result->type = DescriptorType::Struct;
    result->struct_ = {
        .field_list = field_list,
        .field_count = struct_builder->field_count,
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

    for (s64 i = 0; i < descriptor->struct_.field_count; ++i)
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

    fn_builder->instruction_buffer.append({ cmp, { reg_a->operand, temp_b->operand } });

    // @TODO: use xor
    reg_a = reg_value(value_buffer, Register::A, &descriptor_s64);
    move_value(value_buffer, fn_builder, reg_a, s64_value(value_buffer, 0));

    switch (compare_op)
    {
        case CompareOp::Equal:
            fn_builder->instruction_buffer.append({ setz, { reg.al }});
            break;
        case CompareOp::Less:
            fn_builder->instruction_buffer.append({ setl, { reg.al }});
            break;
        case CompareOp::Greater:
            fn_builder->instruction_buffer.append({ setg, { reg.al } });
            break;
        default:
            RNS_NOT_IMPLEMENTED;
            break;
    }

    Value* result = stack_reserve(value_buffer, fn_builder, (Descriptor*)&descriptor_s64);
    move_value(value_buffer, fn_builder, result, reg_a);

    return (result);
}

#if 0 // Function macro substitute
Value *_id_ = 0; \
for (\
  Function_Builder builder_ = fn_begin(&_id_, program_);\
  !fn_is_frozen(&builder_);\
  fn_end(&builder_)\
)
#endif
#if 0
#define Arg(id, descriptor)\
Value* id = fn_arg(&builder, descriptor);
#endif



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

    fn_builder->instruction_buffer.append({ arithmetic_instruction, {reg_a->operand, temp_b->operand }} );

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

    fn_builder->instruction_buffer.append({ imul, { reg_a->operand, b_temp->operand } });

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
            fn_builder->instruction_buffer.append({ cwd });
            break;
        case (s64)OperandSize::Bits_32:
            fn_builder->instruction_buffer.append({ cdq });
            break;
        case (s64)OperandSize::Bits_64:
            fn_builder->instruction_buffer.append({ cqo });
            break;
        default:
            RNS_NOT_IMPLEMENTED;
    }

    fn_builder->instruction_buffer.append({ idiv, {divisor->operand}});

    Value* temporary_value = stack_reserve(value_buffer, fn_builder, a->descriptor);
    move_value(value_buffer, fn_builder, temporary_value, reg_a);

    // Restore RDX
    move_value(value_buffer, fn_builder, reg_rdx, rdx_temp);

    return (temporary_value);
}

TestEncodingFn(test_basic_conditional)
{
    FunctionBuilder fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    Value* arg_value = fn_arg(value_buffer, &fn_builder, &descriptor_s32);
    Value* if_condition = compare(value_buffer, &fn_builder, CompareOp::Equal, arg_value, s32_value(value_buffer, 0));
    Label* conditional = if_begin(general_allocator, &fn_builder, if_condition);
    fn_return(value_buffer, &fn_builder, s32_value(value_buffer, 0));
    if_end(&fn_builder, conditional);
    fn_return(value_buffer, &fn_builder, s32_value(value_buffer, 1));
    fn_end(&fn_builder);

    auto* fn = value_as_function(fn_builder.value, Ret_S32_Args_s32);
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
    FunctionBuilder id_fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    Value* arg_value = fn_arg(value_buffer, &id_fn_builder, &descriptor_s64);
    fn_return(value_buffer, &id_fn_builder, arg_value);
    fn_end(&id_fn_builder);

    FunctionBuilder partial_fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    Value* call_args[] = { s64_value(value_buffer, test_n) };
    Value* call_value = call_function_value(descriptor_buffer, value_buffer, &partial_fn_builder, id_fn_builder.value, call_args, rns_array_length(call_args));
    fn_return(value_buffer, &partial_fn_builder, call_value);
    fn_end(&partial_fn_builder);

    auto* value = value_as_function(partial_fn_builder.value, RetS64ParamVoid);
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

    Value* puts_value = C_function_value(descriptor_buffer, value_buffer, "int puts(const char*)", (u64)puts);
    assert(puts_value);

    FunctionBuilder fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);

    (void)call_function_value(descriptor_buffer, value_buffer, &fn_builder, puts_value, args, rns_array_length(args));

    fn_return(value_buffer, &fn_builder, &void_value);
    fn_end(&fn_builder);

    value_as_function(fn_builder.value, VoidRetVoid)();
    printf("Should see a hello world\n");

    return true;
}

TestEncodingFn(test_rns_add_sub)
{
    s64 a = 15123;
    s64 b = 6;
    s64 sub = 4;

    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    Value* a_value = fn_arg(value_buffer, &fn_builder, &descriptor_s64);
    Value* b_value = fn_arg(value_buffer, &fn_builder, &descriptor_s64);

    auto* return_result = rns_add(value_buffer, &fn_builder, rns_sub(value_buffer, &fn_builder, a_value, s64_value(value_buffer, sub)), b_value);
    fn_return(value_buffer, &fn_builder, return_result);
    fn_end(&fn_builder);

    s64 result = value_as_function(fn_builder.value , RetS64_ParamS64_S64)(a, b);

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

    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    Value* a_value = fn_arg(value_buffer, &fn_builder, &descriptor_s32);

    auto* return_result = rns_multiply_signed(value_buffer, &fn_builder, a_value, s32_value(value_buffer, b));
    fn_return(value_buffer, &fn_builder, return_result);
    fn_end(&fn_builder);

    auto result = value_as_function(fn_builder.value , RetS32_ParamS32_S32)(a, b);
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
    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    Value* a_value = fn_arg(value_buffer, &fn_builder, &descriptor_s32);
    Value* b_value = fn_arg(value_buffer, &fn_builder, &descriptor_s32);

    auto* return_result = rns_signed_div(value_buffer, &fn_builder, a_value, b_value);
    fn_return(value_buffer, &fn_builder, return_result);
    fn_end(&fn_builder);

    auto result = value_as_function(fn_builder.value, RetS32_ParamS32_S32)(a, b);
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

    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    {
        auto* arr_arg = fn_arg(value_buffer, &fn_builder, &array_pointer_descriptor);
        auto* index = Stack(value_buffer, &fn_builder, &descriptor_s32, s32_value(value_buffer, 0));
        auto* temp = Stack(value_buffer, &fn_builder, &array_pointer_descriptor, arr_arg);

        u32 element_size = static_cast<u32>(descriptor_size(array_pointer_descriptor.pointer_to->fixed_size_array.data));
        {
            auto loop = loop_start(general_allocator, &fn_builder);
            auto length = array_pointer_descriptor.pointer_to->fixed_size_array.len;

            // if (i > array_length)
            {
                auto* condition = compare(value_buffer, &fn_builder, CompareOp::Greater, index, s32_value(value_buffer, static_cast<s32>(length-1)));
                auto* if_label = if_begin(general_allocator, &fn_builder, condition);
                loop_break(&fn_builder, &loop);
                if_end(&fn_builder, if_label);
            }

            Value* reg_a = reg_value(value_buffer, Register::A, temp->descriptor);
            move_value(value_buffer, &fn_builder, reg_a, temp);

            Operand pointer = {
                .type = OperandType::MemoryIndirect,
                .size = element_size,
                .indirect = {
                    .displacement = 0,
                    .reg = reg.rax.reg,
                },
            };

            fn_builder.instruction_buffer.append({ inc, { pointer } });
            fn_builder.instruction_buffer.append({ add, { temp->operand, imm32(element_size) } });
            fn_builder.instruction_buffer.append({ inc, {index->operand} });
            loop_end(&fn_builder, &loop);
        }
        fn_end(&fn_builder);
    }

    value_as_function(fn_builder.value, RetVoid_Param_P_S32)(arr);

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
    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    {
        auto* size_struct = fn_arg(value_buffer, &fn_builder, size_struct_pointer_desc);
        auto* width = struct_get_field(general_allocator, size_struct, "width");
        auto* height = struct_get_field(general_allocator, size_struct, "height");
        auto* mul_result = rns_multiply_signed(value_buffer, &fn_builder, width, height);
        fn_return(value_buffer, &fn_builder, mul_result);
        fn_end(&fn_builder);
    }

    struct { s32 width; s32 height; s32 dummy; } size = { 10, 42 };
    auto result = value_as_function(fn_builder.value, RetS32_Param_VoidP)(&size);
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
    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    auto* arg0 = fn_arg(value_buffer, &fn_builder, &descriptor_s64);
    auto* arg1 = fn_arg(value_buffer, &fn_builder, &descriptor_s64);
    auto* arg2 = fn_arg(value_buffer, &fn_builder, &descriptor_s64);
    auto* arg3 = fn_arg(value_buffer, &fn_builder, &descriptor_s64);
    auto* arg4 = fn_arg(value_buffer, &fn_builder, &descriptor_s64);
    auto* arg5 = fn_arg(value_buffer, &fn_builder, &descriptor_s64);
    auto* arg6 = fn_arg(value_buffer, &fn_builder, &descriptor_s64);
    auto* arg7 = fn_arg(value_buffer, &fn_builder, &descriptor_s64);

    fn_return(value_buffer, &fn_builder, arg5);
    fn_end(&fn_builder);

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
    s64 result = value_as_function(fn_builder.value, RetS64_ALotOfS64Args)(v0, v1, v2, v3, v4, v5, v6, v7); return (result == v5);
}

TestEncodingFn(test_type_checker)
{
    bool result = typecheck(&descriptor_s32, &descriptor_s32);
    result = result && !typecheck(&descriptor_s32, &descriptor_s16);
    result = result && !typecheck(&descriptor_s64, descriptor_pointer_to(descriptor_buffer, &descriptor_s64));
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

    auto fn_builder_a = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    auto* a_arg0 = fn_arg(value_buffer, &fn_builder_a, &descriptor_s32);
    fn_end(&fn_builder_a);
    auto fn_builder_b = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    auto* b_arg0 = fn_arg(value_buffer, &fn_builder_b, &descriptor_s32);
    fn_end(&fn_builder_b);
    auto fn_builder_c = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    auto* c_arg0 = fn_arg(value_buffer, &fn_builder_c, &descriptor_s32);
    auto* c_arg1 = fn_arg(value_buffer, &fn_builder_c, &descriptor_s32);
    fn_end(&fn_builder_c);
    auto fn_builder_d = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    auto* d_arg0 = fn_arg(value_buffer, &fn_builder_d, &descriptor_s64);
    fn_end(&fn_builder_d);
    auto fn_builder_e = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    auto* e_arg0 = fn_arg(value_buffer, &fn_builder_e, &descriptor_s32);
    fn_return(value_buffer, &fn_builder_e, s32_value(value_buffer, 0));
    fn_end(&fn_builder_e);

    result = result && typecheck_values(fn_builder_a.value, fn_builder_b.value);
    result = result && !typecheck_values(fn_builder_a.value, fn_builder_c.value);
    result = result && !typecheck_values(fn_builder_a.value, fn_builder_d.value);
    result = result && !typecheck_values(fn_builder_d.value, fn_builder_c.value);
    result = result && !typecheck_values(fn_builder_d.value, fn_builder_e.value);
    return result;
}

TestEncodingFn(test_fibonacci)
{
    auto fib_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    {
        auto* n = fn_arg(value_buffer, &fib_builder, &descriptor_s64);
        {
            auto* condition = compare(value_buffer, &fib_builder, CompareOp::Equal, n, s64_value(value_buffer, 0));
            auto* if_lbl = if_begin(general_allocator, &fib_builder, condition);
            fn_return(value_buffer, &fib_builder, s64_value(value_buffer, 0));
            if_end(&fib_builder, if_lbl);
        }
        {
            auto* condition = compare(value_buffer, &fib_builder, CompareOp::Equal, n, s64_value(value_buffer, 1));
            auto* if_lbl = if_begin(general_allocator, &fib_builder, condition);
            fn_return(value_buffer, &fib_builder, s64_value(value_buffer, 1));
            if_end(&fib_builder, if_lbl);
        }

        auto* n_minus_one = rns_sub(value_buffer, &fib_builder, n, s64_value(value_buffer, 1));
        auto* n_minus_two = rns_sub(value_buffer, &fib_builder, n, s64_value(value_buffer, 2));

        auto* f_minus_one = call_function_value(descriptor_buffer, value_buffer, &fib_builder, fib_builder.value, &n_minus_one, 1);
        auto* f_minus_two = call_function_value(descriptor_buffer, value_buffer, &fib_builder, fib_builder.value, &n_minus_two, 1);

        auto* result = rns_add(value_buffer, &fib_builder, f_minus_one, f_minus_two);
        fn_return(value_buffer, &fib_builder, result);
        fn_end(&fib_builder);
    }

    auto* fib = value_as_function(fib_builder.value, RetS64_ParamS64);
    bool result = true;
    result = result && fib(0) == 0;
    result = result && fib(1) == 1;
    result = result && fib(2) == 1;
    result = result && fib(3) == 2;
    result = result && fib(6) == 8;

    return result;
}

Value* make_identity(Allocator* general_allocator, ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, ExecutionBuffer* execution_buffer, Descriptor* descriptor)
{
    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    auto* arg = fn_arg(value_buffer, &fn_builder, descriptor);
    fn_return(value_buffer, &fn_builder, arg);
    fn_end(&fn_builder);

    return fn_builder.value;
}
Value* make_add_two(Allocator* general_allocator, ValueBuffer* value_buffer, DescriptorBuffer* descriptor_buffer, ExecutionBuffer* execution_buffer, Descriptor* descriptor)
{
    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    auto* arg = fn_arg(value_buffer, &fn_builder, descriptor);
    auto* result = rns_add(value_buffer, &fn_builder, arg, s64_value(value_buffer, 2));
    fn_return(value_buffer, &fn_builder, result);
    fn_end(&fn_builder);

    return fn_builder.value;
}

TestEncodingFn(test_basic_parametric_polymorphism)
{
    Value* id_s64 = make_identity(general_allocator, value_buffer, descriptor_buffer, execution_buffer, &descriptor_s64);
    Value* id_s32 = make_identity(general_allocator, value_buffer, descriptor_buffer, execution_buffer, &descriptor_s32);
    Value* add_two_s64 = make_add_two(general_allocator, value_buffer, descriptor_buffer, execution_buffer, &descriptor_s64);

    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    {
        Value* value_s64 = s64_value(value_buffer, 0);
        Value* value_s32 = s32_value(value_buffer, 0);
        call_function_value(descriptor_buffer, value_buffer, &fn_builder, id_s64, &value_s64, 1);
        call_function_value(descriptor_buffer, value_buffer, &fn_builder, id_s32, &value_s32, 1);
        call_function_value(descriptor_buffer, value_buffer, &fn_builder, add_two_s64, &value_s64, 1);
        fn_end(&fn_builder);
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

    auto fn_builder = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);

    Value* field_count = fn_reflect(value_buffer, &fn_builder, point_struct_descriptor);
    auto* struct_ = Stack(value_buffer, &fn_builder, &descriptor_struct_reflection, field_count);
    auto* field = struct_get_field(general_allocator, struct_, "field_count");
    fn_return(value_buffer, &fn_builder, field);
    fn_end(&fn_builder);
    s32 count = value_as_function(fn_builder.value, RetS32ParamVoid)();
    bool fn_result = count == 2;
    return fn_result;
}

// @AdhocPolymorphism
TestEncodingFn(test_adhoc_polymorphism)
{
    auto sizeof_s32 = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    {
        auto* s32_arg = fn_arg(value_buffer, &sizeof_s32, &descriptor_s32);
        fn_return(value_buffer, &sizeof_s32, s64_value(value_buffer, sizeof(s32)));
        fn_end(&sizeof_s32);
    }
    auto sizeof_s64 = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    {
        auto* s64_arg = fn_arg(value_buffer, &sizeof_s64, &descriptor_s64);
        fn_return(value_buffer, &sizeof_s64, s64_value(value_buffer, sizeof(s64)));
        fn_end(&sizeof_s64);
    }

    sizeof_s32.descriptor->function.next_overload = sizeof_s64.value;

    auto checker_fn = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    {
        auto* zero_value_1 = s64_value(value_buffer, 0);
        auto* zero_value_2 = s32_value(value_buffer, 0);

        auto* x = call_function_value(descriptor_buffer, value_buffer, &checker_fn, sizeof_s32.value, &zero_value_1, 1);
        auto* y = call_function_value(descriptor_buffer, value_buffer, &checker_fn, sizeof_s32.value, &zero_value_2, 1);

        auto* addition = rns_add(value_buffer, &checker_fn, x, y);
        fn_return(value_buffer, &checker_fn, addition);
        fn_end(&checker_fn);
    }

    auto* sizeofs64_fn = value_as_function(sizeof_s64.value, RetS64ParamVoid);
    auto size_result = sizeofs64_fn();
    bool fn_result = size_result == sizeof(s64);
    if (!fn_result)
    {
        return false;
    }

    auto* checker_function = value_as_function(checker_fn.value, RetS64ParamVoid);
    auto checker_result = checker_function();
    fn_result = checker_result == sizeof(s64) + sizeof(s32);
    return fn_result;
}

TestEncodingFn(test_rip_addressing_mode)
{
    s32 a = 32, b = 10;
    // @TODO: consider refactoring (abstracting) this in a more robust way (don't hardcode)
    GlobalBuffer global_buffer = GlobalBuffer::create(general_allocator, 1024);
    Value* global_a = global_value(&global_buffer, value_buffer, &descriptor_s32);
    {
        if (global_a->operand.type != OperandType::RIP_relative)
        {
            return false;
        }

        s32* address = reinterpret_cast<s32*>(global_a->operand.imm64);
        *address = a;
    }
    Value* global_b = global_value(&global_buffer, value_buffer, &descriptor_s32);
    {
        if (global_b->operand.type != OperandType::RIP_relative)
        {
            return false;
        }

        s32* address = reinterpret_cast<s32*>(global_b->operand.imm64);
        *address = b;
    }

    auto checker_fn = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    {
        auto* addition = rns_add(value_buffer, &checker_fn, global_a, global_b);
        fn_return(value_buffer, &checker_fn, addition);
        fn_end(&checker_fn);
    }

    auto* checker = value_as_function(checker_fn.value, RetS32ParamVoid);
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
        {.name = "None", .field_list = nullptr, .field_count = 0, },
        {.name = "Some", .field_list = some_fields, .field_count = rns_array_length(some_fields), },
    };

    Descriptor option_s64_descriptor = {
        .type = DescriptorType::TaggedUnion,
        .tagged_union = {
            .struct_list = constructors,
            .struct_count = rns_array_length(constructors),
        }
    };

    auto with_default_value = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    auto* option_value = fn_arg(value_buffer, &with_default_value, descriptor_pointer_to(descriptor_buffer, &option_s64_descriptor));
    auto* default_value = fn_arg(value_buffer, &with_default_value, &descriptor_s64);

    {
        Value* some = cast_to_tag(general_allocator, descriptor_buffer, value_buffer, &with_default_value, "Some", option_value);
        auto* some_if_block = if_begin(general_allocator, &with_default_value, some);
        auto* value = struct_get_field(general_allocator, some, "value");
        fn_return(value_buffer, &with_default_value, value);
        if_end(&with_default_value, some_if_block);
    }

    fn_return(value_buffer, &with_default_value, default_value);
    fn_end(&with_default_value);

    auto* with_default = value_as_function(with_default_value.value, RetS64ParamVoidStar_s64);
    struct { s64 tag; s64 maybe_value; } test_none = {0};
    struct { s64 tag; s64 maybe_value; } test_some = {1, 21};
    bool result = with_default(&test_none, 42) == 42;
    result = result && with_default(&test_some, 42) == 21;

    return result;
}


TestEncodingFn(test_hello_world_lea)
{
    Value* puts_value = C_function_value(descriptor_buffer, value_buffer, "int puts(const char*)", (u64)puts);
    
    Descriptor message_descriptor = {
        .type = DescriptorType::FixedSizeArray,
        .fixed_size_array = {
            .data = &descriptor_s8,
            .len = 4,
         }
    };

    u8 hi[] = { 'H', 'i', '!', 0 };
    s32 hi_s32 = *(s32*)hi;
    auto hello_world = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    Value* message_value = stack_reserve(value_buffer, &hello_world, &message_descriptor);
    move_value(value_buffer, &hello_world, message_value, s32_value(value_buffer, hi_s32));
    auto* message_pointer_value = pointer_value(value_buffer, descriptor_buffer, &hello_world, message_value);
    call_function_value(descriptor_buffer, value_buffer, &hello_world, puts_value, &message_pointer_value, 1);
    fn_end(&hello_world);
    printf("It should print hello world\n");
    value_as_function(hello_world.value, RetVoidParamVoid)();

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
        .operand = {
            .type = OperandType::MemoryIndirect,
            .indirect = {
                .displacement = 0,
                .reg = reg.rsp.reg,
            }
        }
    };

    Descriptor* c_test_fn_descriptor = descriptor_buffer->allocate();
    *c_test_fn_descriptor = 
    {
        .type = DescriptorType::Function,
        .function = {
            .arg_count = 0,
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

    auto checker_fn = fn_begin(general_allocator, value_buffer, descriptor_buffer, execution_buffer);
    Value* test_result = call_function_value(descriptor_buffer, value_buffer, &checker_fn, c_test_fn_value, NULL, 0);
    Value* x = struct_get_field(general_allocator, test_result, "x");
    fn_return(value_buffer, &checker_fn, x);
    fn_end(&checker_fn);

    RetS64ParamVoid* checker_value_fn = value_as_function(checker_fn.value, RetS64ParamVoid);
    s64 result = checker_value_fn();
    bool fn_result = result == 42;

    return fn_result;
}

struct TestFunctionArgs
{
    Allocator* general_allocator;
    ValueBuffer* value_buffer;
    DescriptorBuffer* descriptor_buffer;
    ExecutionBuffer* execution_buffer;
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
            auto& test_case = test_cases[i];
            auto result = test_case.fn(args.general_allocator, args.value_buffer, args.descriptor_buffer, args.execution_buffer);
            const char* test_result_msg = result ? "OK" : "FAILED";
            passed_test_case_count += result;
            printf("%s [%s]\n", test_case.name, test_result_msg);
        }
        auto tests_run = i + bool(test_case_count != i);
        printf("Tests passed: %u. Tests failed: %u. Tests run: %u. Total tests: %u\n", passed_test_case_count, tests_run - passed_test_case_count, tests_run, test_case_count);

        return passed_test_case_count == test_case_count;
    }
};

void wna_main(s32 argc, char* argv[])
{
    DebugAllocator test_allocator;

    s64 test_allocator_size = 1024*1024*1024;

    void* address = RNS::virtual_alloc(nullptr, test_allocator_size, { .commit = 1, .reserve = 1, .execute = 0, .read = 1, .write = 1 });
    test_allocator.pool = { (u8*)address, 0, test_allocator_size };

    s64 value_count = 10000;
    s64 descriptor_count = 1000;
    ValueBuffer value_buffer = ValueBuffer::create(&test_allocator, value_count);
    DescriptorBuffer descriptor_buffer = DescriptorBuffer::create(&test_allocator, descriptor_count);
    DebugAllocator execution_allocator = DebugAllocator::create(virtual_alloc(nullptr, 1024*1024, { .commit = 1, .reserve = 1, .execute = 1, .read = 1, .write = 1 }), 1024*1024);
    auto execution_buffer = ExecutionBuffer::create(&execution_allocator, execution_allocator.pool.cap);
#if TEST_ENCODING
    test_main(&test_allocator, &value_buffer, &descriptor_buffer);
#else
    TestFunctionArgs args = {
        .general_allocator = &test_allocator,
        .value_buffer = &value_buffer,
        .descriptor_buffer = &descriptor_buffer,
        .execution_buffer = &execution_buffer
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
    };

    auto test_suite = TestSuite::create(args, test_cases, rns_array_length(test_cases));
    s32 exit_code = test_suite.run() ? 0 : -1;
#endif
}

