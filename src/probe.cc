#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <list>
using namespace std;

#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "utils.h"
#include "probe.h"
#include "side_code_area_manager.h"
#include "disassembler.h"
#include "opecode_relocator.h"

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
	// restore RAX that is changed to jump to here.
	// The original RAX is pushed before it is changed.
	// see. overwrite_jump_abs64().
	asm volatile("pop %rax");

	// When install type is REL32, the bridge code begins from here
	asm volatile("bridge_begin_no_pop_ax:");

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
extern "C" void bridge_begin_no_pop_ax(void);
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

#define OFFSET_BRIDGE(label) \
utils::calc_func_distance(get_bridge_begin_addr(), label)

void _ret_probe_bridge_template(void)
{
	asm volatile("ret_probe_bridge_begin:");

	// set original caller's return point
	PSEUDO_PUSH("ret_probe_return_addr:");

	// save registers
	PUSH_ALL_REGS();

	// set private data
	PSEUDO_PUSH("ret_probe_set_private_data:");

	// 2nd argument: address of the return probe bridge block
	// The address is used as an identifier.
	asm volatile("ret_probe_set_bridge_addr:");
	asm volatile("mov $0x0123456789abcdef,%rsi");

	// 1st argument: probe_arg_t *arg
	asm volatile("mov %rsp,%rdi");

	// set the return address for the dispacher (return_ret_probe_bridge)
	PSEUDO_PUSH("ret_probe_call_set_ret_addr:");

	// call the dispacher
	PSEUDO_PUSH("ret_probe_call_set_dispatcher_addr:");
	asm volatile("ret"); // use 'ret' instread of 'call'
	asm volatile("ret_probe_bridge_end:");
}
extern "C" void ret_probe_bridge_begin(void);
extern "C" void ret_probe_return_addr(void);
extern "C" void ret_probe_set_private_data(void);
extern "C" void ret_probe_set_bridge_addr(void);
extern "C" void ret_probe_call_set_ret_addr(void);
extern "C" void ret_probe_call_set_dispatcher_addr(void);
extern "C" void ret_probe_bridge_end(void);

#define OFFSET_RET_BRIDGE(label) \
utils::calc_func_distance(ret_probe_bridge_begin, label)

void _return_ret_probe_bridge(void)
{
	// NOTE: function is not template, it is actually used as it is.
	asm volatile("return_ret_probe_bridge:");
	asm volatile("add $0x8,%rsp");
	POP_ALL_REGS();
	asm volatile("ret");
}
extern "C" void return_ret_probe_bridge(void);

static void set_pseudo_push_parameter(uint8_t *code_addr, unsigned long param)
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

unsigned long cockroach_get_target_func_arg(probe_arg_t *arg, size_t nth_arg)
{
	if (nth_arg == 0) {
		ROACH_ERR("nth_arg 0: is invalid\n");
		return 0;
	}

	if (nth_arg == 1)
		return arg->rdi;
	else if (nth_arg == 2)
		return arg->rsi;
	else if (nth_arg == 3)
		return arg->rdx;
	else if (nth_arg == 4)
		return arg->rcx;
	else if (nth_arg == 5)
		return arg->r8;
	else if (nth_arg == 6)
		return arg->r9;

	unsigned long *caller_stack =
	   cockroach_get_stack_addr_of_target_caller(arg);
	return caller_stack[nth_arg - 7];
}

#endif // __x86_64__

#ifdef __i386__
#define PUSH_ALL_REGS() \
do { \
	asm volatile("pushf"); \
	asm volatile("push %eax"); \
	asm volatile("push %ebp"); \
	asm volatile("push %edi"); \
	asm volatile("push %esi"); \
	asm volatile("push %edx"); \
	asm volatile("push %ecx"); \
	asm volatile("push %ebx"); \
} while(0)

#define POP_ALL_REGS() \
do { \
	asm volatile("pop %ebx"); \
	asm volatile("pop %ecx"); \
	asm volatile("pop %edx"); \
	asm volatile("pop %esi"); \
	asm volatile("pop %edi"); \
	asm volatile("pop %ebp"); \
	asm volatile("pop %eax"); \
	asm volatile("popf"); \
} while(0)

