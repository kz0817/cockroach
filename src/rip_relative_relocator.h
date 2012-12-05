#ifndef rip_relative_relocator_h
#define rip_relative_relocator_h

#ifdef __x86_64__
#include "opecode_relocator.h"

class rip_relative_relocator : public opecode_relocator {
	int m_disp_offset; // offset of the DISP location from the opecode top
public:
	rip_relative_relocator(opecode *op, int offset);
	~rip_relative_relocator();

	// virtual function
	int relocate(uint8_t *addr);
	//int get_max_code_length(void);
};

#endif // __x86_64__
#endif


