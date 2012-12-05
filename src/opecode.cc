#include <cstdio>
#include <cstring>
#include "opecode.h"
#include "utils.h"
#include "opecode_relocator.h"

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
  m_disp(0),
  m_immediate_type(IMM_INVALID),
  m_immediate(0),
  m_relocator(NULL),
  m_relocated_code_size(0)
{
}

opecode::~opecode()
{
	if (m_code)
		delete [] m_code;
	if (m_relocator)
		delete m_relocator;
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

void opecode::set_disp(opecode_disp_t disp_type, uint32_t disp)
{
	m_disp_type = disp_type;
	m_disp = disp;
}

void opecode::set_immediate(opecode_imm_t imm_type, uint64_t imm)
{
	m_immediate_type = imm_type;
	m_immediate = imm;
}

void opecode::copy_code(uint8_t *addr)
{
	if (m_length == 0)
		ROACH_BUG("m_length: 0\n");
	if (m_code)
		delete [] m_code;
	m_code = new uint8_t[m_length];
	memcpy(m_code, addr, m_length);
}

int opecode::get_relocated_code_length(void)
{
	if (m_relocator) {
  		if (m_relocated_code_size == 0) {
			int len = m_relocator->get_relocated_area_length();
			m_relocated_code_size = len;
		}
		return m_relocated_code_size;
	}
	return m_length;
}

void opecode::relocate(uint8_t *dest_addr)
{
	if (m_length == 0)
		ROACH_BUG("m_length: 0\n");
	memcpy(dest_addr, m_code, m_length);
}

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------


#endif // __x86_64__
