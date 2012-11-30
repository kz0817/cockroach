#include <cstdio>
using namespace std;

#include <sys/mman.h>
#include <unistd.h>

#include "utils.h"
#include "probe_info.h"

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------
void probe_info::change_page_permission(void *addr)
{
	int page_size = utils::get_page_size();
	int prot =  PROT_READ | PROT_WRITE | PROT_EXEC;
	if (mprotect(addr, page_size, prot) == -1) {
		/*
		EMPL_P(ERR, "Failed to mprotect: %p, %d, %x (%d)\n",
		       addr, page_size, prot, errno);
		abort();
		*/
	}
}

void probe_info::change_page_permission_all(void *addr, int len)
{
	int page_size = utils::get_page_size();
	unsigned long mask = ~(page_size - 1);
	unsigned long addr_ul = (unsigned long)addr;
	unsigned long addr_aligned = addr_ul & mask;
	change_page_permission((void *)addr_aligned);

	int mod = addr_ul % page_size;
	if (mod > page_size - len)
		change_page_permission((void *)(addr_aligned + page_size));
}

#ifdef __x86_64__
void probe_info::overwrite_jump_code(uint8_t *intrude_addr,
                                     uint8_t *jump_abs_addr,
                                     int copy_code_size)
{
	change_page_permission_all(intrude_addr, copy_code_size);

	// calculate the count of nop, which should be filled
	int idx;
	int len_nops = copy_code_size - JUMP_OP_LEN;
	if (len_nops < 0) {
		/*
		EMPL_P(BUG, "len_nops is negaitve: %d, %d\n",
		       copy_code_size, LenRelCall);
		abort();
		*/
	}
	uint8_t *code = intrude_addr;

	// push $rax
	*code = 0x50;
	code++;

	// mov $jump_abs_addr, $rax
	*code = 0x48;
	code++;
	*code = 0xb8;
	code++;

	//*((uint64_t *)code) = jump_abs_addr;
	code += 8;

	// push %rax
	*code = 0x50;
	code++;

	// ret
	*code = 0xc3;
	code++;

	// fill NOP instructions
	for (idx = 0; idx < len_nops; idx++, code++)
		*code = 0x90;  // nop
}
#endif // __x86_64__

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
probe_info::probe_info(probe_type type)
: m_type(type),
  m_offset_addr(0),
  m_probe(NULL),
  m_ret_probe(NULL)
{
}

void probe_info::set_target_address(const char *target_lib_path, unsigned long addr, int overwrite_length)
{
	if (target_lib_path)
		m_target_lib_path = target_lib_path;
	else
		m_target_lib_path.erase();
	m_offset_addr = addr;
	m_overwrite_length = overwrite_length;
}

void probe_info::set_probe(const char *probe_lib_path, probe_func_t probe)
{
	if (probe_lib_path)
		m_probe_lib_path = probe_lib_path;
	else
		m_probe_lib_path.erase();
	m_probe =  probe;
}

void probe_info::set_ret_probe(probe_func_t probe)
{
	m_ret_probe = probe;
}

const char *probe_info::get_target_lib_path(void)
{
	return m_target_lib_path.c_str();
}

void probe_info::install(const mapped_lib_info *lib_info)
{
	printf("install: %s: %08lx, @ %016lx, %d\n",
	       lib_info->get_path(), m_offset_addr, lib_info->get_addr(),
	       m_overwrite_length);
	
}
