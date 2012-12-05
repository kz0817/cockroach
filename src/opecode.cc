#include <cstdio>
#include "opecode.h"

#ifdef __x86_64__
// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------

opecode::opecode(void)
: m_length(0),
  m_code(NULL),
  m_prefix(0),
  m_mod_rm_reg(0)
{
}

opecode::~opecode()
{
	if (m_code)
		delete m_code;
}

void opecode::inc_length(int length)
{
	m_length += length;
}

void opecode::add_prefix(int prefix)
{
	m_prefix |= prefix;
}

int opecode::get_length(void)
{
	return m_length;
}

void opecode::set_mod_rm_reg(int reg)
{
	m_mod_rm_reg = reg;
}

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------


#endif // __x86_64__
