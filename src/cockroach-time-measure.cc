#include <cstdio>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cockroach-time-measure.h"

extern "C"
int cockroach_lock_shm(measured_time_shm_header *header)
{
top:
	int ret = sem_wait(&header->sem);
	if (ret == 0)
		return 0;
	if (errno == EINTR)
		goto top;
	return -1;
}

extern "C"
int cockroach_unlock_shm(measured_time_shm_header *header)
{
	int ret = sem_post(&header->sem); if (ret == 0)
		return 0;
	return -1;
}

extern "C"
measured_time_shm_header *cockroach_map_measured_time_header(int *fd)
{
	*fd = shm_open(COCKROACH_TIME_MEASURE_SHM_NAME, O_RDWR, 0666);
	if (*fd == -1)
		return NULL;

	void *ptr = mmap(NULL, MEASURED_TIME_SHM_HEADER_SIZE,
	                 PROT_READ|PROT_WRITE, MAP_SHARED, *fd, 0);
	if (ptr == MAP_FAILED)
		return NULL;
	return (measured_time_shm_header *)ptr;
}
