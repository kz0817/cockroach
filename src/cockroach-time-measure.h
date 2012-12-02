#ifndef cockroach_time_measure_h
#define cockroach_time_measure_h

#include <semaphore.h>
#include <stdint.h>

struct measured_time_shm_header
{
	sem_t sem;
	uint64_t shm_size; // in bytes
	uint64_t count;
	uint64_t next_index;
};

struct measured_time_shm_slot
{
	double dt;
	unsigned long target_addr;
	unsigned long func_ret_addr;
	pid_t pid;
	pid_t tid;
};

#define MEASURED_TIME_SHM_HEADER_SIZE sizeof(struct measured_time_shm_header)
#define MEASURED_TIME_SHM_SLOT_SIZE sizeof(struct measured_time_shm_slot)

#define COCKROACH_TIME_MEASURE_SHM_NAME "/cockroach_time_measure"

#endif
