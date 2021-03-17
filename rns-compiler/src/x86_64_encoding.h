#pragma once

#define OP(_type, _size) { .type = OperandEncodingType:: ## _type,  .size = static_cast<u8>(OperandSize::Bits_ ## _size), }
#define OP_CODE(...) .op_code = { __VA_ARGS__ }
#define OPS(...) .operand_encodings = { __VA_ARGS__ }
#define OP_COMB(...) { __VA_ARGS__ }
#define COMBINATIONS(...) .operand_combinations = { __VA_ARGS__}
#define ENC_OPTS(_type_, ...) .options = { .type = InstructionOptionType:: ## _type_, __VA_ARGS__ }
#define ENCODING(...) { __VA_ARGS__ }

const InstructionEncoding adc_encoding[] =
{
    ENCODING(OP_CODE(0x14), COMBINATIONS(
        OP_COMB(OPS(OP(Register_A, 8), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x15), COMBINATIONS(
        OP_COMB(OPS(OP(Register_A, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(Register_A, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(Register_A, 64), OP(Immediate, 32))),
        )),
    ENCODING(OP_CODE(0x80), ENC_OPTS(Digit, .digit = 2), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::Rex, OPS(OP(RegisterOrMemory, 8), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x81), ENC_OPTS(Digit, .digit = 2), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Immediate, 32))),
        )),
    ENCODING(OP_CODE(0x83), ENC_OPTS(Digit, .digit = 2), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 8))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Immediate, 8))),
        )),
    ENCODING(OP_CODE(0x10), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
        )),
    ENCODING(OP_CODE(0x11),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Register, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Register, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Register, 64))),
        )),
    ENCODING(OP_CODE(0x12),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
        )),
    ENCODING(OP_CODE(0x13),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(Register, 64), OP(RegisterOrMemory, 64))),
        )),
};

const InstructionEncoding adcx_encoding[] = {0};
const InstructionEncoding add_encoding[] =
{
    ENCODING(OP_CODE(0x04), COMBINATIONS(
        OP_COMB(OPS(OP(Register_A, 8), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x05), COMBINATIONS(
        OP_COMB(OPS(OP(Register_A, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(Register_A, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(Register_A, 64), OP(Immediate, 32))),
    )),
    ENCODING(OP_CODE(0x80),ENC_OPTS(Digit, .digit = 0), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory,8), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(RegisterOrMemory,8), OP(Immediate, 8))),
        )),
    ENCODING(OP_CODE(0x81),ENC_OPTS(Digit, .digit = 0), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Immediate, 32))),
    )),
    ENCODING(OP_CODE(0x83),ENC_OPTS(Digit, .digit = 0), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 8))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x00),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
    )),
    ENCODING(OP_CODE(0x01),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Register, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Register, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Register, 64))),
    )),
    ENCODING(OP_CODE(0x02),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0x03),ENC_OPTS(Reg), COMBINATIONS(
    	OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 16))),
    	OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 32))),
    	OP_COMB(.rex_byte = Rex::W,   OPS(OP(Register, 64), OP(RegisterOrMemory, 64))),
    )),
};

