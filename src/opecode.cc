#include <cstdio>
#include <cstring>
#include "opecode.h"
#include "utils.h"
#include "rip_relative_relocator.h"

#ifdef __x86_64__
mod_rm::mod_rm(void)
: mod(MOD_REG_UNKNOWN),
  reg(REG_UNKNOWN),
  r_m(REG_UNKNOWN)
{
}

sib::sib(void)
: ss(-1),
  index(-1),
  base(-1)
{
}

disp::disp(void)
: type(DISP_NONE),
  value(0)
{
}

immediate::immediate(void)
: type(IMM_INVALID),
  value(0)
{
}

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------

opecode::opecode(uint8_t *orig_addr)
: m_original_addr(orig_addr),
  m_length(0),
  m_code(NULL),
  m_prefix(0),
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

void opecode::set_mod_rm(mod_type mod, register_type reg, register_type r_m)
{
	m_mod_rm.mod = mod;
	m_mod_rm.reg = reg;
	m_mod_rm.r_m = r_m;
}

void opecode::set_sib_param(int ss, int index, int base)
{
	m_sib.ss = ss;
	m_sib.index = index;
	m_sib.base = base;
}

void opecode::set_disp(opecode_disp_t disp_type, uint32_t disp,
                       uint8_t *disp_orig_addr)
{
	// chkeck if RIP relative addressing
	if (m_mod_rm.mod == -1 || m_mod_rm.r_m == -1) {
		ROACH_BUG("Mod or R/M have not been set: %d, %d\n",
		          m_mod_rm.mod, m_mod_rm.r_m == -1);
	}
	if (m_mod_rm.mod == 0 && m_mod_rm.r_m == 5) {
		int offset = (int)(disp_orig_addr - m_original_addr);
		m_relocator = new rip_relative_relocator(this, offset);
	}

	m_disp.type = disp_type;
	m_disp.value = disp;
}

void opecode::set_immediate(opecode_imm_t imm_type, uint64_t imm)
{
	m_immediate.type = imm_type;
	m_immediate.value = imm;
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

const mod_rm &opecode::get_mod_rm(void) const
{
	return m_mod_rm;
}

const sib &opecode::get_sib(void) const
{
	return m_sib;
}

const disp &opecode::get_disp(void) const
{
	return m_disp;
}

const immediate &opecode::get_immediate(void) const
{
	return m_immediate;
}

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------


#endif // __x86_64__
