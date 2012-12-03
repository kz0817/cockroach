#ifndef cockroach_probe_h
#define cockroach_probe_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

void cockroach_set_return_probe(probe_func_t probe, probe_arg_t *arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