const InstructionEncoding adox_encoding[] = { 0 };
const InstructionEncoding and_encoding[] = { 0 };
const InstructionEncoding andn_encoding[] = { 0 };
const InstructionEncoding bextr_encoding[] = { 0 };
const InstructionEncoding blsi_encoding[] = { 0 };
const InstructionEncoding blsmsk_encoding[] = { 0 };
const InstructionEncoding blsr_encoding[] = { 0 };
const InstructionEncoding bndcl_encoding[] = { 0 };
const InstructionEncoding bndcu_encoding[] = { 0 };
const InstructionEncoding bndcn_encoding[] = { 0 };
const InstructionEncoding bndldx_encoding[] = { 0 };
const InstructionEncoding bndmk_encoding[] = { 0 };
const InstructionEncoding bndmov_encoding[] = { 0 };
const InstructionEncoding bndstx_encoding[] = { 0 };
const InstructionEncoding bsf_encoding[] = { 0 };
const InstructionEncoding bsr_encoding[] = { 0 };
const InstructionEncoding bswap_encoding[] = { 0 };
const InstructionEncoding bt_encoding[] = { 0 };
const InstructionEncoding btc_encoding[] = { 0 };
const InstructionEncoding btr_encoding[] = { 0 };
const InstructionEncoding bts_encoding[] = { 0 };
const InstructionEncoding bzhi_encoding[] = { 0 };
const InstructionEncoding call_encoding[] =
{
    ENCODING(OP_CODE(0xE8),  COMBINATIONS(
    	OP_COMB(OPS(OP(Relative, 16))),
    	OP_COMB(OPS(OP(Relative, 32))),
    )),
    ENCODING(OP_CODE(0xFF), ENC_OPTS(Digit, .digit = 2), COMBINATIONS(
    	OP_COMB(OPS(OP(RegisterOrMemory, 64))),
    )),
};
const InstructionEncoding cbw_encoding[] = { 0 };
const InstructionEncoding cwde_encoding[] = { 0 };
const InstructionEncoding cdqe_encoding[] = { 0 };
const InstructionEncoding clac_encoding[] = { 0 };
const InstructionEncoding clc_encoding[] = { 0 };
const InstructionEncoding cld_encoding[] = { 0 };
const InstructionEncoding cldemote_encoding[] = { 0 };
const InstructionEncoding clflush_encoding[] = { 0 };
const InstructionEncoding clflushopt_encoding[] = { 0 };
const InstructionEncoding cli_encoding[] = { 0 };
const InstructionEncoding clrssbsy_encoding[] = { 0 };
const InstructionEncoding clts_encoding[] = { 0 };
const InstructionEncoding clwb_encoding[] = { 0 };
const InstructionEncoding cmc_encoding[] = { 0 };
const InstructionEncoding cmov_encoding[] = { 0 };
const InstructionEncoding cmp_encoding[] =
{
    ENCODING(OP_CODE(0x3C),  COMBINATIONS(
    	OP_COMB(OPS(OP(Register_A, 8), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x3D),  COMBINATIONS(
    	OP_COMB(OPS(OP(Register_A, 16), OP(Immediate, 16))),
    	OP_COMB(OPS(OP(Register_A, 32), OP(Immediate, 32))),
    	OP_COMB(.rex_byte = Rex::W, OPS(OP(Register_A, 64), OP(Immediate, 32))),
    )),
    ENCODING(OP_CODE(0x80), ENC_OPTS(Digit, .digit = 7), COMBINATIONS(
    	OP_COMB(OPS(OP(RegisterOrMemory, 8), OP(Immediate, 8))),
    	OP_COMB(.rex_byte = Rex::Rex, OPS(OP(RegisterOrMemory, 8), OP(Immediate, 8))),
        )),
    ENCODING(OP_CODE(0x81), ENC_OPTS(Digit, .digit = 7), COMBINATIONS(
    	OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 16))),
    	OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 32))),
    	OP_COMB(.rex_byte = Rex::W, OPS(OP(RegisterOrMemory, 64), OP(Immediate, 32))),
        )),
    ENCODING(OP_CODE(0x83), ENC_OPTS(Digit, .digit = 7), COMBINATIONS(
    	OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 8))),
    	OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 8))),
    	OP_COMB(.rex_byte = Rex::W, OPS(OP(RegisterOrMemory, 64), OP(Immediate, 8))),
        )),
    ENCODING(OP_CODE(0x38), ENC_OPTS(Reg), COMBINATIONS(
    	OP_COMB(OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
    	OP_COMB(.rex_byte = Rex::Rex, OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
        )),
    ENCODING(OP_CODE(0x39), ENC_OPTS(Reg), COMBINATIONS(
    	OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Register, 16))),
    	OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Register, 32))),
    	OP_COMB(.rex_byte = Rex::W, OPS(OP(RegisterOrMemory, 64), OP(Register, 64))),
        )),
    ENCODING(OP_CODE(0x3A), ENC_OPTS(Reg), COMBINATIONS(
    	OP_COMB(OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
    	OP_COMB(.rex_byte = Rex::Rex, OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
        )),
    ENCODING(OP_CODE(0x3B), ENC_OPTS(Reg), COMBINATIONS(
    	OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 16))),
    	OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 32))),
    	OP_COMB(.rex_byte = Rex::W, OPS(OP(Register, 64), OP(RegisterOrMemory, 64))),
        )),
};

