#ifndef mapped_lib_info_h
#define mapped_lib_info_h

#include <string>
#include <map>
using namespace std;

class mapped_lib_info {
	bool m_is_exe;
	string m_path;
	unsigned long m_start_addr;

	string m_filename;
	unsigned long m_length;
public:
	mapped_lib_info(const char *path,
	                unsigned long start_addr, unsigned long length,
	                bool is_exe);
	const char *get_path(void) const;
	const char *get_filename(void) const;
	unsigned long get_addr(void) const;
	bool is_exe(void) const;
};

typedef map<string, mapped_lib_info*> mapped_lib_info_map_t;
typedef mapped_lib_info_map_t::iterator mapped_lib_info_map_itr;

#endif
