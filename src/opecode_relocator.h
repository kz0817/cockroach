#ifndef opecode_relocator_h
#define opecode_relocator_h

#include "opecode.h"

class opecode_relocator {
	opecode *m_op;
public:
	opecode_relocator(opecode *op);
	int get_relocated_area_length(void);
};

#endif

