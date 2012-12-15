#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
using namespace std;

#include <semaphore.h> 
#include <sys/mman.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> 
#include <sys/types.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "cockroach-probe.h"
#include "data_on_shm.h"

typedef bool (*command_func_t)(vector<string> &args);
typedef map<string, command_func_t> command_map_t;
typedef command_map_t::iterator command_map_itr;

static bool command_reset(vector<string> &args)
{
	static const int FIRST_ALLOC_SHM_SIZE = sizeof(primary_header_t);

	int shm_fd = shm_open(COCKROACH_DATA_SHM_NAME, O_RDWR|O_CREAT, 0666);
	if (shm_fd == -1) {
		printf("Failed to open shm: %d\n", errno);
		return false;
	}
	if (ftruncate(shm_fd, FIRST_ALLOC_SHM_SIZE) == -1) {
		printf("Failed to truncate shm: %d\n", errno);
		return false;
	}

	void *ptr = mmap(NULL, FIRST_ALLOC_SHM_SIZE,
	                 PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
		printf("Failed to map shm: %d\n", errno);
		return false;
	}
	primary_header_t *header = (primary_header_t *)ptr;
	init_primary_header(header, FIRST_ALLOC_SHM_SIZE);

	printf("reset shm: success\n");
	return true;
}

static bool command_remove(vector<string> &args)
{
	if (shm_unlink(COCKROACH_DATA_SHM_NAME) == -1) {
		printf("Failed to unlink shm: %d\n", errno);
		return false;
	}
	printf("remove shm: success\n");
	return true;
}

static bool command_info(vector<string> &args)
{
	int shm_fd;
	primary_header_t *header = get_primary_header(&shm_fd);
	if (header == NULL) {
		printf("Failed to map header: %d\n", errno);
		return false;
	}

	if (lock_data_shm(header) == -1) {
		printf("Failed to lock shm: %d\n", errno);
		return false;
	}
	int format_version = header->format_version;
	uint64_t shm_size = header->size;
	uint64_t count = header->count;
	uint64_t head_index = header->head_index;
	if (unlock_data_shm(header) == -1) {
		printf("Failed to unlock shm: %d\n", errno);
		return false;
	}

	printf("ver. : %d\n", format_version);
	printf("size : %"PRIu64"\n", shm_size);
	printf("count: %"PRIu64"\n", count);
	printf("index: %"PRIu64"\n", head_index);

	return true;
}

static bool command_list(vector<string> &args)
{
	int shm_fd;
	primary_header_t *header = get_primary_header(&shm_fd);
	if (header == NULL) {
		printf("Failed to map header: %d\n", errno);
		return false;
	}

	if (lock_data_shm(header) == -1) {
		printf("Failed to lock shm: %d\n", errno);
		return false;
	}
	uint64_t shm_size = header->size;
	uint64_t count = header->count;
	if (unlock_data_shm(header) == -1) {
		printf("Failed to unlock shm: %d\n", errno);
		return false;
	}

	// map entire shm
	void *ptr = mmap(NULL, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
		printf("Failed to map shm (entire): %d\n", errno);
		return false;
	}

	// print data
	item_header_t *item = (item_header_t *)(header + 1);
	for (uint64_t i = 0; i < count; i++) {
		printf("%08x %zd\n", item->id, item->size);
		item = (item_header_t *)
		       ((uint8_t *)item + sizeof(item_header_t) + item->size);
	}

	return true;
}

static void print_usage(void)
{
	printf("Usage:\n");
	printf("\n");
	printf("$ cockroach-record-data-tool command args\n");
	printf("\n");
	printf("*** Commands ***\n");
	printf("reset\n");
	printf("remove\n");
	printf("info\n");
	printf("list\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage();
		return EXIT_SUCCESS;
	}
	command_map_t command_map;
	command_map["reset"] = command_reset;
	command_map["info"] = command_info;
	command_map["list"] = command_list;
	command_map["remove"] = command_remove;

	string command = argv[1];
	command_map_itr it = command_map.find(command);
	if (it == command_map.end()) {
		print_usage();
		return EXIT_FAILURE;
	}

	vector<string> args;
	for (int i = 2; i < argc; i++)
		args.push_back(argv[i]);
	if (!(*it->second)(args))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

