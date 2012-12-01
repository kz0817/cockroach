#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
using namespace std;

#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

#include "utils.h"
#include "probe.h"
#include "side_code_area_manager.h"

#ifdef __x86_64__

#define PUSH_ALL_REGS() \
do { \
	asm volatile("pushf"); \
	asm volatile("push %rdi"); \
	asm volatile("push %rsi"); \
	asm volatile("push %rdx"); \
	asm volatile("push %rcx"); \
	asm volatile("push %rax"); \
	asm volatile("push %r8"); \
	asm volatile("push %r9"); \
	asm volatile("push %r10"); \
	asm volatile("push %r11"); \
	asm volatile("push %rbx"); \
	asm volatile("push %rbp"); \
	asm volatile("push %r12"); \
	asm volatile("push %r13"); \
	asm volatile("push %r14"); \
	asm volatile("push %r15"); \
} while(0)

#define POP_ALL_REGS() \
do { \
	asm volatile("pop %r15"); \
	asm volatile("pop %r14"); \
	asm volatile("pop %r13"); \
	asm volatile("pop %r12"); \
	asm volatile("pop %rbp"); \
	asm volatile("pop %rbx"); \
	asm volatile("pop %r11"); \
	asm volatile("pop %r10"); \
	asm volatile("pop %r9"); \
	asm volatile("pop %r8"); \
	asm volatile("pop %rax"); \
	asm volatile("pop %rcx"); \
	asm volatile("pop %rdx"); \
	asm volatile("pop %rsi"); \
	asm volatile("pop %rdi"); \
	asm volatile("popf"); \
} while(0)

#define PSEUDO_PUSH(label) \
do { \
	asm volatile(label); \
	asm volatile("sub $8,%rsp"); \
	asm volatile("movl $0x89abcdef,(%rsp)"); \
	asm volatile("movl $0x01234567,0x4(%rsp)"); \
} while(0)

void _bridge_template(void)
{
	asm volatile("bridge_begin:");
	asm volatile("pop %rax");

	// the return address of the probe
	PSEUDO_PUSH("bridge_set_post_probe_addr:");

	// save registers
	PUSH_ALL_REGS();

	// private data
	PSEUDO_PUSH("bridge_set_private_data:");

	// set an argument for the probe
	asm volatile("mov %rsp,%rdi");

	// push return address
	PSEUDO_PUSH("probe_call_set_ret_addr:");
	PSEUDO_PUSH("probe_call_set_probe_addr:");
	asm volatile("ret"); // use 'ret' instread of 'call'

	// restore stack
	asm volatile("probe_ret_point:");
	asm volatile("add $0x8,%rsp");

	// restore registers
	POP_ALL_REGS();

	// go to the saved orignal code (actually placed soon after here)
	// or the specifed address written in the probe.
	asm volatile("ret");
	asm volatile("bridge_end:");
}
extern "C" void bridge_begin(void);
extern "C" void bridge_set_post_probe_addr(void);
extern "C" void bridge_set_private_data(void);
extern "C" void probe_call_set_ret_addr(void);
extern "C" void probe_call_set_probe_addr(void);
extern "C" void probe_ret_point(void);
extern "C" void probe_call(void);
extern "C" void bridge_end(void);

void _resume_template(void)
{
	PSEUDO_PUSH("resume_begin:");
	asm volatile("ret"); // use 'ret' instread of 'call'
	asm volatile("resume_end:");
}
extern "C" void resume_begin(void);
extern "C" void resume_end(void);

#define OFFSET_BRIDGE(label) utils::calc_func_distance(bridge_begin, label);

void _ret_probe_bridge_template(void)
{
	asm volatile("ret_probe_bridge_begin:");
	PSEUDO_PUSH("ret_probe_return_addr:"); // original caller's point
	PUSH_ALL_REGS();
	PSEUDO_PUSH("ret_probe_set_private_data:");
	asm volatile("mov $0x0123456789abcdef,%rsi"); // 2nd argument
	asm volatile("mov %rsp,%rdi");              // 1st argument
	PSEUDO_PUSH("ret_probe_call_set_ret_addr:");
	PSEUDO_PUSH("ret_probe_call_set_dispatcher_addr:");
	asm volatile("ret"); // use 'ret' instread of 'call'
	asm volatile("ret_probe_bridge_end:");
}
extern "C" void ret_probe_bridge_begin(void);
extern "C" void ret_probe_return_addr(void);
extern "C" void ret_probe_set_private_data(void);
extern "C" void ret_probe_call_set_ret_addr(void);
extern "C" void ret_probe_call_set_dispatcher_addr(void);
extern "C" void ret_probe_bridge_end(void);

