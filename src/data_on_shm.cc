#include <set>
using namespace std;

#include <pthread.h>
#include <sys/mman.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h> 
#include <sys/types.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "cockroach-probe.h"
#include "utils.h"

#define COCKROACH_DATA_SHM_NAME "cockroach_data"

#define DEFAULT_SHM_ADD_UNIT_SIZE (1024*1024) // 1MiB
#define DEFAULT_SHM_WINDOW_SIZE   DEFAULT_SHM_ADD_UNIT_SIZE

struct primary_header_t {
	sem_t sem;
	uint64_t size;
	uint64_t head_index;
	uint64_t count;
};

struct item_header_t {
	uint32_t id;
	size_t size;
};

struct map_info_t {
	int used_count;
	uint64_t offset;
	uint64_t size;
};

typedef set<map_info_t *> map_info_set_t;
typedef map_info_set_t::iterator map_info_set_itr;

static int               g_shm_fd = 0;
static pthread_mutex_t   g_shm_mutex = PTHREAD_MUTEX_INITIALIZER;
static primary_header_t *g_primary_header = NULL;
static pthread_mutex_t   g_map_info_mutex = PTHREAD_MUTEX_INITIALIZER;

static size_t g_shm_add_unit_size = DEFAULT_SHM_ADD_UNIT_SIZE;
static size_t g_shm_window_size   = DEFAULT_SHM_WINDOW_SIZE;

static map_info_set_t &get_map_info_set(void)
{
	static map_info_set_t set;
	return set;
}

static int lock_shm(primary_header_t *header)
{
top:
	int ret = sem_wait(&header->sem);
	if (ret == 0)
		return 0;
	if (errno == EINTR)
		goto top;
	return -1;
}

static int unlock_shm(primary_header_t *header)
{
	int ret = sem_post(&header->sem); if (ret == 0)
		return 0;
	return -1;
}

static void lock_shm(void)
{
	if (lock_shm(g_primary_header) == -1) {
		ROACH_ERR("Failed: cockroach_lock_shm: %d\n", errno);
		ROACH_ABORT();
	}
}

static void unlock_shm(void)
{
	if (unlock_shm(g_primary_header) == -1) {
		ROACH_ERR("Failed: cockroach_unlock_shm: %d\n", errno);
		ROACH_ABORT();
	}
}

static void open_shm_if_needed(void)
{
	if (g_primary_header)
		return;

	pthread_mutex_lock(&g_shm_mutex);
	// check again because other thread might open the shm while
	// in pthread_mutex_lock().
	if (g_primary_header) {
		pthread_mutex_unlock(&g_shm_mutex);
		return;
	}

	size_t shm_size = sizeof(primary_header_t);
	g_shm_fd = shm_open(COCKROACH_DATA_SHM_NAME, O_RDWR, 0644);
	if (g_shm_fd == -1) {
		if (errno == ENOENT) {
			g_shm_fd = shm_open(COCKROACH_DATA_SHM_NAME,
			                    O_RDWR|O_CREAT, 0644);
		}
		if (g_shm_fd == -1) {
			ROACH_ERR("Failed to open shm: %d\n", errno);
			ROACH_ABORT();
		}
		if (ftruncate(g_shm_fd, shm_size) == -1) {
			ROACH_ERR("Failed: ftruncate: %d\n", errno);
			ROACH_ABORT();
		}
		ROACH_INFO("created: SHM for data\n");
	}

	void *ptr = mmap(NULL, shm_size,
	                 PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
	if (ptr == MAP_FAILED) {
		ROACH_ERR("Failed to map shm: %d\n", errno);
		ROACH_ABORT();
	}
	g_primary_header = (primary_header_t *)ptr;
	int shared = 1;
	unsigned int sem_count = 1;
	sem_init(&g_primary_header->sem, shared, sem_count);
	g_primary_header->size = shm_size;
	g_primary_header->head_index = shm_size;
	g_primary_header->count = 0;
	pthread_mutex_unlock(&g_shm_mutex);
}

static
map_info_t *find_map_info_and_inc_used_count(uint64_t offset, size_t size)
{
	pthread_mutex_lock(&g_map_info_mutex);
	pthread_mutex_unlock(&g_map_info_mutex);
	return NULL;
}

static item_header_t *get_shm_region(size_t size)
{
	open_shm_if_needed();

	lock_shm();
	uint64_t new_index = g_primary_header->head_index
	                     + (sizeof(item_header_t) + size);

	// extend shm size if it is smaller than the size with the requested.
	if (new_index > g_primary_header->size) {
		uint64_t new_size = g_primary_header->size;
		while (new_index > new_size)
			new_size += g_shm_add_unit_size;
		if (ftruncate(g_shm_fd, new_size) == -1) {
			ROACH_ERR("Failed: ftrancate: %d\n", errno);
			ROACH_ABORT();
		}
		g_primary_header->size = new_size;
	}

	// just reserve region. Writing data is performed later w/o the lock.
	g_primary_header->head_index = new_index;
	g_primary_header->count++;

	uint64_t shm_size = g_primary_header->size;
	uint64_t shm_head_index = g_primary_header->head_index;
	unlock_shm();

	// map window if needed
	map_info_t *map_info;
	map_info = find_map_info_and_inc_used_count(shm_head_index, size);
	if (map_info) {
		return NULL;
	}

	// make a new mapped region
	uint64_t page_size = utils::get_page_size();
	uint64_t map_offset = shm_head_index & ~(page_size - 1);
	size_t map_length = g_shm_window_size;
	if (map_length < shm_size - shm_head_index)
		map_length = shm_size - shm_head_index;
	void *ptr = mmap(NULL, map_length,
	                 PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd,
	                 map_offset);
	if (ptr == MAP_FAILED) {
		ROACH_ERR("Failed to map shm: %d: offset: %"PRIu64"\n",
		          errno, map_offset);
		ROACH_ABORT();
	}
	map_info = new map_info_t();
	map_info->used_count = 1;
	map_info->offset = map_offset;
	map_info->size = map_length;

	map_info_set_t &map_info_set = get_map_info_set();
	pthread_mutex_lock(&g_map_info_mutex);
	pair<map_info_set_itr, bool> result;
	result = map_info_set.insert(map_info);
	if (result.second == false) {
		// TODO: merge two window
	}
	pthread_mutex_unlock(&g_map_info_mutex);

	//return (item_header_t *)ptr;
	return NULL;
}

static uint8_t *get_shm_data_area_ptr(item_header_t *item_header)
{
	return (uint8_t *)(item_header + 1);
}

void cockroach_record_data_on_shm(uint32_t id, size_t size,
                                  record_data_func_t record_data_func)
{
	if (id >= COCKROACH_RECORD_ID_RESERVED_BEGIN) {
		ROACH_ERR("id: %d is RESERVED. IGNORED.\n", id);
		return;
	}

	item_header_t *item_header = get_shm_region(size);
	uint8_t *data_area = get_shm_data_area_ptr(item_header);
	item_header->id = id;
	item_header->size = size;
	(*record_data_func)(size, data_area);
	// TODO: decrement map used count
}
