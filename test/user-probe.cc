#include <cockroach-probe.h>
#include "user-probe.h"

#define USER_DATA_ID 0x318

extern "C"
void user_probe_init(probe_init_arg_t *arg)
{
}

extern "C"
void user_probe(probe_arg_t *arg)
{
}

extern "C"
void data_recorder_init(probe_init_arg_t *arg)
{
	user_data *priv = new user_data();
	priv->id = USER_DATA_ID;
	priv->call_times = 0;
	arg->priv_data = priv;

}

extern "C"
void data_recorder(probe_arg_t *arg)
{
	user_data *priv = (user_data *)arg->priv_data;
	user_record_t record;
	record.call_times = priv->call_times++;
#ifdef __x86_64__
	record.arg0 = arg->rdi;
#endif // __x86_64__
#ifdef __i386__
	// TODO: get rsp
	//record.arg0 = *((unsigned long *)arg->esp);
	record.arg0 = 0;
#endif // __i386__
	cockroach_record_data_on_shm(priv->id, sizeof(user_record_t), &record);
}