#define PSEUDO_PUSH(label) \
do { \
	asm volatile(label); \
	asm volatile("push $0x89abcdef"); \
} while(0)

void _bridge_template(void)
{
	// This will not be needed, because ABS64_JUMP shall not be used
	// on i386.
	asm volatile("bridge_begin:");
	asm volatile("pop %eax");

	// When install type is REL32, the bridge code begins from here
	asm volatile("bridge_begin_no_pop_ax:");

	// the return address of the probe
	PSEUDO_PUSH("bridge_set_post_probe_addr:");

	// save registers
	PUSH_ALL_REGS();

	// private data
	PSEUDO_PUSH("bridge_set_private_data:");

	// set an argument for the probe
	asm volatile("push %esp");

	// push return address
	PSEUDO_PUSH("probe_call_set_ret_addr:");
	PSEUDO_PUSH("probe_call_set_probe_addr:");
	asm volatile("ret"); // use 'ret' instread of 'call'

	// restore stack
	asm volatile("probe_ret_point:");
	asm volatile("add $0x8,%esp"); // 8: argument(4) + private_data(4)

	// restore registers
	POP_ALL_REGS();

	// go to the saved orignal code (actually placed soon after here)
	// or the specifed address written in the probe.
	asm volatile("ret");
	asm volatile("bridge_end:");
}
extern "C" void bridge_begin(void);
extern "C" void bridge_begin_no_pop_ax(void);
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

#define OFFSET_BRIDGE(label) \
utils::calc_func_distance(get_bridge_begin_addr(), label)

void _ret_probe_bridge_template(void)
{
	asm volatile("ret_probe_bridge_begin:");

	// set original caller's return point
	PSEUDO_PUSH("ret_probe_return_addr:");

	// save registers
	PUSH_ALL_REGS();

	// set private data
	PSEUDO_PUSH("ret_probe_set_private_data:");
	asm volatile("mov %esp,%eax"); // save the ESP for the 1st argument

	// 2nd argument: address of the return probe bridge block
	// The address is used as an identifier.
	PSEUDO_PUSH("ret_probe_set_bridge_addr:");

	// 1st argument: probe_arg_t *arg
	asm volatile("push %eax");

	// set the return address for the dispacher (return_ret_probe_bridge)
	PSEUDO_PUSH("ret_probe_call_set_ret_addr:");

	// call the dispacher
	PSEUDO_PUSH("ret_probe_call_set_dispatcher_addr:");
	asm volatile("ret"); // use 'ret' instread of 'call'
	asm volatile("ret_probe_bridge_end:");
}
extern "C" void ret_probe_bridge_begin(void);
extern "C" void ret_probe_return_addr(void);
extern "C" void ret_probe_set_private_data(void);
extern "C" void ret_probe_set_bridge_addr(void);
extern "C" void ret_probe_call_set_ret_addr(void);
extern "C" void ret_probe_call_set_dispatcher_addr(void);
extern "C" void ret_probe_bridge_end(void);

#define OFFSET_RET_BRIDGE(label) \
utils::calc_func_distance(ret_probe_bridge_begin, label)

void _return_ret_probe_bridge(void)
{
	// NOTE: function is not template, it is actually used as it is.
	asm volatile("return_ret_probe_bridge:");
	asm volatile("add $0xc,%esp"); // c: 1st, 2nd argument, and priv data
	POP_ALL_REGS();
	asm volatile("ret");
}
extern "C" void return_ret_probe_bridge(void);

static void set_pseudo_push_parameter(uint8_t *code_addr, unsigned long param)
{
	// We assume the pseudo push as the followin form.
	//   68 ef cd ab 89          push   $0x89abcdef
	static const int OFFSET = 1;
	uint32_t *code_addr32 = (uint32_t *)(code_addr + OFFSET);
	*code_addr32 = param;
}

unsigned long cockroach_get_target_func_arg(probe_arg_t *arg, size_t nth_arg)
{
	if (nth_arg == 0) {
		ROACH_ERR("nth_arg 0: is invalid\n");
		return 0;
	}
	unsigned long *caller_stack = 
	   cockroach_get_stack_addr_of_target_caller(arg);
	return caller_stack[nth_arg - 1];
}

