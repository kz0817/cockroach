#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>
using namespace std;

#include <semaphore.h> 
#include <sys/mman.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> 
#include <sys/types.h>

#include "cockroach-time-measure.h"

typedef bool (*command_func_t)(vector<string> &args);
typedef map<string, command_func_t> command_map_t;
typedef command_map_t::iterator command_map_itr;

bool command_reset(vector<string> &args)
{
	static const int FIRST_ALLOC_SHM_SIZE = 1024*1024;

	int shm_fd = shm_open(COCKROACH_TIME_MEASURE_SHM_NAME,
	                      O_RDWR|O_CREAT, 0666);
	if (shm_fd == -1) {
		printf("Failed to open shm: %d\n", errno);
		return false;
	}
	if (ftruncate(shm_fd, FIRST_ALLOC_SHM_SIZE) == -1) {
		printf("Failed to truncate shm: %d\n", errno);
		return false;
	}

	void *ptr = mmap(NULL, MEASURED_TIME_SHM_HEADER_SIZE,
	                 PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
		printf("Failed to map shm: %d\n", errno);
		return false;
	}

	measured_time_shm_header *header = (measured_time_shm_header *)ptr;
	static const int shared = 1;
	sem_init(&header->sem, shared, 1);
	header->shm_size = FIRST_ALLOC_SHM_SIZE;
	header->count = 0;
	header->next_index = MEASURED_TIME_SHM_HEADER_SIZE;

	printf("reset shm: success\n");
	return true;
}

bool command_info(vector<string> &args)
{
	return true;
}

void print_usage(void)
{
	printf("Usage:\n");
	printf("\n");
	printf("$ cockroach-time-measure-tool command args\n");
	printf("\n");
	printf("*** Commands ***\n");
	printf("reset\n");
	printf("info\n");
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
