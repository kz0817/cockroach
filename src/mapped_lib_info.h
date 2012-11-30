#ifndef mapped_lib_info_h
#define mapped_lib_info_h

#include <string>
#include <set>
using namespace std;

class mapped_lib_info {
	string m_path;
	unsigned long m_start_addr;

	string m_filename;
	unsigned long m_length;
public:
	mapped_lib_info(const char *path,
	                unsigned long start_addr, unsigned long length);
	bool operator<(const mapped_lib_info &lib_info) const;
};

typedef set<mapped_lib_info> mapped_lib_info_set_t;
typedef mapped_lib_info_set_t::iterator mapped_lib_info_set_itr;

#endif
