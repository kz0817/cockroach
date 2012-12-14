#include <set>
#include <queue>
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
	uint8_t *addr;
	uint64_t offset; // indicates the index where this map starts
	uint64_t size;
};

struct item_slot_t {
	map_info_t    *map_info;
	item_header_t *header;
	uint8_t       *data;
};

struct map_info_comparator {
	bool operator()(const map_info_t *map_info0,
	                const map_info_t *map_info1) {
		if (map_info0->offset < map_info1->offset)
			return true;
		if (map_info0->offset > map_info1->offset)
			return false;

		if (map_info0->size < map_info1->size)
			return true;
		if (map_info0->size > map_info1->size)
			return false;

		return false;
	}
};

typedef set<map_info_t *, map_info_comparator> map_info_set_t;
typedef map_info_set_t::iterator map_info_set_itr;

typedef queue<map_info_t *> map_info_queue_t;

static int               g_shm_fd = 0;
static pthread_mutex_t   g_shm_mutex = PTHREAD_MUTEX_INITIALIZER;
static primary_header_t *g_primary_header = NULL;
static pthread_mutex_t   g_map_info_mutex = PTHREAD_MUTEX_INITIALIZER;

static size_t g_shm_add_unit_size = DEFAULT_SHM_ADD_UNIT_SIZE;
static size_t g_shm_window_size   = DEFAULT_SHM_WINDOW_SIZE;
static map_info_t       *g_latest_map_info = NULL;


static map_info_set_t &get_map_info_set(void)
{
	static map_info_set_t set;
	return set;
}

static map_info_queue_t &get_unused_map_info_queu(void)
{
	static map_info_queue_t q;
	return q;
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
map_info_t *find_map_info_and_inc_used_count(uint64_t start_index, size_t size)
{
	// This assertion ensures that key_map_info never equals to
	// any element in map_info_set.
	if (size >= g_shm_window_size) {
		ROACH_ERR("size: %zd > g_shm_window_size: %zd\n",
		          size, g_shm_window_size);
		ROACH_ABORT();
	}

	map_info_t key_map_info;
	map_info_t *map_info = NULL;
	key_map_info.offset = start_index;
	key_map_info.size = size;

	map_info_set_t &map_info_set = get_map_info_set();

	pthread_mutex_lock(&g_map_info_mutex);
	map_info_set_itr it = map_info_set.lower_bound(&key_map_info);
	/* !!! The following is wrong !!! */
	if (it == map_info_set.end()) {
		pthread_mutex_unlock(&g_map_info_mutex);
		return NULL;
	}

	if (map_info->offset + map_info->size >= start_index + size) {
		// 'map_info->offset <= start_index' is ensured by
		// lower_bound() that calls map_info_comparator::operator()().
		map_info = *it;
	}
	pthread_mutex_unlock(&g_map_info_mutex);
	return map_info;
}

/**
 * This function must be called while g_map_info_mutex is being taken.
 */
static void free_map(map_info_t *map_info)
{
	get_map_info_set().erase(map_info);

	if (munmap(map_info->addr, map_info->size) == -1)
		ROACH_ERR("Failed: munmap: %d\n", errno);

	delete map_info;
}

/**
 * This function must be called while g_map_info_mutex is being taken.
 */
static void free_unused_maps(void)
{
	map_info_queue_t &unused_map_info_queue = get_unused_map_info_queu();
	while (!unused_map_info_queue.empty()) {
		map_info_t *map_info = unused_map_info_queue.front();
		free_map(map_info);
		unused_map_info_queue.pop();
	}
}

static
map_info_t *create_new_map_info(uint64_t shm_head_index, uint64_t shm_size)
{
	// make a new mapped region
	uint64_t page_size = utils::get_page_size();
	uint64_t map_offset = shm_head_index & ~(page_size - 1);
	size_t map_length = g_shm_window_size;
	if (map_length < shm_size - map_offset)
		map_length = shm_size - map_offset;
	void *ptr = mmap(NULL, map_length,
	                 PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd,
	                 map_offset);
	if (ptr == MAP_FAILED) {
		ROACH_ERR("Failed to map shm: %d: offset: %"PRIu64"\n",
		          errno, map_offset);
		ROACH_ABORT();
	}
	map_info_t *map_info = new map_info_t();
	map_info->used_count = 1;
	map_info->addr = (uint8_t *)ptr;
	map_info->offset = map_offset;
	map_info->size = map_length;

	// insert the created map information to map_info_set.
	map_info_set_t &map_info_set = get_map_info_set();
	pthread_mutex_lock(&g_map_info_mutex);
	pair<map_info_set_itr, bool> result;
	result = map_info_set.insert(map_info);
	if (result.second == false) {
		// merge the map info into the existed one
		map_info_set_itr it = map_info_set.find(map_info);
		if (it == map_info_set.end()) {
			ROACH_BUG("map_info is not found. Even the insert "
			          "operation failed.\n");
		}
		delete map_info;

		map_info_t *existed_info = *it;
		existed_info->used_count++;
		map_info = existed_info;
	}

	g_latest_map_info = map_info;
	// TO BE CONSIDERED: Is here the best to unmap and free object ?
	free_unused_maps();

	pthread_mutex_unlock(&g_map_info_mutex);

	return map_info;
}

/**
 * When this function returns, item_slot->map_info->used_count is incremented.
 * The caller must call dec_used_count() when the map_info is no longer needed.
 */
static void alloc_item_slot(size_t size, item_slot_t *item_slot)
{
	open_shm_if_needed();

	lock_shm();
	uint64_t new_index = g_primary_header->head_index
	                     + (sizeof(item_header_t) + size);

	// extend shm size if it is smaller than the size with the requested.
	if (new_index > g_primary_header->size) {
		uint64_t new_size = g_primary_header->size
		                    + g_shm_add_unit_size;
		if (new_size < new_index) {
			ROACH_BUG("new_size: %"PRIu64" < "
			          "new_index: %"PRIu64"\n",
			          new_size, new_index);
		}
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
	if (!map_info)
		map_info = create_new_map_info(shm_head_index, shm_size);

	int offset_in_map = shm_head_index - map_info->offset;
	if (offset_in_map < 0) {
		ROACH_BUG("offset is negative: idx: %"PRIu64", "
		          "off: %"PRIu64"\n", shm_head_index, map_info->offset);
	}

	// set output data
	item_slot->map_info = map_info;
	item_slot->header = (item_header_t *)(map_info->addr + offset_in_map);
	item_slot->data = (uint8_t*)(item_slot->header + 1);
}

static void dec_used_count(map_info_t *map_info)
{
	pthread_mutex_lock(&g_map_info_mutex);
	map_info->used_count--;
	if (map_info->used_count == 0) {
		if (g_latest_map_info != map_info)
			free_map(map_info);
		else
			get_unused_map_info_queu().push(map_info);
	}
	pthread_mutex_unlock(&g_map_info_mutex);
}

void cockroach_record_data_on_shm(uint32_t id, size_t size,
                                  record_data_func_t record_data_func)
{
	if (id >= COCKROACH_RECORD_ID_RESERVED_BEGIN) {
		ROACH_ERR("id: %d is RESERVED. IGNORED.\n", id);
		return;
	}

	item_slot_t item_slot;
	alloc_item_slot(size, &item_slot);
	item_header_t *item_header = item_slot.header;
	item_header->id = id;
	item_header->size = size;
	(*record_data_func)(size, item_slot.data);
	dec_used_count(item_slot.map_info);
}
