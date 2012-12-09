#ifndef testutil_h
#define testutil_h

#include <string>
using namespace std;

#include <glib.h>

class reader_thread {
public:
};

struct exec_command_info {
	// intput
	bool save_stdout;
	bool save_stderr;
	const gchar **argv;
	const gchar **envp;
	const gchar *working_dir;
	GSpawnChildSetupFunc child_setup;
	gpointer user_data;

	// output (automatically freed)
	GPid child_pid;
	string stdout_str;
	string stderr_str;
	GError *error;
	int status;

	// constructor
	exec_command_info(const char *argv[] = NULL);
	~exec_command_info();
};

class testutil {
	
	static void add_dot_libs_to_ld_library_path_if_needed(void);

public:
	static void exec_command(exec_command_info *arg);
	static void exec_time_measure_tool(const char *arg,
	                                   exec_command_info *exec_info);
	static void reset_time_list(void);
	static void assert_measured_time(int expected_num_line, pid_t pid);
	static void assert_measured_time_lines(int expected_num_line,
	                                       string &lines,
	                                       pid_t expected_pid);
	static void assert_measured_time_format(string &line,
	                                        pid_t expected_pid);
	static void exec_helper(const char *recipe_path, const char *arg,
	                        exec_command_info *exec_info);
};

#endif
