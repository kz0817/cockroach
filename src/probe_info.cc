#include "utils.h"
#include "probe_info.h"

probe_info::probe_info(probe_type type)
: m_type(type),
  m_offset_addr(0),
  m_probe(NULL),
  m_ret_probe(NULL)
{
}

void probe_info::set_target_address(char *shared_lib_path, unsigned long addr)
{
	if (shared_lib_path)
		m_shared_lib_path = shared_lib_path;
	else
		m_shared_lib_path.erase();
	m_offset_addr = addr;
}

void probe_info::set_prove(char *probe_lib_path, probe_func_t probe)
{
	if (probe_lib_path)
		m_probe_lib_path = probe_lib_path;
	else
		m_probe_lib_path.erase();
	m_probe =  probe;
}

void probe_info::set_ret_prove(probe_func_t probe)
{
	m_ret_probe = probe;
}