#endif // __i386__

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
		ROACH_ABORT();
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

#if defined(__x86_64__) || defined(__i386__)
void probe::overwrite_jump_code(void *target_addr, void *jump_abs_addr,
                                int copy_code_size)
{
	change_page_permission_all(target_addr, copy_code_size);

	// calculate the count of nop, which should be filled
	int idx;
	int len_nops = copy_code_size - probe::get_overwrite_code_length();
	if (len_nops < 0) {
		ROACH_ERR("len_nops is zero or negaitve: %d (%d)\n",
		          copy_code_size, OPCODES_LEN_OVERWRITE_JUMP);
		ROACH_ABORT();
	}
	uint8_t *code = reinterpret_cast<uint8_t*>(target_addr);

	// fill jump instruction(s)
	if (m_install_type == INSTALL_TYPE_OVERWRITE_ABS64_JUMP) {
		code = overwrite_jump_abs64(code, jump_abs_addr,
		                            copy_code_size);
	} else if (m_install_type == INSTALL_TYPE_OVERWRITE_REL32_JUMP) {
		code = overwrite_jump_rel32(code, jump_abs_addr,
		                            copy_code_size);
	} else {
		ROACH_BUG("Unknown m_install_type: %d\n", m_install_type);
	}

	// fill NOP instructions
	for (idx = 0; idx < len_nops; idx++, code++)
		*code = OPCODE_NOP;
}

uint8_t *probe::overwrite_jump_rel32(uint8_t *code, void *jump_abs_addr,
                                     int copy_code_size)
{
	int32_t rel_addr = get_rel_addr32_for_jump(code, jump_abs_addr);

	// fill jump instruction
	*code = OPCODE_JMP_REL;
	code++;

	// fill address
	*((uint32_t *)code) = (uint32_t)rel_addr;
	code += sizeof(uint32_t);

	return code;
}

uint8_t *probe::overwrite_jump_abs64(uint8_t *code, void *jump_abs_addr,
                                     int copy_code_size)
{
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

	return code;
}

label_func_t probe::get_bridge_begin_addr(void)
{
	if (m_install_type == INSTALL_TYPE_OVERWRITE_ABS64_JUMP)
		return bridge_begin;
	else if (m_install_type == INSTALL_TYPE_OVERWRITE_REL32_JUMP)
		return bridge_begin_no_pop_ax;
	ROACH_BUG("Unknown install type: %d\n", m_install_type);
	return NULL;
}

int probe::get_overwrite_code_length(void)
{
	if (m_install_type == INSTALL_TYPE_OVERWRITE_ABS64_JUMP)
		return OPCODES_LEN_OVERWRITE_JUMP;
	else if (m_install_type == INSTALL_TYPE_OVERWRITE_REL32_JUMP)
		return LEN_OPCODE_JMP_REL32;
	ROACH_BUG("Unknown install type: %d\n", m_install_type);
	return -1;
}


#endif // defined(__x86_64__) || defined(__i386__)

int probe::get_minimum_overwrite_length(void)
{
	if (m_install_type == INSTALL_TYPE_OVERWRITE_ABS64_JUMP)
		return OPCODES_LEN_OVERWRITE_JUMP;
	else if (m_install_type == INSTALL_TYPE_OVERWRITE_REL32_JUMP)
		return LEN_OPCODE_JMP_REL32;
	ROACH_BUG("Unknown install type: %d\n", m_install_type);
	return -1;
}

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
probe::probe(probe_type_t probe_type, install_type_t install_type)
: m_probe_type(probe_type),
  m_install_type(install_type),
  m_offset_addr(0),
  m_overwrite_length(0),
  m_overwrite_length_auto_detect(false),
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
	if (m_overwrite_length == 0)
		m_overwrite_length_auto_detect = true;
}

void probe::set_probe(const char *probe_lib_path, probe_func_t probe,
                           probe_init_func_t probe_init)
{
	if (probe_lib_path)
		m_probe_lib_path = probe_lib_path;
	else
		m_probe_lib_path.erase();
	m_probe = probe;
	m_probe_init = probe_init;
}

