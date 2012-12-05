#ifndef opecode_h
#define opecode_h

#include <stdint.h>

#ifdef __x86_64__

#define PREFIX_REX_W (1 << 0)


class opecode {
	int      m_length;
	uint8_t *m_code;
	int      m_prefix;
	int      m_mod_rm_reg;
public:
	opecode(void);
	virtual ~opecode();

	void inc_length(int length = 1);
	int  get_length(void);
	void add_prefix(int prefix);
	void set_mod_rm_reg(int reg);
	void copy_code(uint8_t *addr);
};

#endif // __x86_64__

#endif
