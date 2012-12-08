#include <cstdio>
#include <vector>
using namespace std;

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
using namespace boost;

#include <sys/types.h> 
#include <sys/wait.h>
#include <cppcutter.h>
#include <gcutter.h>
#include <glib.h>

#include "testutil.h"


exec_command_arg::exec_command_arg()
: save_stdout(false),
  save_stderr(false),
  argv(NULL),
  envp(NULL),
  working_dir(NULL),
  child_setup(NULL),
  user_data(NULL),
  child_pid(0),
  error(NULL),
  status(0)
{
}

exec_command_arg::~exec_command_arg()
{
	if (error)
		g_error_free(error);
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void
testutil::exec_command(exec_command_arg *arg)
{
	// exec
	gboolean ret;
	int fd_stdin, fd_stdout, fd_stderr;
	GSpawnFlags flags = (GSpawnFlags)
	                    (G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD);
	ret = g_spawn_async_with_pipes(arg->working_dir,
	                               (gchar **)arg->argv,
	                               (gchar **)arg->envp,
	                               flags,
	                               arg->child_setup, arg->user_data,
	                               &arg->child_pid,
	                               &fd_stdin, &fd_stdout, &fd_stderr,
	                               &arg->error);
	gcut_assert_error(arg->error);
	cppcut_assert_equal(TRUE, ret);
	waitpid(arg->child_pid, &arg->status, 0);
	cppcut_assert_equal(EXIT_SUCCESS, WEXITSTATUS(arg->status));
	g_spawn_close_pid(arg->child_pid);
}

string testutil::exec_time_measure_tool(const char *arg)
{
	string result;
	const gchar *cmd = "../src/cockroach-time-measure-tool";
	exec_command_arg exec_arg;
	const char *argv[] = {cmd, arg, NULL};
	exec_arg.argv = argv;
	exec_command(&exec_arg);
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
	static const int NUM_TIME_MEASURE_TOKENS = 5;
	vector<string> tokens;
	split(tokens, line, is_any_of(" "), token_compress_on);
	cppcut_assert_equal(NUM_TIME_MEASURE_TOKENS, (int)tokens.size());
}

string testutil::exec_helper(const char *recipe_path, const char *arg)
{
	setenv("LD_PRELOAD", "../src/.libs/cockroach.so", 1);
	setenv("COCKROACH_RECIPE", recipe_path, 1);

	const char *cmd = ".libs/lt-helper-bin";
	const char *argv[] = {cmd, arg, NULL};
	exec_command_arg exec_arg;
	exec_arg.argv = argv;
	exec_command(&exec_arg);
	return exec_arg.stdout_str;

}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