const char *probe::get_target_lib_path(void)
{
	return m_target_lib_path.c_str();
}

#if defined(__x86_64__) || defined(__i386__)
void probe::install(const mapped_lib_info *lib_info)
{
	ROACH_INFO("install: %s: func: %08lx, mapped addr: %016lx, "
	           "overwrite len: %d, install: %d\n",
	           lib_info->get_path(), m_offset_addr, lib_info->get_addr(),
	           m_overwrite_length, m_install_type);
	unsigned long target_addr = m_offset_addr;
	if (!lib_info->is_exe())
		target_addr += lib_info->get_addr();
	install_core(target_addr);
}

void probe::install(void *mapped_addr)
{
	ROACH_INFO("install: func: %08lx, mapped addr: %016lx, "
	           "overwrite len: %d, install: %d\n",
	           m_offset_addr, mapped_addr,
	           m_overwrite_length, m_install_type);
	unsigned long target_addr = (unsigned long)mapped_addr + m_offset_addr;
	install_core(target_addr);
}

void probe::install_core(unsigned long target_addr)
{
	void *target_addr_ptr = (void *)target_addr;

	// detect overwrite length if needed
	list<opecode *> relocated_opecode_list;
	if (m_overwrite_length_auto_detect) {
		uint8_t *code_ptr = (uint8_t *)target_addr;
		int parsed_length = 0;
		bool met_ret_code = false;
		while (parsed_length < get_minimum_overwrite_length()) {
			if (met_ret_code) {
				ROACH_ERR("Met return code @ %d\n.",
				          parsed_length);
				ROACH_ABORT();
			}

			opecode *op = disassembler::parse(code_ptr);
			parsed_length += op->get_length();
			code_ptr += op->get_length();
			relocated_opecode_list.push_back(op);

			if (is_opecode_ret(op))
				met_ret_code = true;
		}
		m_overwrite_length = parsed_length;
	}

	int relocated_code_length = 0;
	int relocated_data_length = 0;
	if (!relocated_opecode_list.empty()) {
		opecode_relocator *relocator;
		list<opecode *>::iterator op = relocated_opecode_list.begin();
		for (; op != relocated_opecode_list.end(); ++op) {
			relocator = (*op)->get_relocator();
			relocated_code_length +=
			  relocator->get_max_code_length();
			relocated_data_length +=
			  relocator->get_max_data_length();
		}
	}
	else
		relocated_code_length = m_overwrite_length;

	// run the probe initializer that creates private data if needed.
	probe_init_arg_t arg;
	arg.target_addr = target_addr;
	if (m_probe_init)
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

	// check if the patch for the same address has already been registered.
	label_func_t bridge_begin_addr = get_bridge_begin_addr();
	int bridge_length =
	  utils::calc_func_distance(bridge_begin_addr, bridge_end);
	static const int RET_BRIDGE_LENGTH =
	  utils::calc_func_distance(resume_begin, resume_end);
	int code_len = bridge_length
	               + relocated_code_length + relocated_data_length
	               + RET_BRIDGE_LENGTH;
	uint8_t *side_code_area = NULL;
	if (m_install_type == INSTALL_TYPE_OVERWRITE_REL32_JUMP) {
		unsigned long next_code_addr
		  = target_addr + relocated_code_length;
		side_code_area =
		  side_code_area_manager::alloc_within_rel32(code_len,
		                                             next_code_addr);
	} else
		side_code_area = side_code_area_manager::alloc(code_len);
	ROACH_DBG("side_code: %p\n", side_code_area);

	// copy bridge code, orignal code and resume code
	uint8_t *side_code_ptr = side_code_area;
	memcpy(side_code_ptr, (void *)bridge_begin_addr, bridge_length);
	side_code_ptr += bridge_length;

	// code relocation or simple copy
	if (!relocated_opecode_list.empty()) {
		opecode_relocator *relocator;
		list<opecode *>::iterator op = relocated_opecode_list.begin();
		for (; op != relocated_opecode_list.end(); ++op) {
			relocator = (*op)->get_relocator();
			side_code_ptr += relocator->relocate(side_code_ptr);
		}
	} else {
		memcpy(side_code_ptr, target_addr_ptr, m_overwrite_length);
		side_code_ptr += relocated_code_length;
	}

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
	side_code_ptr = side_code_area + bridge_length + relocated_code_length;
	uint8_t *dest_addr = (uint8_t *)target_addr_ptr + m_overwrite_length;
	set_pseudo_push_parameter(side_code_ptr, (unsigned long)dest_addr);

	// overwrite jump code
	overwrite_jump_code(target_addr_ptr, 
	                    side_code_area, m_overwrite_length);
}
bool probe::is_opecode_ret(const opecode *ope) const
{
	if (ope->get_length() != 1)
		return false;
	return *ope->get_code() == OPCODE_RET;
}

