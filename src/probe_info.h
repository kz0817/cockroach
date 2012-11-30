#ifndef probe_info_h
#define probe_info_h

#include <string>
using namespace std;

#include <stdint.h>
#include "mapped_lib_info.h"

#ifdef __x86_64__

#define SIDE_CODE_AREA_LENGTH      100

// push %rax (1); mov $adrr,%rax (10); push %rax (1); ret (1);
#define OPCODES_LEN_OVERWRITE_JUMP 13

#define OPCODE_NOP        0x90
#define OPCODE_RET        0xc3
#define OPCODE_PUSH_RAX   0x50
#define OPCODE_MOVQ_0     0x48
#define OPCODE_MOVQ_1     0xb8

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
	string m_target_lib_path;
	string m_symbol_name;
	unsigned long m_offset_addr;
	int m_overwrite_length;
	string m_probe_lib_path;
	probe_func_t m_probe;
	probe_func_t m_ret_probe;

	// methods
	void change_page_permission(void *addr);
	void change_page_permission_all(void *addr, int len);
	void overwrite_jump_code(void *intrude_addr, void *jump_abs_addr,
	                         int copy_code_size);
public:
	probe_info(probe_type type);
	void set_target_address(const char *target_lib_path, unsigned long addr,
	                        int overwrite_length = 0);
	void set_probe(const char *probe_lib_path, probe_func_t probe);
	void set_ret_probe(probe_func_t probe);

	const char *get_target_lib_path(void);
	void install(const mapped_lib_info *lib_info);
};

#endif


