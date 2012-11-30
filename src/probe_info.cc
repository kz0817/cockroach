#include <cstdio>
#include <cstdlib>
using namespace std;

#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"
#include "probe_info.h"
#include "side_code_area_manager.h"

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------
void probe_info::change_page_permission(void *addr)
{
	int page_size = utils::get_page_size();
	int prot =  PROT_READ | PROT_WRITE | PROT_EXEC;
	if (mprotect(addr, page_size, prot) == -1) {
		ROACH_ERR("Failed to mprotect: %p, %d, %x (%d)\n",
		          addr, page_size, prot, errno);
		abort();
	}
}

void probe_info::change_page_permission_all(void *addr, int len)
{
	int page_size = utils::get_page_size();
	unsigned long mask = ~(page_size - 1);
	unsigned long addr_ul = (unsigned long)addr;
	unsigned long addr_aligned = addr_ul & mask;
	change_page_permission((void *)addr_aligned);

	int mod = addr_ul % page_size;
	if (mod > page_size - len)
		change_page_permission((void *)(addr_aligned + page_size));
}

#ifdef __x86_64__
void probe_info::overwrite_jump_code(void *target_addr, void *jump_abs_addr,
                                     int copy_code_size)
{
	change_page_permission_all(target_addr, copy_code_size);

	// calculate the count of nop, which should be filled
	int idx;
	int len_nops = copy_code_size - OPCODES_LEN_OVERWRITE_JUMP;;
	if (len_nops < 0) {
		ROACH_ERR("len_nops is negaitve: %d, %d\n",
		          copy_code_size, OPCODES_LEN_OVERWRITE_JUMP);
		abort();
	}
	uint8_t *code = reinterpret_cast<uint8_t*>(target_addr);

	// push $rax
	*code = OPCODE_PUSH_RAX;
	code++;

	// mov $jump_abs_addr, $rax
	*code = OPCODE_MOVQ_0;
	code++;
	*code = OPCODE_MOVQ_1;
	code++;

	uint64_t jump_addr64 = reinterpret_cast<uint64_t>(jump_abs_addr);
	*(reinterpret_cast<uint64_t *>(code)) = jump_addr64;
	code += 8;

	// push %rax
	*code = OPCODE_PUSH_RAX;
	code++;

	// ret
	*code = OPCODE_RET;
	code++;

	// fill NOP instructions
	for (idx = 0; idx < len_nops; idx++, code++)
		*code = OPCODE_NOP;
}
#endif // __x86_64__

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
probe_info::probe_info(probe_type type)
: m_type(type),
  m_offset_addr(0),
  m_probe(NULL),
  m_ret_probe(NULL)
{
}

void probe_info::set_target_address(const char *target_lib_path, unsigned long addr, int overwrite_length)
{
	if (target_lib_path)
		m_target_lib_path = target_lib_path;
	else
		m_target_lib_path.erase();
	m_offset_addr = addr;
	m_overwrite_length = overwrite_length;
}

void probe_info::set_probe(const char *probe_lib_path, probe_func_t probe)
{
	if (probe_lib_path)
		m_probe_lib_path = probe_lib_path;
	else
		m_probe_lib_path.erase();
	m_probe =  probe;
}

void probe_info::set_ret_probe(probe_func_t probe)
{
	m_ret_probe = probe;
}

const char *probe_info::get_target_lib_path(void)
{
	return m_target_lib_path.c_str();
}

void probe_info::install(const mapped_lib_info *lib_info)
{
	printf("install: %s: %08lx, @ %016lx, %d\n",
	       lib_info->get_path(), m_offset_addr, lib_info->get_addr(),
	       m_overwrite_length);

	// --------------------------------------------------------------------
	// [Side Code Area Layout]
	// (1) save registers
	// (2) call probe
	// (3) restore registers
	// (4) original code
	// (5) return to original position
	// --------------------------------------------------------------------

	// check if the patch for the same address has already been registered.
	uint8_t *side_code_area
	  = side_code_area_manager::alloc(SIDE_CODE_AREA_LENGTH);
	/*
	uint8_t *intrude_addr = (uint8_t *)addr + m_dl_map_offset;
	patch_func_map_itr it = m_patch_func_map.find(intrude_addr);
	if (it != m_patch_func_map.end()) {
		EMPL_P(INFO, "Patch: %p is already registered\n",
		       intrude_addr);
		return;
	}
	m_patch_func_map[intrude_addr] = func; // regist this patch to the map

	// allocate the program area
	unsigned long bridge_templ_code_size;
	bridge_templ_code_size = CALC_LENGTH(bridge_begin, bridge_end);
	int side_code_size = save_code_length + bridge_templ_code_size;
	int side_data_size = sizeof(func);

	uint8_t *code_area = m_side_code_area_mgr.allocate(side_code_size);
	uint8_t *data_area = m_side_data_area_mgr.allocate(side_data_size,
	                                                   sizeof(long));

	// copy orignal code
	memcpy(code_area, intrude_addr, save_code_length);

	// copy bridge template
	uint8_t *bridge_area = code_area + save_code_length;
	memcpy(bridge_area, (void *)bridge_begin, bridge_templ_code_size);

	long index;
	unsigned long *ptr;

	// set return address
	index = CALC_LENGTH(bridge_begin, bridge_push_ret_addr);
	ptr = (unsigned long *)(bridge_area + index + LenPushOpCode);
	*ptr = (unsigned long)intrude_addr + save_code_length;

	// set pointer of member function pointer of the patch
	index = CALC_LENGTH(bridge_begin, bridge_push_patch_func_ptr_addr);
	ptr = (unsigned long *)(bridge_area + index + LenPushOpCode);
	*ptr = (unsigned long)data_area;
	memcpy((void *)data_area, &func, sizeof(func));

	// set object address
	index = CALC_LENGTH(bridge_begin, bridge_push_runner_addr);
	ptr = (unsigned long *)(bridge_area + index + LenPushOpCode);
	*ptr = (unsigned long)this;

	// set relative address to bridge_cbind()
	index = CALC_LENGTH(bridge_begin, bridge_call);
	ptr = (unsigned long *)(bridge_area + index + LenRelCallOpCode);
	long rel_addr_to_bridge_cbind =
	    CALC_LENGTH(bridge_area + index + LenRelCall, bridge_cbind);
	*ptr = rel_addr_to_bridge_cbind;
	*/

	// overwrite jump code
	unsigned long target_addr = lib_info->get_addr() +  m_offset_addr;
	void *target_addr_ptr = reinterpret_cast<void *>(target_addr);
	overwrite_jump_code(target_addr_ptr, 
	                    side_code_area,
	                    m_overwrite_length);
}