#endif // defined(__x86_64__) || defined(__i386__)

// ---------------------------------------------------------------------------
// functions for a return probe 
// ---------------------------------------------------------------------------
typedef map<uint8_t *, probe_func_t> ret_probe_func_map_t;
typedef ret_probe_func_map_t::iterator ret_probe_func_map_itr;

typedef queue<uint8_t *> ret_probe_bridge_queue_t;

static pthread_mutex_t
  g_ret_probe_bridge_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t
  g_ret_probe_func_map_mutex = PTHREAD_MUTEX_INITIALIZER;

//
// The following two functions may be called before main().
// They avoid static variable from being used without initialization.
// i.e. The instance as a simple global variable may cause a problem
//      when it is used before main().
//
static ret_probe_func_map_t &get_ret_probe_func_map(void)
{
	static ret_probe_func_map_t ret_probe_func_map;
	return ret_probe_func_map;
}

static ret_probe_bridge_queue_t &get_ret_probe_bridge_queue(void)
{
	static ret_probe_bridge_queue_t ret_probe_bridge_queue;
	return ret_probe_bridge_queue;
}

static uint8_t *get_ret_probe_bridge(void)
{
	uint8_t *ret = NULL;
	ret_probe_bridge_queue_t &ret_probe_bridge_queue
	  = get_ret_probe_bridge_queue();
	pthread_mutex_lock(&g_ret_probe_bridge_queue_mutex);
	if (!ret_probe_bridge_queue.empty()) {
		ret = ret_probe_bridge_queue.front();
		ret_probe_bridge_queue.pop();
	}
	pthread_mutex_unlock(&g_ret_probe_bridge_queue_mutex);
	return ret;
}

static void release_ret_probe_bridge(uint8_t *ret_probe_bridge)
{
	pthread_mutex_lock(&g_ret_probe_bridge_queue_mutex);
	get_ret_probe_bridge_queue().push(ret_probe_bridge);
	pthread_mutex_unlock(&g_ret_probe_bridge_queue_mutex);
}

static void ret_probe_dispatcher(probe_arg_t *arg,
                                 uint8_t *ret_probe_bridge)
{
	ret_probe_func_map_itr it;
	ret_probe_func_map_t &ret_probe_func_map = get_ret_probe_func_map();
	pthread_mutex_lock(&g_ret_probe_func_map_mutex);
	it = ret_probe_func_map.find(ret_probe_bridge);
	if (it == ret_probe_func_map.end()) {
		ROACH_ERR("Not found : arg: %p, ret_probe_bridge: %p\n",
		          arg, ret_probe_bridge);
		ROACH_ABORT();
	}
	probe_func_t probe = it->second;
	ret_probe_func_map.erase(it);
	pthread_mutex_unlock(&g_ret_probe_func_map_mutex);

	// execute the return probe
	(*probe)(arg);

	// release ret_probe_bridge
	release_ret_probe_bridge(ret_probe_bridge);
}

