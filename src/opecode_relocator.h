#ifndef opecode_relocator_h
#define opecode_relocator_h

#include "opecode.h"

class opecode_relocator {
	const opecode *m_op;

protected:
	const opecode *get_opecode(void);

public:
	opecode_relocator(opecode *op);
	virtual ~opecode_relocator();
	virtual int get_max_code_length(void);
	virtual int get_max_data_length(void);
	virtual int relocate(uint8_t *addr);
};

#endif

