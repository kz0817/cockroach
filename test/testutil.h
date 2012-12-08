#ifndef testutil_h
#define testutil_h

#include <string>
using namespace std;

#include <glib.h>

struct exec_command_arg {
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
	exec_command_arg();
	~exec_command_arg();
};

class testutil {

public:
	static void exec_command(exec_command_arg *arg);
	static string exec_time_measure_tool(const char *arg);
	static void reset_time_list(void);
	static void assert_measured_time(int expected_num_line);
	static void assert_measured_time_lines(int expected_num_line, string &lines);
	static void assert_measured_time_format(string &line);
	static string exec_helper(const char *recipe_path, const char *arg);
};

#endif
