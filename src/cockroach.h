#ifndef cockroach_h
#define cockroach_h

#include <list>
#include <vector>
using namespace std;

#include "mapped_lib_manager.h"
#include "probe.h"

typedef list<probe *> probe_list_t;
typedef probe_list_t::iterator  probe_list_itr;

typedef map<string, probe_list_t> libpath_probe_list_map_t;
typedef libpath_probe_list_map_t::iterator libpath_probe_list_map_itr;

typedef map<string, void *> user_probe_lib_handle_map_t;
typedef user_probe_lib_handle_map_t::iterator user_probe_lib_handle_map_itr;

typedef void *(*dlopen_func_t)(const char *, int);
typedef int (*dlclose_func_t)(void *);

class cockroach {
	bool m_flag_not_target;
	mapped_lib_manager m_mapped_lib_mgr;
	probe_list_t m_probe_list;
	libpath_probe_list_map_t m_waiting_probe_map;
	static pthread_mutex_t m_mutex;

	static user_probe_lib_handle_map_t &get_user_probe_lib_handle_map(void);
	static void _parse_one_recipe(const char *line, void *arg);
	void add_probe_to_waiting_probe_map(probe *aprobe);
	void parse_recipe(const char *recipe_file);
	void parse_one_recipe(const char *line);
	void parse_target_exe(vector<string> &target_exe_line);
	void add_user_probe(probe *user_probe, vector<string> &tokens,
	                    size_t &idx);
	void dlopen_hook_each(probe_list_t &probe_list, void *handle,
	                      const mapped_lib_info *lib_info);
public:
	// public members
	static dlopen_func_t  m_orig_dlopen;
	static dlclose_func_t m_orig_dlclose;

	// methods
	cockroach(void);
	virtual ~cockroach();
	void *dlopen_hook(const char *filename, int flag, void *handle);
};

#endif // cockroach_h
