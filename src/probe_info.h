#ifndef probe_info_h
#define probe_info_h

#include <string>
using namespace std;

#ifdef __x86_64__
struct probe_arg_t {
	unsigned long      ret_addr; // absolute
};
#endif // __x86_64__

typedef void (*probe_func_t)(probe_arg_t *t);

enum probe_type {
	PROBE_TYPE_OVERWRITE_JUMP,
	PROBE_TYPE_REPLACE_JUMP_ADDR,
};

class probe_info {
	probe_type m_type;
	string m_shared_lib_path;
	string m_symbol_name;
	unsigned long m_offset_addr;
	string m_probe_lib_path;
	probe_func_t m_probe;
	probe_func_t m_ret_probe;
public:
	probe_info(probe_type type);
	void set_target_address(const char *shared_lib_path, unsigned long addr);
	void set_probe(const char *probe_lib_path, probe_func_t probe);
	void set_ret_probe(probe_func_t probe);
};

#endif


