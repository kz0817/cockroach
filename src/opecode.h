#ifndef opecode_h
#define opecode_h

#include <stdint.h>

#ifdef __x86_64__
#include "opecode_relocator.h"

#define PREFIX_REX_W (1 << 0)

enum opecode_disp_t {
	DISP_NONE,
	DISP8,
	DISP32,
};

enum opecode_imm_t {
	IMM_INVALID,
	IMM8,
	IMM16,
	IMM32,
	IMM64,
};

class opecode {
	int      m_length;
	uint8_t *m_code;
	int      m_prefix;
	int      m_mod_rm_reg;
	int      m_sib_ss;
	int      m_sib_index;
	int      m_sib_base;
	opecode_disp_t m_disp_type;
	uint32_t       m_disp;
	opecode_imm_t m_immediate_type;
	uint64_t      m_immediate;
	opecode_relocator *m_relocator;
public:
	opecode(void);
	virtual ~opecode();

	void inc_length(int length = 1);
	int  get_length(void);
	void add_prefix(int prefix);
	void set_mod_rm_reg(int reg);
	void set_sib_param(int ss, int index, int base);
	void set_disp(opecode_disp_t disp_type, uint32_t disp);
	void set_immediate(opecode_imm_t imm_type, uint64_t imm);
	void copy_code(uint8_t *addr);
};

#endif // __x86_64__

#endif
