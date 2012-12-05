#include "rip_relative_relocator.h"
#include "utils.h"

rip_relative_relocator::rip_relative_relocator(opecode *op)
: opecode_relocator(op)
{
}

rip_relative_relocator::~rip_relative_relocator()
{
}
/*
int rip_relative_relocator::relocate(uint8_t *addr)
{
	ROACH_BUG("Not implemented\n");
	return 0;
}

int rip_relative_relocator::get_max_code_length(void)
{
	ROACH_BUG("Not implemented\n");
	return 0;
}*/
