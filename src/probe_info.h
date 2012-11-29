#ifndef probe_info_h
#define probe_info_h

#include <string>
using namespace std;

enum probe_type {
	PROBE_TYPE_OVERWRITE_FUNC_HEAD,
};

class probe_info {
	probe_type m_type;
	string m_shared_lib_path;
	string m_symbol_name;
	unsigned long m_offset;
public:
};

#endif


