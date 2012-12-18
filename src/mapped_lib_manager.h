#ifndef mapped_lib_manager_h
#define mapped_lib_manager_h

#include "mapped_lib_info.h"

class mapped_lib_manager {
	string m_exe_path;
	mapped_lib_info_map_t m_lib_info_path_map;
	mapped_lib_info_map_t m_lib_info_filename_map;

	static void _parse_mapped_lib_line(const char *line, void *arg);
	void parse_mapped_lib_line(const char *line);
public:
	mapped_lib_manager(void);
	const mapped_lib_info *get_lib_info(const char *name);
	void update(void);
};

#endif
