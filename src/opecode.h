#ifndef opecode_h
#define opecode_h

#include <stdint.h>

#ifdef __x86_64__
class opecode_relocator;

#define PREFIX_REX_B (1 << 1)
#define PREFIX_REX_W (1 << 8)
#define PREFIX_REX_WB (PREFIX_REX_W|PREFIX_REX_B)

enum mod_type {
	MOD_REG_UNKNOWN = -1,
	MOD_REG_INDIRECT,
	MOD_REG_INDIRECT_DISP8,
	MOD_REG_INDIRECT_DISP32,
	MOD_REG_DIRECT,
};

enum register_type {
	REG_UNKNOWN = -1,
	REG_AX,
	REG_CX,
	REG_DX,
	REG_BX,
	REG_SP,
	REG_BP,
	REG_SI,
	REG_DI,
};

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
	mod_type      mod;
	register_type reg;
	register_type r_m;

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

struct disp {
	opecode_disp_t type;
	uint32_t       value;

	// constructor
	disp(void);
};

struct immediate {
	opecode_imm_t type;
	uint64_t      value;

	// constructor
	immediate(void);
};

class opecode {
	uint8_t *m_original_addr;
	int      m_length;
	uint8_t *m_code;
	int      m_prefix;
	mod_rm   m_mod_rm;
	sib      m_sib;
	disp      m_disp;
	immediate m_immediate;
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
	void set_mod_rm(mod_type mod, register_type reg, register_type r_m);
	void set_sib_param(int ss, int index, int base);
	void set_disp(opecode_disp_t disp_type, uint32_t disp,
	              uint8_t *disp_orig_addr);
	void set_immediate(opecode_imm_t imm_type, uint64_t imm);
	void set_rel_jump_addr(rel_jump_t rel_type, int32_t value);
	void copy_code(uint8_t *addr);
	opecode_relocator *get_relocator(void);
	const mod_rm &get_mod_rm(void) const;
	const sib &get_sib(void) const;
	const disp &get_disp(void) const;
	const immediate &get_immediate(void) const;
};

#endif // __x86_64__

#endif
