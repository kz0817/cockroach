#ifndef opecode_h
#define opecode_h

#include <stdint.h>

#ifdef __x86_64__

#define PREFIX_REX_W (1 << 0)

enum opecode_disp_t {
	DISP_NONE,
	DISP8,
	DISP32,
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
	int      m_disp;
public:
	opecode(void);
	virtual ~opecode();

	void inc_length(int length = 1);
	int  get_length(void);
	void add_prefix(int prefix);
	void set_mod_rm_reg(int reg);
	void set_sib_param(int ss, int index, int base);
	void set_disp_type(opecode_disp_t disp_type);
	void set_disp(int);
	void copy_code(uint8_t *addr);
};

#endif // __x86_64__

#endif
