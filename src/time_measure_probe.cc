#include <cstdio>
using namespace std;

#include "time_measure_probe.h"

extern "C"
void roach_time_measure_probe(probe_arg_t *arg)
{
	printf("%s\n", __PRETTY_FUNCTION__);
}

extern "C"
void roach_time_mesure_ret_probe(probe_arg_t *arg)
{
	printf("%s\n", __PRETTY_FUNCTION__);
}
