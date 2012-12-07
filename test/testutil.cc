#include <cstdio>
#include <vector>
using namespace std;

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
using namespace boost;

#include <cppcutter.h>
#include <glib.h>

#include "testutil.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void
testutil::exec_command(const char *command_line, string *std_output)
{
	gboolean ret;
	gint exit_status;
	gchar *str;
	gchar **str_ptr = &str;
	if (std_output == NULL)
		str_ptr = NULL;
	ret = g_spawn_command_line_sync(command_line, str_ptr,
	                                NULL, &exit_status, NULL);
	if (std_output) {
		*std_output = str;
		g_free(str);
	}
	cppcut_assert_equal(TRUE, ret);
	cppcut_assert_equal(EXIT_SUCCESS, exit_status);
}

string testutil::exec_time_measure_tool(const char *arg)
{
	string result;
	string command_line = "../src/cockroach-time-measure-tool ";
	command_line += arg;
	exec_command(command_line.c_str(), &result);
	return result;
}

void testutil::reset_time_list(void)
{
	exec_time_measure_tool("reset");
}

void testutil::assert_measured_time(int expected_num_line)
{
	string result = exec_time_measure_tool("list");
	if (expected_num_line >= 0)
		assert_measured_time_lines(expected_num_line, result);
}

void testutil::assert_measured_time_lines(int expected_num_line, string &lines)
{
	vector<string> split_lines;
	split(split_lines, lines, is_any_of("\n"), token_compress_on); 
	size_t num_lines = split_lines.size();

	// remove the last empty component
	if (num_lines > 0) {
		string &last_line = split_lines.back();
		if (last_line.empty())
			split_lines.pop_back();
	}

	cppcut_assert_equal(expected_num_line, (int)split_lines.size());
	BOOST_FOREACH(string line, split_lines)
		assert_measured_time_format(line);
}

void testutil::assert_measured_time_format(string &line)
{
	vector<string> tokens;
	split(tokens, line, is_any_of(" "), token_compress_on); 
}

string testutil::exec_helper(const char *recipe_path, const char *arg)
{
	setenv("LD_PRELOAD", "../src/.libs/cockroach.so", 1);
	setenv("COCKROACH_RECIPE", recipe_path, 1);

	string std_output;
	string cmd = ".libs/lt-helper-bin ";
	cmd += arg;
	exec_command(cmd.c_str(),  &std_output);
	return std_output;

}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
