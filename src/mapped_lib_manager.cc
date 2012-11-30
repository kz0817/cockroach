#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
using namespace std;

#include <errno.h>

#include "mapped_lib_manager.h"
#include "utils.h"

#define NUM_SHARED_LIB_MAP_ARGS      6
#define IDX_SHARED_LIB_MAP_ADDR_RAGE 0
#define IDX_SHARED_LIB_MAP_PERM      1
#define IDX_SHARED_LIB_MAP_PATH      5

#define LEN_SHARED_LIB_MAP_PERM      4
#define IDX_PERM_EXEC                2

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------
void mapped_lib_manager::parse_mapped_lib_line(const char *line)
{
	// strip head spaces
	vector<string> tokens = utils::split(line);
	if (tokens.size() != NUM_SHARED_LIB_MAP_ARGS)
		return;

	// select path that starts from '/'
	string &path = tokens[IDX_SHARED_LIB_MAP_PATH];
	if (path.at(0) != '/')
		return;

	// select permission that contains execution
	string &perm = tokens[IDX_SHARED_LIB_MAP_PERM];
	if (perm.size() < LEN_SHARED_LIB_MAP_PERM)
		return;
	if (perm[IDX_PERM_EXEC] != 'x')
		return;

	// parse start address
	unsigned long start_addr, end_addr;
	string &addr_range = tokens[0];
	int ret = sscanf(addr_range.c_str(), "%lx-%lx", &start_addr, &end_addr);
	if (ret != 2)
		return;

	unsigned long length = end_addr - start_addr;
	mapped_lib_info lib_info(path.c_str(), start_addr, length);
	m_lib_info_set.insert(lib_info);
}

void mapped_lib_manager::_parse_mapped_lib_line(const char *line, void *arg)
{
	mapped_lib_manager *obj = static_cast<mapped_lib_manager *>(arg);
	obj->parse_mapped_lib_line(line);
}

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
mapped_lib_manager::mapped_lib_manager()
{
	const char *map_file_path = "/proc/self/maps";
	bool ret = utils::read_one_line_loop(map_file_path,
	                                     mapped_lib_manager::_parse_mapped_lib_line,
	                                     this);
	if (!ret) {
		printf("Failed to parse recipe file: %s (%d)\n", map_file_path,
		       errno);
		exit(EXIT_FAILURE);
	}
}