const InstructionEncoding cmpxchg_encoding[] = { 0 };
const InstructionEncoding cmpxchg8b_encoding[] = { 0 };
const InstructionEncoding cmpxchg16b_encoding[] = { 0 };
const InstructionEncoding cpuid_encoding[] = { 0 };
const InstructionEncoding crc32_encoding[] = { 0 };

const InstructionEncoding cwd_encoding[] =
{
    ENCODING(OP_CODE(0x99),ENC_OPTS(None, .explicit_byte_size = static_cast<u8>(OperandSize::Bits_16)),
        COMBINATIONS({ {} } ) ),
};
const InstructionEncoding cdq_encoding[] =
{
    ENCODING(OP_CODE(0x99),ENC_OPTS(None, .explicit_byte_size = static_cast<u8>(OperandSize::Bits_32)),
        COMBINATIONS({ {} } ) ),
};
const InstructionEncoding cqo_encoding[] =
{
    ENCODING(OP_CODE(0x99),ENC_OPTS(None, .explicit_byte_size = static_cast<u8>(OperandSize::Bits_64)),
        COMBINATIONS({ {} } ) ),
};

const InstructionEncoding dec_encoding[] = { 0 };

// unsigned division
const InstructionEncoding div__encoding[] =
{
    ENCODING(OP_CODE(0xF6),ENC_OPTS(Digit, .digit = 6), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0xF7),ENC_OPTS(Digit, .digit = 6), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(RegisterOrMemory, 64))),
    )),
};

const InstructionEncoding endbr32_encoding[] = { 0 };
const InstructionEncoding endbr64_encoding[] = { 0 };
const InstructionEncoding enter_encoding[] = { 0 };
// Tons of float instructions here
const InstructionEncoding hlt_encoding[] = { 0 };

// signed division
const InstructionEncoding idiv_encoding[] =
{
    ENCODING(OP_CODE(0xF6),ENC_OPTS(Digit, .digit = 7), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0xF7),ENC_OPTS(Digit, .digit = 7), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(RegisterOrMemory, 64))),
    )),
};

// Signed multiply
const InstructionEncoding imul_encoding[] =
{
    ENCODING(OP_CODE(0xF6),ENC_OPTS(Digit, .digit = 5), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0xF7),ENC_OPTS(Digit, .digit = 5), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(RegisterOrMemory, 64))),
    )),
    ENCODING(OP_CODE(0x0F, 0xAF), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(RegisterOrMemory, 64))),
    )),
    ENCODING(OP_CODE(0x6B), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 16), OP(Immediate, 8))),
        OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 32), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(RegisterOrMemory, 64), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x69), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(RegisterOrMemory, 64), OP(Immediate, 32))),
    )),
};

const InstructionEncoding in_encoding[] = { 0 };

const InstructionEncoding inc_encoding[] =
{
    ENCODING(OP_CODE(0xFE), ENC_OPTS(Digit, .digit = 0), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,     OPS(OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0xFF), ENC_OPTS(Digit, .digit = 0), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(RegisterOrMemory, 64))),
    )),
    ENCODING(OP_CODE(0x40), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16))),
        OP_COMB(OPS(OP(Register, 32))),
    )),
};

const InstructionEncoding incssp_encoding[] = { 0 };
const InstructionEncoding ins_encoding[] = { 0 };
const InstructionEncoding int3_encoding[] =
{
    ENCODING(OP_CODE(0xCC), COMBINATIONS(
        OP_COMB(),
    )),
};
const InstructionEncoding int_encoding[] = { 0 };
const InstructionEncoding invd_encoding[] = { 0 };
const InstructionEncoding invlpg_encoding[] = { 0 };
const InstructionEncoding invpcid_encoding[] = { 0 };
const InstructionEncoding iret_encoding[] = { 0 };

