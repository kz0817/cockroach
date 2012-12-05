#include <cstring>
#include "opecode_relocator.h"
#include "utils.h"

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
opecode_relocator::opecode_relocator(opecode *op)
: m_op(op)
{
}

opecode_relocator::~opecode_relocator()
{
}

const opecode *opecode_relocator::get_opecode(void)
{
	return m_op;
}

int opecode_relocator::get_max_code_length(void)
{
	return m_op->get_length();
}

int opecode_relocator::get_max_data_length(void)
{
	return 0;
}

int opecode_relocator::relocate(uint8_t *dest_addr)
{
	int length = m_op->get_length();
	if (length == 0)
		ROACH_BUG("length: 0\n");
	uint8_t *code = m_op->get_code();
	if (code == NULL)
		ROACH_BUG("code: NULL\n");
	memcpy(dest_addr, code, length);
	return length;
}

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------

