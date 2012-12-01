#include <cstdio>
using namespace std;

#include "time_measure_probe.h"

extern "C"
void roach_time_measure_probe(probe_arg_t *arg)
{
	printf("%s\n", __PRETTY_FUNCTION__);
	printf("arg: %p\n", arg);
	printf("arg->priv_data: %p\n", arg->priv_data);
	printf("arg->r15: %lx\n", arg->r15);
	printf("arg->probe_ret_addr: %lx\n", arg->probe_ret_addr);
	printf("arg->func_ret_addr: %lx\n", arg->func_ret_addr);
}

extern "C"
void roach_time_mesure_ret_probe(probe_arg_t *arg)
{
	printf("%s\n", __PRETTY_FUNCTION__);
}