const InstructionEncoding ja_encoding[] =
{
    ENCODING(OP_CODE(0x77),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x87),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jae_encoding[] =
{
    ENCODING(OP_CODE(0x73),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x83),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jb_encoding[] =
{
    ENCODING(OP_CODE(0x72),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x82),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jbe_encoding[] =
{
    ENCODING(OP_CODE(0x76),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x86),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jc_encoding[] =
{
    ENCODING(OP_CODE(0x72),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x82),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jecxz_encoding[] =
{
    ENCODING(OP_CODE(0xE3),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
};
const InstructionEncoding jrcxz_encoding[] =
{
    ENCODING(OP_CODE(0xE3),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
};
const InstructionEncoding je_encoding[] =
{
    ENCODING(OP_CODE(0x74),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x84),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jg_encoding[] =
{
    ENCODING(OP_CODE(0x7F),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8F),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jge_encoding[] =
{
    ENCODING(OP_CODE(0x7D),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8D),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jl_encoding[] =
{
    ENCODING(OP_CODE(0x7C),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8C),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jle_encoding[] =
{
    ENCODING(OP_CODE(0x7E),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8E),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jna_encoding[] =
{
    ENCODING(OP_CODE(0x76),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x86),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jnae_encoding[] =
{
    ENCODING(OP_CODE(0x72),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x82),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jnb_encoding[] =
{
    ENCODING(OP_CODE(0x73),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x83),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jnbe_encoding[] =
{
    ENCODING(OP_CODE(0x77),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x87),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jnc_encoding[] =
{
    ENCODING(OP_CODE(0x73),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x83),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jne_encoding[] =
{
    ENCODING(OP_CODE(0x75),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x85),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jng_encoding[] =
{
    ENCODING(OP_CODE(0x7E),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8E),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jnge_encoding[] =
{
    ENCODING(OP_CODE(0x7C),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8C),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jnl__encoding[] =
{
    ENCODING(OP_CODE(0x7D),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8D),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jnle_encoding[] =
{
    ENCODING(OP_CODE(0x7F),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8F),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jno_encoding[] =
{
    ENCODING(OP_CODE(0x71),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x81),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jnp_encoding[] =
{
    ENCODING(OP_CODE(0x7B),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8B),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jns_encoding[] =
{
    ENCODING(OP_CODE(0x79),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x89),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jnz_encoding[] =
{
    ENCODING(OP_CODE(0x75),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x85),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jo_encoding[] =
{
    ENCODING(OP_CODE(0x70),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x80),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jp_encoding[] =
{
    ENCODING(OP_CODE(0x7A),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8A),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jpe_encoding[] =
{
    ENCODING(OP_CODE(0x7A),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8A),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jpo_encoding[] =
{
    ENCODING(OP_CODE(0x7B),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x8B),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding js_encoding[] =
{
    ENCODING(OP_CODE(0x78),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x88),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};
const InstructionEncoding jz_encoding[] =
{
    ENCODING(OP_CODE(0x74),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0x84),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
};

const InstructionEncoding jmp_encoding[] =
{
    ENCODING(OP_CODE(0xEB),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 8))),
    )),
    ENCODING(OP_CODE(0xE9),  COMBINATIONS(
        OP_COMB(OPS(OP(Relative, 32))),
    )),
    ENCODING(OP_CODE(0xFF), ENC_OPTS(Digit, .digit = 4), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 64))),
    )),
    // ... Jump far
};

const InstructionEncoding lar_encoding[] = { 0 };
const InstructionEncoding lds_encoding[] = { 0 };
const InstructionEncoding lss_encoding[] = { 0 };
const InstructionEncoding les_encoding[] = { 0 };
const InstructionEncoding lfs_encoding[] = { 0 };
const InstructionEncoding lgs_encoding[] = { 0 };
const InstructionEncoding lea_encoding[] =
{
    ENCODING(OP_CODE(0x8D), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16), OP(Memory, 64))),
        OP_COMB(OPS(OP(Register, 32), OP(Memory, 64))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(Memory, 64))),
    )),

};
const InstructionEncoding leave_encoding[] = { 0 };
const InstructionEncoding lfence_encoding[] = { 0 };
const InstructionEncoding lgdt_encoding[] = { 0 };
const InstructionEncoding lidt_encoding[] = { 0 };
const InstructionEncoding lldt_encoding[] = { 0 };
const InstructionEncoding lmsw_encoding[] = { 0 };
const InstructionEncoding lock_encoding[] = { 0 };
const InstructionEncoding lods_encoding[] = { 0 };
const InstructionEncoding lodsb_encoding[] = { 0 };
const InstructionEncoding lodsw_encoding[] = { 0 };
const InstructionEncoding lodsd_encoding[] = { 0 };
const InstructionEncoding lodsq_encoding[] = { 0 };
const InstructionEncoding loop_encoding[] = { 0 };
const InstructionEncoding loope_encoding[] = { 0 };
const InstructionEncoding loopne_encoding[] = { 0 };
const InstructionEncoding lsl_encoding[] = { 0 };
const InstructionEncoding ltr_encoding[] = { 0 };
const InstructionEncoding lzcnt_encoding[] = { 0 };
const InstructionEncoding mfence_encoding[] = { 0 };

const InstructionEncoding mov_encoding[] =
{
    ENCODING(OP_CODE(0x88), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
    )),
    ENCODING(OP_CODE(0x89), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Register, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Register, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(RegisterOrMemory, 64), OP(Register, 64))),
    )),
    ENCODING(OP_CODE(0x8A), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0x8B), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(RegisterOrMemory, 64))),
    )),
    /*  @TODO: NOT CODED SEGMENT AND OFFSET INSTRUCTIONS */
    ENCODING(OP_CODE(0xB0), ENC_OPTS(OpCodePlusReg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 8), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(Register, 8), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0xB8), ENC_OPTS(OpCodePlusReg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(Register, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(Immediate, 64))),
    )),
    ENCODING(OP_CODE(0xC6), ENC_OPTS(Digit, .digit = 0), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0xC7), ENC_OPTS(Digit, .digit = 0), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(RegisterOrMemory, 64), OP(Immediate, 32))),
    )),
};

const InstructionEncoding movcr_encoding[] = { 0 };
const InstructionEncoding movdbg_encoding[] = { 0 };
const InstructionEncoding movbe_encoding[] = { 0 };
const InstructionEncoding movdq_encoding[] = { 0 };
const InstructionEncoding movdiri_encoding[] = { 0 };
const InstructionEncoding movdir64b_encoding[] = { 0 };
const InstructionEncoding movq_encoding[] = { 0 };
const InstructionEncoding movs_encoding[] = { 0 };

const InstructionEncoding movsx_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0xBE), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 8))),
        OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0x0F, 0xBF), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 16))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(RegisterOrMemory, 16))),
    )),
    ENCODING(OP_CODE(0x63), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(Register, 64), OP(RegisterOrMemory, 32))),
    )),
};

