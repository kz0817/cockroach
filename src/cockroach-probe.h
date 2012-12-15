#ifndef cockroach_probe_h
#define cockroach_probe_h

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define COCKROACH_RECORD_ID_STRING 0xffff0000
#define COCKROACH_RECORD_ID_RESERVED_BEGIN COCKROACH_RECORD_ID_STRING

#ifdef __x86_64__
struct probe_arg_t {
	void *priv_data;
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long rbp;
	unsigned long rbx;
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rax;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long flags;

	unsigned long probe_ret_addr;

	// Not valid in a return probe
	unsigned long func_ret_addr;
};
#endif // __x86_64__

struct probe_init_arg_t {
	unsigned long target_addr;
	void *priv_data; // set in init probe if needed.
};

typedef void (*probe_init_func_t)(probe_init_arg_t *t);
typedef void (*probe_func_t)(probe_arg_t *t);

typedef void (*record_data_func_t)(size_t size, void *buf, void *priv);

void cockroach_set_return_probe(probe_func_t probe, probe_arg_t *arg);
void cockroach_record_data_on_shm(uint32_t id, size_t size, void *data);
void
cockroach_record_data_on_shm_with_func(uint32_t id, size_t size,
                                       record_data_func_t record_data_func,
                                       void *priv);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

