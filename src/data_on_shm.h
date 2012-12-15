#ifndef data_on_shm_h
#define data_on_shm_h

#define COCKROACH_DATA_SHM_NAME "cockroach_data"

#define DEFAULT_SHM_ADD_UNIT_SIZE (1024*1024) // 1MiB
#define DEFAULT_SHM_WINDOW_SIZE   DEFAULT_SHM_ADD_UNIT_SIZE

#define COCKROACH_DATA_SHM_FORMAT_VERSION 1

struct primary_header_t {
	int format_version;
	sem_t sem;
	uint64_t size;
	uint64_t head_index;
	uint64_t count;
};

struct item_header_t {
	uint32_t id;
	size_t size;
};

primary_header_t *get_primary_header(int *shm_fd);
void init_primary_header(primary_header_t *header, size_t shm_size);
int lock_data_shm(primary_header_t *header);
int unlock_data_shm(primary_header_t *header);

#endif // data_on_shm_h
