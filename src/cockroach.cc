#include <cstdio>
#include <cstdlib>
#include <vector>
#include <set>
#include <string>
using namespace std;

#include <errno.h> 
#include <dlfcn.h> 

#include "utils.h"
#include "mapped_lib_manager.h"
#include "probe_info.h"

static void *(*g_orig_dlopen)(const char *, int) = NULL;
static int (*g_orig_dlclose)(void *) = NULL;

class cockroach {
	mapped_lib_manager mapped_lib_mgr;

	void parse_recipe(const char *recipe_file);
	void parse_one_recipe(const char *line);
	void parse_time_measurement(vector<string> &tokens);
public:
	cockroach(void);
};

cockroach::cockroach(void)
{
	printf("started cockroach (%s)\n", __DATE__);
	g_orig_dlopen = (void *(*)(const char *, int))dlsym(RTLD_NEXT, "dlopen");
	if (!g_orig_dlopen) {
		printf("Failed to call dlsym() for dlopen.\n");
		exit(EXIT_FAILURE);
	}

	g_orig_dlclose = (int (*)(void *))dlsym(RTLD_NEXT, "dlclose");
	if (!g_orig_dlclose) {
		printf("Failed to call dlsym() for dlclose.\n");
		exit(EXIT_FAILURE);
	}

	// parse recipe file
	char *recipe_file = getenv("COCKROACH_RECIPE");
	if (!recipe_file) {
		printf("Not defined environment variable: COCKROACH_RECIPE\n");
		exit(EXIT_FAILURE);
	}
	parse_recipe(recipe_file);

	// install probes for libraries that have already been mapped.
}

void cockroach::parse_time_measurement(vector<string> &tokens)
{
	if (tokens.size() != 3) {
		printf("Invalid format: tokens(%zd) != 3\n", tokens.size());
		for (int i = 0; i < tokens.size(); i++) {
			printf("[%d] %s\n", i, tokens[i].c_str());
		}
		exit(EXIT_FAILURE);
	}
}

void cockroach::parse_one_recipe(const char *line)
{
	// strip head spaces
	vector<string> tokens = utils::split(line);
	if (tokens.empty())
		return;

	if (tokens[0].at(0) == '#') // comment line
		return;

	if (tokens[0] == "T")
		parse_time_measurement(tokens);
	else {
		printf("Unknown command: '%s' : %s\n", tokens[0].c_str(), line);
		exit(EXIT_FAILURE);
	}
}

void cockroach::parse_recipe(const char *recipe_file)
{
	FILE *fp = fopen(recipe_file, "r");
	if (!fp) {
		printf("Failed to open recipe file: %s (%d)\n", recipe_file,
		       errno);
		exit(EXIT_FAILURE);
	}

	static const int MAX_CHARS_ONE_LINE = 4096;
	char buf[MAX_CHARS_ONE_LINE];
	while (true) {
		char *ret = fgets(buf, MAX_CHARS_ONE_LINE, fp);
		if (!ret)
			break;
		parse_one_recipe(buf);
	}
	fclose(fp);
}

// make instance
static cockroach roach;

// --------------------------------------------------------------------------
// wrapper functions
// --------------------------------------------------------------------------
/**
 * dlopen() wrapper to call dlopen_hook_start() if necesary.
 *
 * @param filename of dynamic library (full path).
 * @param flag dlopen() flags.
 * @returns an opaque "handle" for the dynamic library.
 */
extern "C"
void *dlopen(const char *filename, int flag)
{
	void *handle = (*g_orig_dlopen)(filename, flag);
	/*
	map<string, workaround_runner *>::iterator it;
	it = g_dlopen_runner_map.find(filename);
	if (it != g_dlopen_runner_map.end()) {
		workaround_runner *runner = it->second;
		runner->dlopen_hook_start(handle, filename);
	}*/
	return handle;
}

/**
 * just calling glibc dlclose().
 */
extern "C"
int dlclose(void *handle) __THROW
{
	return (*g_orig_dlclose)(handle);
}

