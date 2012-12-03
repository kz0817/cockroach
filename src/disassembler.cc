#include <cstdio>
#include <cstring>
#include "disassembler.h"
#include "utils.h"

#ifdef __x86_64__

#define PREFIX_REX_W (1 << 0)

struct code_parser_arg_t {
	// input to the parser
	uint32_t prefix;
	uint8_t *code;

	// output from the pasrser
	int parsed_length;
};

typedef void (*code_parser_t)(code_parser_arg_t *arg);

struct instr_info {
	int length;
	uint32_t prefix;
	code_parser_t parser;
};

static void parse_operand(uint8_t *code)
{
	mod =   (code[0] & 0xc0) >> 6;
	digit = (code[0] & 0x38) >> 3;
	r_m =   (code[0] & 0x07);
}

// 0x48
static instr_info instr_info_rex_w = {
	1,
	PREFIX_REX_W,
};

// 0x89
static void parser_mov_ev_gv(code_parser_arg_t *arg)
{
	uint8_t *code = arg->code;
	printf("%s, code: %p\n", __func__, code);
	printf("code: %02x %02x %02x\n", code[0], code[1], code[2]);
	parser_operand(arg->code);
	ROACH_ABORT();
}

static instr_info instr_info_mov_ev_gv = {
	1,
	0,
	parser_mov_ev_gv,
};


// 0xc3
static instr_info instr_info_ret = {
	1,
	0,
};


