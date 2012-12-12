#include <cstdio>
#include <cstdlib>
#include <vector>
#include <list>
#include <string>
using namespace std;

#include <errno.h>
#include <dlfcn.h>

#include "utils.h"
#include "mapped_lib_manager.h"
#include "probe.h"
#include "time_measure_probe.h"

static void *(*g_orig_dlopen)(const char *, int) = NULL;
static int (*g_orig_dlclose)(void *) = NULL;

// probe type, install type, target lib, and address
static const size_t NUM_RECIPE_MIN_TOKENS = 4;
static const size_t NUM_RECIPE_MIN_USER_PROBE_TOKENS = 2; // probe_lib and symbol 

typedef list<probe> probe_list_t;
typedef probe_list_t::iterator  probe_list_itr;

typedef map<string, void *> user_probe_lib_handle_map_t;
typedef user_probe_lib_handle_map_t::iterator user_probe_lib_handle_map_itr;

class cockroach {
	mapped_lib_manager m_mapped_lib_mgr;
	probe_list_t m_probe_list;

	static user_probe_lib_handle_map_t &get_user_probe_lib_handle_map(void);
	static void _parse_one_recipe(const char *line, void *arg);
	void parse_recipe(const char *recipe_file);
	void parse_one_recipe(const char *line);
	void add_user_probe(probe &user_probe, vector<string> &tokens,
	                    size_t &idx);
public:
	cockroach(void);
};

cockroach::cockroach(void)
{
	// Init original funcs
	utils::init_original_func_addr_table();

	ROACH_INFO("started cockroach (%s)\n", __DATE__);
	g_orig_dlopen = (void *(*)(const char *, int))dlsym(RTLD_NEXT, "dlopen");
	if (!g_orig_dlopen) {
		ROACH_ERR("Failed to call dlsym() for dlopen.\n");
		exit(EXIT_FAILURE);
	}

	g_orig_dlclose = (int (*)(void *))dlsym(RTLD_NEXT, "dlclose");
	if (!g_orig_dlclose) {
		ROACH_ERR("Failed to call dlsym() for dlclose.\n");
		exit(EXIT_FAILURE);
	}

	// parse recipe file
	char *recipe_file = getenv("COCKROACH_RECIPE");
	if (!recipe_file) {
		ROACH_ERR("Not defined environment variable: COCKROACH_RECIPE\n");
		exit(EXIT_FAILURE);
	}
	parse_recipe(recipe_file);

	// install probes for libraries that have already been mapped.
	probe_list_itr probe = m_probe_list.begin();
	for (; probe != m_probe_list.end(); ++probe) {
		const char *target_name = probe->get_target_lib_path();
		const mapped_lib_info* lib_info;
		lib_info = m_mapped_lib_mgr.get_lib_info(target_name);
		if (!lib_info)
			continue;
		probe->install(lib_info);
	}
}

user_probe_lib_handle_map_t &cockroach::get_user_probe_lib_handle_map(void)
{
	static user_probe_lib_handle_map_t map;
	return map;
}

