#include <cstdio>
#include <cstring>
#include "opecode.h"

#ifdef __x86_64__
// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------

opecode::opecode(void)
: m_length(0),
  m_code(NULL),
  m_prefix(0),
  m_mod_rm_reg(-1),
  m_sib_ss(-1),
  m_sib_index(-1),
  m_sib_base(-1),
  m_disp_type(DISP_NONE),
  m_disp(-1)
{
}

opecode::~opecode()
{
	if (m_code)
		delete [] m_code;
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

void opecode::set_sib_param(int ss, int index, int base)
{
	m_sib_ss = ss;
	m_sib_index = index;
	m_sib_base = base;
}

void opecode::set_disp_type(opecode_disp_t disp_type)
{
	m_disp_type = disp_type;
}

void opecode::set_disp(int disp)
{
	m_disp = disp;
}

void opecode::copy_code(uint8_t *addr)
{
	if (m_code)
		delete [] m_code;
	m_code = new uint8_t[m_length];
	memcpy(m_code, addr, m_length);
}

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------


#endif // __x86_64__