const InstructionEncoding movzx_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0xB6), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 8))),
        OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(RegisterOrMemory, 8))), // This one can be problematic, refer to manual
    )),
    ENCODING(OP_CODE(0x0F, 0xB7), ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 16))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(Register, 64), OP(RegisterOrMemory, 16))),
    )),
};

// Unsigned multiply
const InstructionEncoding mul_encoding[] =
{
    ENCODING(OP_CODE(0xF6), ENC_OPTS(Digit, .digit = 4), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,     OPS(OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0xF7), ENC_OPTS(Digit, .digit = 4), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32))),
        OP_COMB(.rex_byte = Rex::W,     OPS(OP(RegisterOrMemory, 64))),
    )),
};

const InstructionEncoding mulx_encoding[] = { 0 };
const InstructionEncoding mwait_encoding[] = { 0 };
const InstructionEncoding neg_encoding[] = { 0 };
const InstructionEncoding nop_encoding[] = { 0 };
const InstructionEncoding not_encoding[] = { 0 };
const InstructionEncoding or_encoding[] = { 0 };
const InstructionEncoding out_encoding[] = { 0 };
const InstructionEncoding outs_encoding[] = { 0 };
const InstructionEncoding pause_encoding[] = { 0 };
const InstructionEncoding pdep_encoding[] = { 0 };
const InstructionEncoding pext_encoding[] = { 0 };
const InstructionEncoding pop_encoding[] =
{
    ENCODING(OP_CODE(0x58), ENC_OPTS(OpCodePlusReg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16))),
        OP_COMB(OPS(OP(Register, 64))),
    )),
    ENCODING(OP_CODE(0x8f), ENC_OPTS(Digit, .digit = 0), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 64))),
    )),
    ENCODING(OP_CODE(0x6A),  COMBINATIONS(
        OP_COMB(OPS(OP(Immediate, 8))),
    )),
    // @TODO: these need two-byte opcode
    //// Pop FS
    //ENCODING(OP_CODE(0x0f 0xa1,
    //    OP_COMB(OPS(0)),
    //),
    //// Pop GS
    //ENCODING(OP_CODE(0x0f 0xa9,
    //    OP_COMB(OPS(0)),
    //),

};
const InstructionEncoding popcnt_encoding[] = { 0 };
const InstructionEncoding popf_encoding[] = { 0 };
const InstructionEncoding por_encoding[] = { 0 };
const InstructionEncoding prefetch_encoding[] = { 0 };
const InstructionEncoding prefetchw_encoding[] = { 0 };
const InstructionEncoding ptwrite_encoding[] = { 0 };
const InstructionEncoding push_encoding[] =
{
    ENCODING(OP_CODE(0x50), ENC_OPTS(OpCodePlusReg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 16))),
        OP_COMB(OPS(OP(Register, 64))),
    )),
    ENCODING(OP_CODE(0xff), ENC_OPTS(Digit, .digit = 6), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 64))),
    )),
    ENCODING(OP_CODE(0x6A),  COMBINATIONS(
        OP_COMB(OPS(OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x68),  COMBINATIONS(
        OP_COMB(OPS(OP(Immediate, 16))),
        OP_COMB(OPS(OP(Immediate, 32))),
    )),
    // @TODO: these need two-byte opcode
    //// Push FS
    //ENCODING(OP_CODE(0x0f 0xa0,
    //    OP_COMB(OPS(0)),
    //),
    //// Push GS
    //ENCODING(OP_CODE(0x0f 0xa8,
    //    OP_COMB(OPS(0)),
    //),
};
const InstructionEncoding pushf_encoding[] = { 0 };
const InstructionEncoding rotate_encoding[] = { 0 };
const InstructionEncoding rdfsbase_encoding[] = { 0 };
const InstructionEncoding rdgsbase_encoding[] = { 0 };
const InstructionEncoding rdmsr_encoding[] = { 0 };
const InstructionEncoding rdpid_encoding[] = { 0 };
const InstructionEncoding rdpmc_encoding[] = { 0 };
const InstructionEncoding rdrand_encoding[] = { 0 };
const InstructionEncoding rdseed_encoding[] = { 0 };
const InstructionEncoding rdssp_encoding[] = { 0 };
const InstructionEncoding rdtsc_encoding[] = { 0 };
const InstructionEncoding rdtscp_encoding[] = { 0 };
const InstructionEncoding rep_encoding[] = { 0 };
const InstructionEncoding ret_encoding[] =
{
    ENCODING(OP_CODE(0xC3),
        COMBINATIONS({ {} } ) ),
    ENCODING(OP_CODE(0xCB),
        COMBINATIONS({ {} } ) ),
    ENCODING(OP_CODE(0xC2),  COMBINATIONS(
        OP_COMB(OPS(OP(Immediate, 16))),
    )),
    ENCODING(OP_CODE(0xCA),  COMBINATIONS(
        OP_COMB(OPS(OP(Immediate, 16))),
    )),
};

const InstructionEncoding rsm_encoding[] = { 0 };
const InstructionEncoding rstorssp_encoding[] = { 0 };
const InstructionEncoding sahf_encoding[] = { 0 };
const InstructionEncoding sal_encoding[] = { 0 };
const InstructionEncoding sar_encoding[] = { 0 };
const InstructionEncoding shl_encoding[] = { 0 };
const InstructionEncoding shr_encoding[] = { 0 };
const InstructionEncoding sarx_encoding[] = { 0 };
const InstructionEncoding shlx_encoding[] = { 0 };
const InstructionEncoding shrx_encoding[] = { 0 };
const InstructionEncoding saveprevssp_encoding[] = { 0 };
const InstructionEncoding sbb_encoding[] = { 0 };
const InstructionEncoding scas_encoding[] = { 0 };

const InstructionEncoding seta_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x97), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setae_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x93), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setb_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x92), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setbe_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x96), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setc_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x92), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding sete_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x94), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setg_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9F), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setge_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9D), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setl_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9C), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setle_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9E), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setna_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x96), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setnae_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x92), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setnb_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x93), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setnbe_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x97), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setnc_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x97), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setne_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x95), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setng_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9E), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setnge_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9C), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setnl_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9D), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setnle_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9F), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setno_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x91), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setnp_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9B), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setns_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x99), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setnz_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x95), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding seto_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x90), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setp_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9A), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setpe_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9A), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setpo_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x9B), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding sets_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x98), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setz_encoding[] =
{
    ENCODING(OP_CODE(0x0F, 0x94), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,      OPS(OP(RegisterOrMemory, 8))),
    )),
};

