#include <cstdio>
#include <cstring>
#include "opecode.h"
#include "utils.h"
#include "rip_relative_relocator.h"

#ifdef __x86_64__
// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------

opecode::opecode(uint8_t *orig_addr)
: m_original_addr(orig_addr),
  m_length(0),
  m_code(NULL),
  m_prefix(0),
  m_mod_rm_mod(-1),
  m_mod_rm_reg(-1),
  m_mod_rm_r_m(-1),
  m_sib_ss(-1),
  m_sib_index(-1),
  m_sib_base(-1),
  m_disp_type(DISP_NONE),
  m_disp(0),
  m_immediate_type(IMM_INVALID),
  m_immediate(0),
  m_rel_jump_type(REL_INVALID),
  m_rel_jump_value(0),
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

uint8_t *opecode::get_original_addr(void) const
{
	return m_original_addr;
}

void opecode::inc_length(int length)
{
	m_length += length;
}

void opecode::add_prefix(int prefix)
{
	m_prefix |= prefix;
}

int opecode::get_length(void) const
{
	return m_length;
}

uint8_t *opecode::get_code(void) const
{
	return m_code;
}

void opecode::set_mod_rm(int mod, int reg, int r_m)
{
	m_mod_rm_mod = mod;
	m_mod_rm_reg = reg;
	m_mod_rm_r_m = r_m;
}

void opecode::set_sib_param(int ss, int index, int base)
{
	m_sib_ss = ss;
	m_sib_index = index;
	m_sib_base = base;
}

void opecode::set_disp(opecode_disp_t disp_type, uint32_t disp,
                       uint8_t *disp_orig_addr)
{
	// chkeck if RIP relative addressing
	if (m_mod_rm_mod == -1 || m_mod_rm_r_m == -1) {
		ROACH_BUG("Mod or R/M have not been set: %d, %d\n",
		          m_mod_rm_mod, m_mod_rm_r_m == -1);
	}
	if (m_mod_rm_mod == 0 && m_mod_rm_r_m == 5) {
		int offset = (int)(disp_orig_addr - m_original_addr);
		m_relocator = new rip_relative_relocator(this, offset);
	}

	m_disp_type = disp_type;
	m_disp = disp;
}

uint32_t opecode::get_disp(void) const
{
	return m_disp;
}

void opecode::set_immediate(opecode_imm_t imm_type, uint64_t imm)
{
	m_immediate_type = imm_type;
	m_immediate = imm;
}

void opecode::set_rel_jump_addr(rel_jump_t rel_type, int32_t value)
{
	m_rel_jump_type = rel_type;
	m_rel_jump_value = value;
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

opecode_relocator *opecode::get_relocator(void)
{
	if (!m_relocator)
		m_relocator = new opecode_relocator(this);
	return m_relocator;
}

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------


#endif // __x86_64__
