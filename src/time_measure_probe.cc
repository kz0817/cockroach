#include <cstdio>
#include <cstdlib>
using namespace std;

#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "utils.h"
#include "time_measure_probe.h"
#include "cockroach-time-measure.h"

#define SHM_ADD_UNIT_SIZE (1024*1024) // 1MiB
#define SHM_WINDOW_SIZE   SHM_ADD_UNIT_SIZE

static int g_shm_fd = 0;
static measured_time_shm_header *g_shm_header = NULL;
static void *g_shm_window_addr = NULL;
static size_t g_shm_window_size = 0;
static off_t g_shm_window_offset = 0;
static pthread_mutex_t g_shm_window_mutex = PTHREAD_MUTEX_INITIALIZER;

struct time_measure_data {
	struct timeval t0;
	unsigned long target_addr;
	unsigned long func_ret_addr;
	pid_t pid;
};

static void lock_shm(void)
{
	if (cockroach_lock_shm(g_shm_header) == -1) {
		ROACH_ERR("Failed: cockroach_lock_shm: %d\n", errno);
		ROACH_ABORT();
	}
}

static void unlock_shm(void)
{
	if (cockroach_unlock_shm(g_shm_header) == -1) {
		ROACH_ERR("Failed: cockroach_unlock_shm: %d\n", errno);
		ROACH_ABORT();
	}
}

/**
 * This function must be called with g_shm_window_mutex
 */
static measured_time_shm_slot *get_shm_slot(uint64_t shm_index)
{
	int64_t shm_window_idx = shm_index - g_shm_window_offset;
	uint8_t *slot = (uint8_t *)g_shm_window_addr + shm_window_idx;
	return (measured_time_shm_slot *)slot;
}

static void remap_shm_window(uint64_t should_include_index, uint64_t shm_size)
{
	if (g_shm_window_addr) {
		if (munmap(g_shm_window_addr, g_shm_window_size) == -1) {
			ROACH_ERR("Failed: munmap: %d\n", errno);
			ROACH_ABORT();
		}
	}

	int page_size = utils::get_page_size();
	g_shm_window_offset = (should_include_index / page_size) * page_size;

	g_shm_window_size = SHM_WINDOW_SIZE;
	size_t shm_remain_size = shm_size - g_shm_window_offset;
	if (g_shm_window_size > shm_remain_size)
		g_shm_window_size = shm_remain_size;

	void *ptr = mmap(NULL, g_shm_window_size,
	                 PROT_READ|PROT_WRITE, MAP_SHARED, g_shm_fd,
	                 g_shm_window_offset);
	if (ptr == MAP_FAILED) {
		ROACH_ERR("Failed to map shm: %d\n", errno);
		ROACH_ABORT();
	}
	g_shm_window_addr = (measured_time_shm_header *)ptr;
}

static void open_shm_if_needed(void)
{
	if (g_shm_header)
		return;

	pthread_mutex_lock(&g_shm_window_mutex);
	// check again because other thread might open the shm while
	// in pthread_mutex_lock().
	if (g_shm_header) {
		pthread_mutex_unlock(&g_shm_window_mutex);
		return;
	}

	g_shm_fd = shm_open(COCKROACH_TIME_MEASURE_SHM_NAME, O_RDWR, 0644);
	if (g_shm_fd == -1) {
		ROACH_ERR("Failed to open shm: %d\n", errno);
		ROACH_ABORT();
	}

	// head
	void *ptr = mmap(NULL, MEASURED_TIME_SHM_HEADER_SIZE,
	                 PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
	if (ptr == MAP_FAILED) {
		ROACH_ERR("Failed to map shm: %d\n", errno);
		ROACH_ABORT();
	}
	g_shm_header = (measured_time_shm_header *)ptr;
	pthread_mutex_unlock(&g_shm_window_mutex);
}

static measured_time_shm_slot *get_shm_data_slot(void)
{
	open_shm_if_needed();

	lock_shm();
	uint64_t shm_index = g_shm_header->next_index;
	uint64_t next_index = shm_index + MEASURED_TIME_SHM_SLOT_SIZE;

	// extend shm size if needed
	if (g_shm_header->shm_size < next_index) {
		uint64_t new_shm_size;
		new_shm_size = g_shm_header->shm_size + SHM_ADD_UNIT_SIZE;
		if (ftruncate(g_shm_fd, new_shm_size) == -1) {
			unlock_shm();
			ROACH_ERR("Failed to truncate shm: %d\n", errno);
			ROACH_ABORT();
		}
		g_shm_header->shm_size = new_shm_size;
	}

	g_shm_header->next_index = next_index;
	g_shm_header->count++;
	uint64_t shm_size = g_shm_header->shm_size;
	unlock_shm();

	pthread_mutex_lock(&g_shm_window_mutex);
	if (!g_shm_window_addr ||
	    (next_index > g_shm_window_offset + g_shm_window_size)) {
		// we have to change the window area.
		remap_shm_window(shm_index, shm_size);
	}
	measured_time_shm_slot *slot = get_shm_slot(shm_index);
	pthread_mutex_unlock(&g_shm_window_mutex);
	return slot;
}

static double calc_diff_time(timeval *tv0, timeval *tv1)
{
	double t0 = tv0->tv_sec + tv0->tv_usec/1.0e6;
	double t1 = tv1->tv_sec + tv1->tv_usec/1.0e6;
	return t1 - t0;
}

static void roach_time_measure_ret_probe(probe_arg_t *arg)
{
	time_measure_data *priv =
	   static_cast<time_measure_data*>(arg->priv_data);
	timeval t1;
	gettimeofday(&t1, NULL);
	double dt = calc_diff_time(&priv->t0, &t1);
	measured_time_shm_slot *slot = get_shm_data_slot();
	slot->dt = dt;
	slot->target_addr = priv->target_addr;
	slot->func_ret_addr = priv->func_ret_addr;
	slot->pid = priv->pid;
	slot->tid = utils::get_tid();
}

extern "C"
void roach_time_measure_probe_init(probe_init_arg_t *arg)
{
	time_measure_data *priv =new time_measure_data();
	priv->target_addr = arg->target_addr;
	priv->pid = getpid();
	arg->priv_data = priv;
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