const InstructionEncoding setssbsy_encoding[] = { 0 };
const InstructionEncoding sfence_encoding[] = { 0 };
const InstructionEncoding sgdt_encoding[] = { 0 };
const InstructionEncoding shld_encoding[] = { 0 };
const InstructionEncoding shrd_encoding[] = { 0 };
const InstructionEncoding sidt_encoding[] = { 0 };
const InstructionEncoding sldt_encoding[] = { 0 };
const InstructionEncoding smsw_encoding[] = { 0 };
const InstructionEncoding stac_encoding[] = { 0 };
const InstructionEncoding stc_encoding[] = { 0 };
const InstructionEncoding std_encoding[] = { 0 };
const InstructionEncoding sti_encoding[] = { 0 };
const InstructionEncoding stos_encoding[] = { 0 };
const InstructionEncoding str_encoding[] = { 0 };

const InstructionEncoding sub_encoding[] =
{
    ENCODING(OP_CODE(0x2C), COMBINATIONS(
        OP_COMB(OPS(OP(Register_A, 8), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x2D), COMBINATIONS(
        OP_COMB(OPS(OP(Register_A, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(Register_A, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(Register_A, 64), OP(Immediate, 32))),
    )),
    ENCODING(OP_CODE(0x80),ENC_OPTS(Digit, .digit = 5), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory,8), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(RegisterOrMemory,8), OP(Immediate, 8))),
        )),
    ENCODING(OP_CODE(0x81),ENC_OPTS(Digit, .digit = 5), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Immediate, 32))),
    )),
    ENCODING(OP_CODE(0x83),ENC_OPTS(Digit, .digit = 5), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 8))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x28),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
    )),
    ENCODING(OP_CODE(0x29),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Register, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Register, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Register, 64))),
    )),
    ENCODING(OP_CODE(0x2A),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0x2B),ENC_OPTS(Reg), COMBINATIONS(
    	OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 16))),
    	OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 32))),
    	OP_COMB(.rex_byte = Rex::W,   OPS(OP(Register, 64), OP(RegisterOrMemory, 64))),
    )),
};

