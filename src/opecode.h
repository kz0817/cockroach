#ifndef opecode_h
#define opecode_h

#include <stdint.h>

#ifdef __x86_64__
class opecode_relocator;

#define PREFIX_REX_B (1 << 1)
#define PREFIX_REX_W (1 << 8)
#define PREFIX_REX_WB (PREFIX_REX_W|PREFIX_REX_B)

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

enum rel_jump_t {
	REL_INVALID,
	REL8,
	REL16,
	REL32,
};

struct mod_rm {
	int mod;
	int reg;
	int r_m;

	// constructor
	mod_rm(void);
};

struct sib {
	int ss;
	int index;
	int base;

	// constructor
	sib(void);
};

class opecode {
	uint8_t *m_original_addr;
	int      m_length;
	uint8_t *m_code;
	int      m_prefix;
	mod_rm   m_mod_rm;
	sib      m_sib;
	opecode_disp_t m_disp_type;
	uint32_t       m_disp;
	opecode_imm_t m_immediate_type;
	uint64_t      m_immediate;
	rel_jump_t    m_rel_jump_type;
	int32_t       m_rel_jump_value;
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
	void set_rel_jump_addr(rel_jump_t rel_type, int32_t value);
	void copy_code(uint8_t *addr);
	opecode_relocator *get_relocator(void);
	const opecode_disp_t get_disp_type(void) const;
	const mod_rm &get_mod_rm(void) const;
};

#endif // __x86_64__

#endif