void cockroach::parse_one_recipe(const char *line)
{
	size_t idx = 0;
	// strip head spaces
	vector<string> tokens = utils::split(line);
	if (tokens.empty())
		return;

	// check if this is the comment line
	if (tokens[idx].at(0) == '#') // comment line
		return;

	// check the numbe of tokens
	if (tokens.size() < NUM_RECIPE_MIN_TOKENS) {
		ROACH_ERR("Number of tokens is too small: %zd: %s\n",
		          tokens.size(), line);
		ROACH_ABORT();
	}

	// probe type
	probe_type_t probe_type = PROBE_TYPE_UNKNOWN;
	string &probe_type_def = tokens[idx];
	if (probe_type_def == "T") {
		probe_type = PROBE_TYPE_BUILT_IN_TIME_MEASURE;
	} else if (probe_type_def == "P") {
		probe_type = PROBE_TYPE_USER;
	}
	else {
		ROACH_ERR("Unknown probe_type: '%s' : %s\n",
		          probe_type_def.c_str(), line);
		ROACH_ABORT();
	}
	idx++;

	// install type
	install_type_t install_type = INSTALL_TYPE_UNKNOWN;;
	string &install_type_def = tokens[idx];
	if (install_type_def == "ABS64")
		install_type = INSTALL_TYPE_OVERWRITE_ABS64_JUMP;
	else if (install_type_def == "REL32")
		install_type = INSTALL_TYPE_OVERWRITE_REL32_JUMP;
	else {
		ROACH_ERR("Unknown command: '%s' : %s\n",
		          tokens[0].c_str(), line);
		ROACH_ABORT();
	}
	idx++;

	// target address
	string &target_lib = tokens[idx];
	idx++;
	string &sym_or_addr = tokens[idx];
	idx++;
	unsigned long target_addr = 0;
	if (utils::is_hex_number(sym_or_addr.c_str())) {
		if (sscanf(sym_or_addr.c_str(), "%lx", &target_addr) != 1) {
			ROACH_ERR("Failed to parse address: %s: %s", 
			          target_lib.c_str(), sym_or_addr.c_str());
			exit(EXIT_FAILURE);
		}
	} else {
		ROACH_ERR("Currently symbol is not implemented. "
		          "Please specify address. %s: %s\n",
		       target_lib.c_str(), sym_or_addr.c_str());
		exit(EXIT_FAILURE);
	}

	// target address
	int overwrite_length = 0;
	if (tokens.size() > idx && utils::is_hex_number(tokens[idx])) {
		string &overwrite_length_def = tokens[idx];
		overwrite_length = atoi(overwrite_length_def.c_str());
		idx++;
	}

	// register probe
	probe probe(probe_type, install_type);
	probe.set_target_address(target_lib.c_str(), target_addr,
	                         overwrite_length);

	if (probe_type == PROBE_TYPE_BUILT_IN_TIME_MEASURE) {
		probe.set_probe(NULL, roach_time_measure_probe,
		                      roach_time_measure_probe_init);
	}
	else if (probe_type == PROBE_TYPE_USER) {
		if (tokens.size() - idx < NUM_RECIPE_MIN_USER_PROBE_TOKENS) {
			ROACH_ERR("Token is too short: %s: %d\n", line, errno);
			ROACH_ABORT();
		}
		add_user_probe(probe, tokens, idx);
	}

	m_probe_list.push_back(probe);
}

void cockroach::_parse_one_recipe(const char *line, void *arg)
{
	cockroach *obj = static_cast<cockroach *>(arg);
	obj->parse_one_recipe(line);
}

void cockroach::parse_recipe(const char *recipe_file)
{
	bool ret = utils::read_one_line_loop(recipe_file,
	                                     cockroach::_parse_one_recipe,
	                                     this);
	if (!ret) {
		ROACH_ERR("Failed to parse recipe file: %s (%d)\n", recipe_file,
		          errno);
		ROACH_ABORT();
	}
}

void
cockroach::add_user_probe(probe &user_probe, vector<string> &tokens, size_t &idx)
{
	// check number of arguments
	string &probe_lib_name = tokens[idx++];
	string &probe_func_name = tokens[idx++];

	user_probe_lib_handle_map_t &user_probe_map
	  = get_user_probe_lib_handle_map();
	user_probe_lib_handle_map_itr it = user_probe_map.find(probe_lib_name);
	void *handle = NULL;
	dlerror(); // clear
	if (it == user_probe_map.end()) {
		// open the shared library that contains user probes
		handle = dlopen(probe_lib_name.c_str(), RTLD_LAZY);
		if (handle == NULL) {
			ROACH_ERR("Failed to call dlopen(): %s: %s\n",
			          probe_lib_name.c_str(), dlerror());
			ROACH_ABORT();
		}
		user_probe_map[probe_lib_name] = handle;
	} else
		handle = it->second;

	// probe
	probe_func_t probe_func =
	  (probe_func_t)dlsym(handle, probe_func_name.c_str());
	if (probe_func == NULL) {
		ROACH_ERR("Failed to call dlsym(): %s: %s\n",
		          probe_func_name.c_str(), dlerror());
		ROACH_ABORT();
	}

	// probe initializer
	probe_init_func_t probe_init_func = NULL;
	if (tokens.size() > idx) {
		string &probe_init_func_name = tokens[idx++];
		probe_init_func =
		  (probe_init_func_t)dlsym(handle, probe_init_func_name.c_str());
		if (probe_func == NULL) {
			ROACH_ERR("Failed to call dlsym(): %s: %s\n",
			          probe_init_func_name.c_str(), dlerror());
			ROACH_ABORT();
		}
	}

	user_probe.set_probe(NULL, probe_func, probe_init_func);
}

// make an instance
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

