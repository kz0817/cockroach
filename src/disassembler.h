#ifndef disassembler_h
#define disassembler_h

#include <stdint.h>

class disassembler {
	
public:
	static int parse(uint8_t *code_start);
};

#endif