const InstructionEncoding swapgs_encoding[] = { 0 };
const InstructionEncoding syscall_encoding[] = { 0 };
const InstructionEncoding sysenter_encoding[] = { 0 };
const InstructionEncoding sysexit_encoding[] = { 0 };
const InstructionEncoding sysret_encoding[] = { 0 };
const InstructionEncoding test_encoding[] = { 0 };
const InstructionEncoding tpause_encoding[] = { 0 };
const InstructionEncoding tzcnt_encoding[] = { 0 };
const InstructionEncoding ud_encoding[] = { 0 };
const InstructionEncoding umonitor_encoding[] = { 0 };
const InstructionEncoding umwait_encoding[] = { 0 };
const InstructionEncoding wait_encoding[] = { 0 };
const InstructionEncoding wbinvd_encoding[] = { 0 };
const InstructionEncoding wbnoinvd_encoding[] = { 0 };
const InstructionEncoding wrfsbase_encoding[] = { 0 };
const InstructionEncoding wrgsbase_encoding[] = { 0 };
const InstructionEncoding wrmsr_encoding[] = { 0 };
const InstructionEncoding wrss_encoding[] = { 0 };
const InstructionEncoding wruss_encoding[] = { 0 };
const InstructionEncoding xacquire_encoding[] = { 0 };
const InstructionEncoding xrelease_encoding[] = { 0 };
const InstructionEncoding xabort_encoding[] = { 0 };
const InstructionEncoding xadd_encoding[] = { 0 };
const InstructionEncoding xbegin_encoding[] = { 0 };
const InstructionEncoding xchg_encoding[] = { 0 };
const InstructionEncoding xend_encoding[] = { 0 };
const InstructionEncoding xgetbv_encoding[] = { 0 };
const InstructionEncoding xlat_encoding[] = { 0 };

