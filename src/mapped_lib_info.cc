#include <cstdio>
#include <cstring>
#include "utils.h"
#include "mapped_lib_info.h"

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
mapped_lib_info::mapped_lib_info(const char *path, unsigned long start_addr,
                                 unsigned long length, bool is_exe)
: m_is_exe(is_exe),
  m_path(path),
  m_start_addr(start_addr),
  m_length(length)
{
	m_filename = utils::get_basename(path);
}

const char *mapped_lib_info::get_path(void) const
{
	return m_path.c_str();
}

const char *mapped_lib_info::get_filename(void) const
{
	return m_filename.c_str();
}

unsigned long mapped_lib_info::get_addr(void) const
{
	return m_start_addr;
}

bool mapped_lib_info::is_exe(void) const
{
	return m_is_exe;
}
