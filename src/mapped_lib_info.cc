#include <cstdio>
#include "utils.h"
#include "mapped_lib_info.h"

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
mapped_lib_info::mapped_lib_info(const char *path, unsigned long start_addr,
                                 unsigned long length)
: m_path(path),
  m_start_addr(start_addr),
  m_length(length)
{
	m_filename = utils::get_basename(path);
}

bool mapped_lib_info::operator<(const mapped_lib_info &lib_info) const
{
	printf("%s\n", __func__);
	return true;
}