void _return_ret_probe_bridge(void)
{
	// NOTE: function is not template, it is actually used as it is.
	asm volatile("return_ret_probe_bridge:");
	asm volatile("add $0x8,%rsp");
	POP_ALL_REGS();
	asm volatile("ret");
}
extern "C" void return_ret_probe_bridge(void);

#endif // __x86_64__

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------
void probe::change_page_permission(void *addr)
{
	int page_size = utils::get_page_size();
	int prot =  PROT_READ | PROT_WRITE | PROT_EXEC;
	if (mprotect(addr, page_size, prot) == -1) {
		ROACH_ERR("Failed to mprotect: %p, %d, %x (%d)\n",
		          addr, page_size, prot, errno);
		abort();
	}
}

void probe::change_page_permission_all(void *addr, int len)
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
void probe::overwrite_jump_code(void *target_addr, void *jump_abs_addr,
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

	// jmp *%rax
	*code = OPCODE_JMP_ABS_RAX_0;
	code++;
	*code = OPCODE_JMP_ABS_RAX_1;
	code++;

	// fill NOP instructions
	for (idx = 0; idx < len_nops; idx++, code++)
		*code = OPCODE_NOP;
}

void probe::set_pseudo_push_parameter(uint8_t *code_addr,
                                           unsigned long param)
{
	// We assume the pseudo push as the followin form.
	//   sub  $8,%rsp
	//   movl $0x89abcdef,(%rsp)
	//   movl $0x01234567,0x4(%rsp)
	// *** assembler code ***
	//   48 83 ec 08               sub    $0x8,%rsp
	//   c7 04 24 ef cd ab 89      movl   $0x89abcdef,(%rsp)
	//   c7 44 24 04 67 45 23 01   movl   $0x01234567,0x4(%rsp)
	static const int OFFSET_ADDR_LSB32 = 7;
	static const int OFFSET_ADDR_MSB32 = 15;
	uint32_t param_lsb32 = param & 0xffffffff;
	uint32_t param_msb32 = param >> 32;
	uint32_t *code_addr_lsb32 = (uint32_t *)(code_addr + OFFSET_ADDR_LSB32);
	uint32_t *code_addr_msb32 = (uint32_t *)(code_addr + OFFSET_ADDR_MSB32);
	*code_addr_lsb32 = param_lsb32;
	*code_addr_msb32 = param_msb32;
}
#endif // __x86_64__

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
probe::probe(probe_type type)
: m_type(type),
  m_offset_addr(0),
  m_probe_init(NULL),
  m_probe(NULL),
  m_probe_priv_data(NULL)
{
}

void probe::set_target_address(const char *target_lib_path, unsigned long addr, int overwrite_length)
{
	if (target_lib_path)
		m_target_lib_path = target_lib_path;
	else
		m_target_lib_path.erase();
	m_offset_addr = addr;
	m_overwrite_length = overwrite_length;
}

void probe::set_probe(const char *probe_lib_path, probe_func_t probe,
                           probe_init_func_t probe_init)
{
	if (probe_lib_path)
		m_probe_lib_path = probe_lib_path;
	else
		m_probe_lib_path.erase();
	m_probe =  probe;
	m_probe_init =  probe_init;
}

const char *probe::get_target_lib_path(void)
{
	return m_target_lib_path.c_str();
}