const InstructionEncoding xor__encoding[] =
{
    ENCODING(OP_CODE(0x34), COMBINATIONS(
        OP_COMB(OPS(OP(Register_A, 8), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x35), COMBINATIONS(
        OP_COMB(OPS(OP(Register_A, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(Register_A, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(Register_A, 64), OP(Immediate, 32))),
    )),
    ENCODING(OP_CODE(0x80),ENC_OPTS(Digit, .digit = 6), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory,8), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(RegisterOrMemory,8), OP(Immediate, 8))),
        )),
    ENCODING(OP_CODE(0x81),ENC_OPTS(Digit, .digit = 6), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Immediate, 32))),
    )),
    ENCODING(OP_CODE(0x83),ENC_OPTS(Digit, .digit = 6), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Immediate, 8))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Immediate, 8))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Immediate, 8))),
    )),
    ENCODING(OP_CODE(0x30),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(RegisterOrMemory, 8), OP(Register, 8))),
    )),
    ENCODING(OP_CODE(0x31),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(RegisterOrMemory, 16), OP(Register, 16))),
        OP_COMB(OPS(OP(RegisterOrMemory, 32), OP(Register, 32))),
        OP_COMB(.rex_byte = Rex::W,   OPS(OP(RegisterOrMemory, 64), OP(Register, 64))),
    )),
    ENCODING(OP_CODE(0x32),ENC_OPTS(Reg), COMBINATIONS(
        OP_COMB(OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
        OP_COMB(.rex_byte = Rex::Rex,    OPS(OP(Register, 8), OP(RegisterOrMemory, 8))),
    )),
    ENCODING(OP_CODE(0x33),ENC_OPTS(Reg), COMBINATIONS(
    	OP_COMB(OPS(OP(Register, 16), OP(RegisterOrMemory, 16))),
    	OP_COMB(OPS(OP(Register, 32), OP(RegisterOrMemory, 32))),
    	OP_COMB(.rex_byte = Rex::W,   OPS(OP(Register, 64), OP(RegisterOrMemory, 64))),
    )),
};

const InstructionEncoding xrstor_encoding[] = { 0 };
const InstructionEncoding xrstors_encoding[] = { 0 };
const InstructionEncoding xsave_encoding[] = { 0 };
const InstructionEncoding xsavec_encoding[] = { 0 };
const InstructionEncoding xsaveopt_encoding[] = { 0 };
const InstructionEncoding xsaves_encoding[] = { 0 };
const InstructionEncoding xsetbv_encoding[] = { 0 };
const InstructionEncoding xtest_encoding[] = { 0 };


#define define_mnemonic(instruction)\
    const Mnemonic instruction = { .encodings = (const InstructionEncoding*) instruction ## _encoding, .encoding_count = rns_array_length(instruction ## _encoding), }

define_mnemonic(adc);
define_mnemonic(add);
define_mnemonic(call);
define_mnemonic(cmp);
define_mnemonic(cwd);
define_mnemonic(cdq);
define_mnemonic(cqo);
define_mnemonic(div_);
define_mnemonic(idiv);
define_mnemonic(imul);
define_mnemonic(inc);
define_mnemonic(int3);

define_mnemonic(jmp);

define_mnemonic(ja);
define_mnemonic(jae);
define_mnemonic(jb);
define_mnemonic(jbe);
define_mnemonic(jc);
define_mnemonic(jecxz);
define_mnemonic(jrcxz);
define_mnemonic(je);
define_mnemonic(jg);
define_mnemonic(jge);
define_mnemonic(jl);
define_mnemonic(jle);
define_mnemonic(jna);
define_mnemonic(jnae);
define_mnemonic(jnb);
define_mnemonic(jnbe);
define_mnemonic(jnc);
define_mnemonic(jne);
define_mnemonic(jng);
define_mnemonic(jnge);
define_mnemonic(jnl_);
define_mnemonic(jnle);
define_mnemonic(jno);
define_mnemonic(jnp);
define_mnemonic(jns);
define_mnemonic(jnz);
define_mnemonic(jo);
define_mnemonic(jp);
define_mnemonic(jpe);
define_mnemonic(jpo);
define_mnemonic(js);
define_mnemonic(jz);

define_mnemonic(lea);
define_mnemonic(mul);
define_mnemonic(mov);
define_mnemonic(pop);
define_mnemonic(push);
define_mnemonic(ret);
define_mnemonic(sete);
define_mnemonic(setg);
define_mnemonic(setl);
define_mnemonic(setz);
define_mnemonic(sub);
define_mnemonic(xor_);
