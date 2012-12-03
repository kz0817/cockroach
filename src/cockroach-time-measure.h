#ifndef cockroach_time_measure_h
#define cockroach_time_measure_h

#include <semaphore.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct measured_time_shm_header
{
	int format_version;
	sem_t sem;
	uint64_t shm_size; /* in bytes */
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

#define MEASURED_TIME_SHM_FORMAT_VERSION 1

#define COCKROACH_TIME_MEASURE_SHM_NAME "/cockroach_time_measure"

int cockroach_lock_shm(measured_time_shm_header *header);
int cockroach_unlock_shm(measured_time_shm_header *header);
measured_time_shm_header *cockroach_map_measured_time_header(int *fd);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
