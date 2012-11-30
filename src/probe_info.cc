#include <cstdio>
using namespace std;

#include "utils.h"
#include "probe_info.h"

probe_info::probe_info(probe_type type)
: m_type(type),
  m_offset_addr(0),
  m_probe(NULL),
  m_ret_probe(NULL)
{
}

void probe_info::set_target_address(const char *target_lib_path, unsigned long addr)
{
	if (target_lib_path)
		m_target_lib_path = target_lib_path;
	else
		m_target_lib_path.erase();
	m_offset_addr = addr;
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
	printf("install: %s: %08lx, @ %016lx\n",
	       lib_info->get_path(), m_offset_addr, lib_info->get_addr());
}
