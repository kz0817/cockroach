#ifndef time_measure_probe_h
#define time_measure_probe_h

#include "probe_info.h"

extern "C"
void roach_time_measure_probe_init(probe_init_arg_t *arg);

extern "C"
void roach_time_measure_probe(probe_arg_t *arg);

extern "C"
void roach_time_mesure_ret_probe(probe_arg_t *arg);

#endif
