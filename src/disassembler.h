#ifndef disassembler_h
#define disassembler_h

#include <stdint.h>
#include "opecode.h"

class disassembler {
	
public:
	static opecode *parse(uint8_t *code_start);
};

#endif

