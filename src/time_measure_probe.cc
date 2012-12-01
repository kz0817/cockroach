#include <cstdio>
using namespace std;

#include <sys/time.h>

#include "time_measure_probe.h"

struct time_measure_data {
	struct timeval t0;
	unsigned long func_ret_addr;
};

void roach_time_measure_ret_probe(probe_arg_t *arg)
{
	printf("%s\n", __PRETTY_FUNCTION__);
	time_measure_data *data =
	  static_cast<time_measure_data*>(arg->priv_data);
}

extern "C"
void roach_time_measure_probe_init(probe_init_arg_t *arg)
{
	printf("%s\n", __PRETTY_FUNCTION__);
	arg->priv_data = new time_measure_data();
}

extern "C"
void roach_time_measure_probe(probe_arg_t *arg)
{
	time_measure_data *data =
	  static_cast<time_measure_data*>(arg->priv_data);
	printf("%s\n", __PRETTY_FUNCTION__);
	printf("arg->priv_data: %p\n", arg->priv_data);
	printf("arg->func_ret_addr: %lx\n", arg->func_ret_addr);
	gettimeofday(&data->t0, NULL);
	data->func_ret_addr = arg->func_ret_addr;
	cockroach_set_return_probe(roach_time_measure_ret_probe, arg);
}

