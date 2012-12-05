#ifndef opecode_h
#define opecode_h

#include <stdint.h>

#ifdef __x86_64__
class opecode_relocator;

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
	uint8_t *m_original_addr;
	int      m_length;
	uint8_t *m_code;
	int      m_prefix;
	int      m_mod_rm_mod;
	int      m_mod_rm_reg;
	int      m_mod_rm_r_m;
	int      m_sib_ss;
	int      m_sib_index;
	int      m_sib_base;
	opecode_disp_t m_disp_type;
	uint32_t       m_disp;
	opecode_imm_t m_immediate_type;
	uint64_t      m_immediate;
	opecode_relocator *m_relocator;
	int                m_relocated_code_size;
public:
	opecode(uint8_t *orig_addr);
	virtual ~opecode();

	uint8_t *get_original_addr(void) const;
	void inc_length(int length = 1);
	int  get_length(void) const;
	uint8_t *get_code(void) const;
	void add_prefix(int prefix);
	void set_mod_rm(int mod, int reg, int r_m);
	void set_sib_param(int ss, int index, int base);
	void set_disp(opecode_disp_t disp_type, uint32_t disp,
	              uint8_t *disp_orig_addr);
	uint32_t get_disp(void) const;
	void set_immediate(opecode_imm_t imm_type, uint64_t imm);
	void copy_code(uint8_t *addr);
	opecode_relocator *get_relocator(void);
};

#endif // __x86_64__

#endif
