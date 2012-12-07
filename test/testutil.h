#ifndef testutil_h
#define testutil_h

#include <string>
using namespace std;

#include <glib.h>

class testutil {

public:
	static void exec_command(const char *command_line,
	                         string *std_output = NULL);
	static string exec_time_measure_tool(const char *arg);
	static void reset_time_list(void);
	static void assert_measured_time(int expected_num_line);
	static void assert_measured_time_lines(int expected_num_line, string &lines);
	static void assert_measured_time_format(string &line);
	static string exec_helper(const char *recipe_path, const char *arg);
};

#endif
