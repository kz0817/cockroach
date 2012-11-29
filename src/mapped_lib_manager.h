#ifndef mapped_lib_manager_h
#define mapped_lib_manager_h

#include "mapped_lib_info.h"

class mapped_lib_manager {
	mapped_lib_info_set_t m_lib_info_set;

	void parse_mapped_lib_line(const char *line);
public:
	mapped_lib_manager();
};

#endif
