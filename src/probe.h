#ifndef probe_h
#define probe_h

#include <string>
using namespace std;

#include <stdint.h>
#include "cockroach-probe.h"
#include "mapped_lib_info.h"

#ifdef __x86_64__
// push %rax (1); mov $adrr,%rax (10); push *%rax (2);
#define OPCODES_LEN_OVERWRITE_JUMP 13

#define OPCODE_NOP        0x90
#define OPCODE_RET        0xc3
#define OPCODE_PUSH_RAX   0x50
#define OPCODE_MOVQ_0     0x48
#define OPCODE_MOVQ_1     0xb8
#define OPCODE_JMP_ABS_RAX_0 0xff
#define OPCODE_JMP_ABS_RAX_1 0xe0
#endif // __x86_64__

enum probe_type {
	PROBE_TYPE_OVERWRITE_JUMP,
	PROBE_TYPE_REPLACE_JUMP_ADDR,
};

class probe {
	probe_type m_type;
	string m_target_lib_path;
	string m_symbol_name;
	unsigned long m_offset_addr;
	int m_overwrite_length;

	string            m_probe_lib_path;
	probe_init_func_t m_probe_init;
	probe_func_t      m_probe;
	void             *m_probe_priv_data;

	// methods
	void change_page_permission(void *addr);
	void change_page_permission_all(void *addr, int len);
	void overwrite_jump_code(void *intrude_addr, void *jump_abs_addr,
	                         int copy_code_size);
	void set_pseudo_push_parameter(uint8_t *code_addr, unsigned long param);
public:
	probe(probe_type type);
	void set_target_address(const char *target_lib_path, unsigned long addr,
	                        int overwrite_length = 0);
	void set_probe(const char *probe_lib_path, probe_func_t probe,
	               probe_init_func_t probe_init = NULL);

	const char *get_target_lib_path(void);
	void install(const mapped_lib_info *lib_info);
};

#endif


