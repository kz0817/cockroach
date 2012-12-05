#include <cstdio>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "rip_relative_relocator.h"
#include "utils.h"

rip_relative_relocator::rip_relative_relocator(opecode *op, int offset)
: opecode_relocator(op),
  m_disp_offset(offset)
{
}

rip_relative_relocator::~rip_relative_relocator()
{
}

int rip_relative_relocator::relocate(uint8_t *addr)
{
	const opecode *op = get_opecode();
	int64_t diff = (uint64_t)op->get_original_addr() - (uint64_t)addr; 
	int64_t rel = op->get_disp() + diff;
	if (rel >  2147483647 || rel < -2147483648) {
		ROACH_ERR("relative addr: %016x\n", rel);
		ROACH_ABORT();
	}

	// copy the original code
	int length = opecode_relocator::relocate(addr);

	// overwrite DISP
	uint32_t *ptr = (uint32_t*)(addr + m_disp_offset);
	*ptr = rel;

	return length;
}

/*
int rip_relative_relocator::get_max_code_length(void)
{
	ROACH_BUG("Not implemented\n");
	return 0;
}*/
