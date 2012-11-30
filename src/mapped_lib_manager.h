#ifndef mapped_lib_manager_h
#define mapped_lib_manager_h

#include "mapped_lib_info.h"

class mapped_lib_manager {
	mapped_lib_info_set_t m_lib_info_set;

	static void _parse_mapped_lib_line(const char *line, void *arg);
	void parse_mapped_lib_line(const char *line);
public:
	mapped_lib_manager();
	const mapped_lib_info *get_lib_info(const char *name);
};

#endif