#if defined(__x86_64__) || defined(__i386__)
static uint8_t *create_ret_probe_bridge(void)
{
	static const int RET_PROBE_BRIDGE_LENGTH =
	  utils::calc_func_distance(ret_probe_bridge_begin,
	                            ret_probe_bridge_end);
	uint8_t *side_code_area =
	  side_code_area_manager::alloc(RET_PROBE_BRIDGE_LENGTH);

	// copy return probe bridge code
	uint8_t *side_code_ptr = side_code_area;
	memcpy(side_code_ptr, (void *)ret_probe_bridge_begin,
	       RET_PROBE_BRIDGE_LENGTH);

	// set the head address of this area (2nd arguemnt of dispatcher)
#if __x86_64__
	//   48 be ef cd ab 89 67 45 23 01 movabs $0x123456789abcdef,%rsi
	side_code_ptr = 
	  side_code_area + OFFSET_RET_BRIDGE(ret_probe_set_bridge_addr) + 2;
	*((unsigned long *)side_code_ptr) = (unsigned long)side_code_area;
#endif // __x86_64__
#if __i386__
	side_code_ptr =
	  side_code_area + OFFSET_RET_BRIDGE(ret_probe_set_bridge_addr);
	set_pseudo_push_parameter(side_code_ptr,
	                          (unsigned long)side_code_area);
#endif // __i386__

	// set the address to the 'retrun_ret_probe_bridge'
	side_code_ptr =
	  side_code_area + OFFSET_RET_BRIDGE(ret_probe_call_set_ret_addr);
	set_pseudo_push_parameter(side_code_ptr,
	                          (unsigned long)return_ret_probe_bridge);

	// set the address of the dispatcher
	side_code_ptr = side_code_area +
	                OFFSET_RET_BRIDGE(ret_probe_call_set_dispatcher_addr);
	set_pseudo_push_parameter(side_code_ptr,
	                          (unsigned long)ret_probe_dispatcher);

	return side_code_area;
}

void cockroach_set_return_probe(probe_func_t probe, probe_arg_t *arg)
{
	// fist, try to retrive from the pool
	uint8_t *ret_probe_bridge = get_ret_probe_bridge();
	if (!ret_probe_bridge)
		ret_probe_bridge = create_ret_probe_bridge();

	// set the original caller's return point address
	uint8_t *side_code_ptr =
	  ret_probe_bridge + OFFSET_RET_BRIDGE(ret_probe_return_addr);
	set_pseudo_push_parameter(side_code_ptr, arg->func_ret_addr);

	// set the private data
	side_code_ptr =
	  ret_probe_bridge + OFFSET_RET_BRIDGE(ret_probe_set_private_data);
	set_pseudo_push_parameter(side_code_ptr, (unsigned long)arg->priv_data);

	// overwrite the return address to the orignal function
	arg->func_ret_addr = (unsigned long)ret_probe_bridge;

	// regist the return probe information to the map
	ret_probe_func_map_itr it;
	ret_probe_func_map_t &ret_probe_func_map = get_ret_probe_func_map();
	pthread_mutex_lock(&g_ret_probe_func_map_mutex);
	it = ret_probe_func_map.find(ret_probe_bridge);
	if (it != ret_probe_func_map.end()) {
		ROACH_ERR("Already registered: ret_probe_bridge: %p\n",
		          ret_probe_bridge);
		ROACH_ABORT();
	}
	ret_probe_func_map[ret_probe_bridge] = probe;
	pthread_mutex_unlock(&g_ret_probe_func_map_mutex);
}

unsigned long *cockroach_get_stack_addr_of_target_caller(probe_arg_t *arg)
{
	return (unsigned long *)(arg + 1);
}

#endif // defined(__x86_64__) || defined(__i386__)

#if __x86_64__
int32_t probe::get_rel_addr32_for_jump(void *curr, void *dest)
{
	// calculate relative address
	int64_t rel_addr =
	   (uint64_t)dest - (uint64_t)((uint8_t *)curr + LEN_OPCODE_JMP_REL32);
	if (rel_addr > INT32_MAX || rel_addr < INT32_MIN) {
		ROACH_ERR("Invalid: rel_addr: %"PRIx64"\n", rel_addr);
		ROACH_ABORT();
	}
	return (int32_t)rel_addr;
}
#endif // __x86_64__

#if __i386__
int32_t probe::get_rel_addr32_for_jump(void *curr, void *dest)
{
	return (int32_t)dest -
	       (uint32_t)((uint8_t *)curr + LEN_OPCODE_JMP_REL32);
}
#endif // __i386__

