#ifndef rip_relative_relocator_h
#define rip_relative_relocator_h

#include "opecode_relocator.h"

class rip_relative_relocator : public opecode_relocator {
public:
	rip_relative_relocator(opecode *op);
	~rip_relative_relocator();

	// virtual function
	//int relocate(uint8_t *addr);
	//int get_max_code_length(void);
};

#endif


