#include <cstdio>
using namespace std;

#include <sys/time.h>
#include "time_measure_probe.h"

struct time_measure_data {
	struct timeval t0;
	unsigned long func_ret_addr;
};

static double calc_diff_time(timeval *tv0, timeval *tv1)
{
	double t0 = tv0->tv_sec + tv0->tv_usec/1.0e6;
	double t1 = tv1->tv_sec + tv1->tv_usec/1.0e6;
	return t1 - t0;
}

static void roach_time_measure_ret_probe(probe_arg_t *arg)
{
	time_measure_data *data =
	   static_cast<time_measure_data*>(arg->priv_data);
	timeval t1;
	gettimeofday(&t1, NULL);
	double dt = calc_diff_time(&data->t0, &t1);
	printf("dT: %.3f (ms)\n", dt*1000);
}

extern "C"
void roach_time_measure_probe_init(probe_init_arg_t *arg)
{
	arg->priv_data = new time_measure_data();
}

extern "C"
void roach_time_measure_probe(probe_arg_t *arg)
{
	time_measure_data *data =
	  static_cast<time_measure_data*>(arg->priv_data);
	gettimeofday(&data->t0, NULL);
	data->func_ret_addr = arg->func_ret_addr;
	cockroach_set_return_probe(roach_time_measure_ret_probe, arg);
}

