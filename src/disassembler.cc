#include <cstdio>
#include "disassembler.h"
#include "utils.h"

#ifdef __x86_64__

struct instr_info {
	int length;
};

static instr_info instr_info_ret = {
	1,
};

static instr_info *first_byte_instr_array[0x100] = 
{
	NULL,                    // 0x00
	NULL,                    // 0x01
/*
	&instr_info_push_ax,     // 0x50
	&instr_info_push_cx,     // 0x51
	&instr_info_push_dx,     // 0x52
	&instr_info_push_bx,     // 0x53
	NULL,                    // 0x54
	&instr_info_push_bp,     // 0x55
	&instr_info_push_si,     // 0x56
	&instr_info_push_di,     // 0x57
	&instr_info_pop_ax,      // 0x58
	&instr_info_pop_cx,      // 0x59
	&instr_info_pop_dx,      // 0xab
	&instr_info_pop_bx,      // 0x5b
	NULL,                    // 0x5c
	&instr_info_pop_bp,      // 0x5d
	&instr_info_pop_si,      // 0x5e
	&instr_info_pop_di,      // 0x5f
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
	uint8_t *code = code_start;
	instr_info *info = first_byte_instr_array[*code];
	if (info == NULL) {
		ROACH_ERR("Failed to parse 1st byte: %p\n", code);
		ROACH_ABORT();
	}
	//if (info->length == 1)
	return info->length;
}