static instr_info *first_byte_instr_array[0x100] = 
{
	NULL,                    // 0x00
	NULL,                    // 0x01
	NULL,                    // 0x02
	NULL,                    // 0x03
	NULL,                    // 0x04
	NULL,                    // 0x05
	NULL,                    // 0x06
	NULL,                    // 0x07
	NULL,                    // 0x08
	NULL,                    // 0x09
	NULL,                    // 0x0a
	NULL,                    // 0x0b
	NULL,                    // 0x0c
	NULL,                    // 0x0d
	NULL,                    // 0x0e
	NULL,                    // 0x0f

	NULL,                    // 0x10
	NULL,                    // 0x11
	NULL,                    // 0x12
	NULL,                    // 0x13
	NULL,                    // 0x14
	NULL,                    // 0x15
	NULL,                    // 0x16
	NULL,                    // 0x17
	NULL,                    // 0x18
	NULL,                    // 0x19
	NULL,                    // 0x1a
	NULL,                    // 0x1b
	NULL,                    // 0x1c
	NULL,                    // 0x1d
	NULL,                    // 0x1e
	NULL,                    // 0x1f

	NULL,                    // 0x20
	NULL,                    // 0x21
	NULL,                    // 0x22
	NULL,                    // 0x23
	NULL,                    // 0x24
	NULL,                    // 0x25
	NULL,                    // 0x26
	NULL,                    // 0x27
	NULL,                    // 0x28
	NULL,                    // 0x29
	NULL,                    // 0x2a
	NULL,                    // 0x2b
	NULL,                    // 0x2c
	NULL,                    // 0x2d
	NULL,                    // 0x2e
	NULL,                    // 0x2f

	NULL,                    // 0x30
	NULL,                    // 0x31
	NULL,                    // 0x32
	NULL,                    // 0x33
	NULL,                    // 0x34
	NULL,                    // 0x35
	NULL,                    // 0x36
	NULL,                    // 0x37
	NULL,                    // 0x38
	NULL,                    // 0x39
	NULL,                    // 0x3a
	NULL,                    // 0x3b
	NULL,                    // 0x3c
	NULL,                    // 0x3d
	NULL,                    // 0x3e
	NULL,                    // 0x3f

	NULL,                    // 0x40
	NULL,                    // 0x41
	NULL,                    // 0x42
	NULL,                    // 0x43
	NULL,                    // 0x44
	NULL,                    // 0x45
	NULL,                    // 0x46
	NULL,                    // 0x47
	&instr_info_rex_w,       // 0x48
	NULL,                    // 0x49
	NULL,                    // 0x4a
	NULL,                    // 0x4b
	NULL,                    // 0x4c
	NULL,                    // 0x4d
	NULL,                    // 0x4e
	NULL,                    // 0x4f
/*
	&instr_info_push_ax,     // 0x50
	&instr_info_push_cx,     // 0x51
	&instr_info_push_dx,     // 0x52
	&instr_info_push_bx,     // 0x53
	&instr_info_push_sp,     // 0x54
	&instr_info_push_bp,     // 0x55
	&instr_info_push_si,     // 0x56
	&instr_info_push_di,     // 0x57
	&instr_info_pop_ax,      // 0x58
	&instr_info_pop_cx,      // 0x59
	&instr_info_pop_dx,      // 0xab
	&instr_info_pop_bx,      // 0x5b
	&instr_info_pop_sp,      // 0x5c
	&instr_info_pop_bp,      // 0x5d
	&instr_info_pop_si,      // 0x5e
	&instr_info_pop_di,      // 0x5f
*/
	NULL,                    // 0x50
	NULL,                    // 0x51
	NULL,                    // 0x52
	NULL,                    // 0x53
	NULL,                    // 0x54
	NULL,                    // 0x55
	NULL,                    // 0x56
	NULL,                    // 0x57
	NULL,                    // 0x58
	NULL,                    // 0x59
	NULL,                    // 0x5a
	NULL,                    // 0x5b
	NULL,                    // 0x5c
	NULL,                    // 0x5d
	NULL,                    // 0x5e
	NULL,                    // 0x5f

	NULL,                    // 0x60
	NULL,                    // 0x61
	NULL,                    // 0x62
	NULL,                    // 0x63
	NULL,                    // 0x64
	NULL,                    // 0x65
	NULL,                    // 0x66
	NULL,                    // 0x67
	NULL,                    // 0x68
	NULL,                    // 0x69
	NULL,                    // 0x6a
	NULL,                    // 0x6b
	NULL,                    // 0x6c
	NULL,                    // 0x6d
	NULL,                    // 0x6e
	NULL,                    // 0x6f

	NULL,                    // 0x70
	NULL,                    // 0x71
	NULL,                    // 0x72
	NULL,                    // 0x73
	NULL,                    // 0x74
	NULL,                    // 0x75
	NULL,                    // 0x76
	NULL,                    // 0x77
	NULL,                    // 0x78
	NULL,                    // 0x79
	NULL,                    // 0x7a
	NULL,                    // 0x7b
	NULL,                    // 0x7c
	NULL,                    // 0x7d
	NULL,                    // 0x7e
	NULL,                    // 0x7f

	NULL,                    // 0x80
	NULL,                    // 0x81
	NULL,                    // 0x82
	NULL,                    // 0x83
	NULL,                    // 0x84
	NULL,                    // 0x85
	NULL,                    // 0x86
	NULL,                    // 0x87
	NULL,                    // 0x88
	&instr_info_mov_ev_gv,   // 0x89
	NULL,                    // 0x8a
	NULL,                    // 0x8b
	NULL,                    // 0x8c
	NULL,                    // 0x8d
	NULL,                    // 0x8e
	NULL,                    // 0x8f
/*
	&instr_info_pushf,       // 0x9c
	&instr_info_popf,        // 0x9d
*/
	&instr_info_ret,         // 0xc3
};

#endif // __x86_64__

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
int disassembler::parse(uint8_t *code_start)
{
	int parsed_length = 0;
	uint8_t *code = code_start;

	code_parser_arg_t arg;
	memset(&arg, 0, sizeof(code_parser_t));
	while (true) {
		instr_info *info = first_byte_instr_array[*code];
		if (info == NULL) {
			ROACH_ERR("Failed to parse 1st byte: %p: %02x\n",
			          code, *code);
			ROACH_ABORT();
		}
		parsed_length += info->length;
		code += info->length;

		if (info->parser) {
			arg.code = code;
			(*info->parser)(&arg);
			parsed_length += arg.parsed_length;
			code += arg.parsed_length;
		}

		// If the first byte is prefix, we parse again
		if (info->prefix & PREFIX_REX_W) {
			arg.prefix = PREFIX_REX_W;
			continue;
		}
		break;
	}

	return parsed_length;
}