void probe::install(const mapped_lib_info *lib_info)
{
	printf("install: %s: %08lx, @ %016lx, %d\n",
	       lib_info->get_path(), m_offset_addr, lib_info->get_addr(),
	       m_overwrite_length);
	
	// run the probe initializer that creates private data if needed.
	probe_init_arg_t arg;
	(*m_probe_init)(&arg);
	m_probe_priv_data = arg.priv_data;

	// --------------------------------------------------------------------
	// [Side Code Area Layout]
	// (0) restore rax that is used to jump to here
	// (1) code to save registers
	// (2) code to call probe
	// (3) code to restore registers
	// (4) original code
	// (6) code to resume the original code
	// --------------------------------------------------------------------
	unsigned long target_addr = lib_info->get_addr() +  m_offset_addr;
	void *target_addr_ptr = (void *)target_addr;

	// check if the patch for the same address has already been registered.
	static const int BRIDGE_LENGTH =
	  utils::calc_func_distance(bridge_begin, bridge_end);
	static const int RET_BRIDGE_LENGTH =
	  utils::calc_func_distance(resume_begin, resume_end);
	int code_len = BRIDGE_LENGTH + m_overwrite_length + RET_BRIDGE_LENGTH;
	uint8_t *side_code_area = side_code_area_manager::alloc(code_len);

	// copy bridge code, orignal code and resume code
	uint8_t *side_code_ptr = side_code_area;
	memcpy(side_code_ptr, (void *)bridge_begin, BRIDGE_LENGTH);
	side_code_ptr += BRIDGE_LENGTH;
	memcpy(side_code_ptr, target_addr_ptr, m_overwrite_length);
	side_code_ptr += m_overwrite_length;
	memcpy(side_code_ptr, (void *)resume_begin, RET_BRIDGE_LENGTH);

	// set the address to be executed after the probe is returned.
	// By default, we set to execute the saved orignal code.
	// The probe can changed the address by set probe_arg_t::probe_ret_addr.
	side_code_ptr =
	  side_code_area + OFFSET_BRIDGE(bridge_set_post_probe_addr);
	uint8_t *saved_orig_code = side_code_area + OFFSET_BRIDGE(bridge_end);
	set_pseudo_push_parameter(side_code_ptr, (unsigned long)saved_orig_code);

	// set probe return address
	side_code_ptr = side_code_area + OFFSET_BRIDGE(probe_call_set_ret_addr);
	uint8_t *ret_addr = side_code_area + OFFSET_BRIDGE(probe_ret_point);
	set_pseudo_push_parameter(side_code_ptr, (unsigned long)ret_addr);

	// set probe private address
	side_code_ptr = side_code_area + OFFSET_BRIDGE(bridge_set_private_data);
	set_pseudo_push_parameter(side_code_ptr,
	                          (unsigned long)m_probe_priv_data);

	// set probe address
	side_code_ptr =
	  side_code_area + OFFSET_BRIDGE(probe_call_set_probe_addr);
	set_pseudo_push_parameter(side_code_ptr, (unsigned long)m_probe);

	// set address to the original path
	side_code_ptr = side_code_area + BRIDGE_LENGTH + m_overwrite_length;
	uint8_t *dest_addr = (uint8_t *)target_addr_ptr + m_overwrite_length;
	set_pseudo_push_parameter(side_code_ptr, (unsigned long)dest_addr);

	// overwrite jump code
	overwrite_jump_code(target_addr_ptr, 
	                    side_code_area, m_overwrite_length);
}

// ---------------------------------------------------------------------------
// functions for a return probe 
// ---------------------------------------------------------------------------
static queue<uint8_t *> g_ret_probe_bridge_queue;
static uint8_t *get_ret_probe_bridge(void)
{
	// TODO: should be MT
	if (g_ret_probe_bridge_queue.empty())
		return NULL;
	uint8_t *ret = g_ret_probe_bridge_queue.front();
	g_ret_probe_bridge_queue.pop();
	return ret;
}

static uint8_t *create_ret_probe_bridge(void)
{
	return NULL;
}

void cockroach_set_return_probe(probe_func_t probe, probe_arg_t *arg)
{
	printf("%s: probe: %p\n", __PRETTY_FUNCTION__, probe);
	// fist, try to retrive from the pool
	uint8_t *ret_probe_bridge = get_ret_probe_bridge();
	if (!ret_probe_bridge)
		ret_probe_bridge = create_ret_probe_bridge();
}
