#include <cstdio>
#include <vector>
using namespace std;

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
using namespace boost;

#include <sys/types.h> 
#include <sys/wait.h>
#include <cppcutter.h>
#include <gcutter.h>
#include <glib.h>

#include "testutil.h"

exec_command_info::exec_command_info(const char *_argv[])
: save_stdout(false),
  save_stderr(false),
  argv(_argv),
  envp(NULL),
  working_dir(NULL),
  child_setup(NULL),
  user_data(NULL),
  child_pid(0),
  error(NULL),
  status(0)
{
}

exec_command_info::~exec_command_info()
{
	if (error)
		g_error_free(error);
}

static void read_fd_thread(int fd, string &buf, int &errno_ret)
{
	static const int BUF_SIZE = 0x1000;
	char rd_buf[BUF_SIZE];
	while (true) {
		ssize_t ret = read(fd, rd_buf, BUF_SIZE);
		if (ret == -1) {
			errno_ret = errno;
			break;
		} else if (ret == 0)
			break;
		buf.append(rd_buf, 0, ret);
	}
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void
testutil::exec_command(exec_command_info *arg)
{
	// exec
	gboolean ret;
	int fd_stdin, fd_stdout, fd_stderr;
	int errno_stdout = 0;
	int errno_stderr = 0;
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
	thread_group thr_grp;
	if (arg->save_stdout) {
		thr_grp.create_thread(bind(read_fd_thread, ref(fd_stdout),
		                           ref(arg->stdout_str),
		                           ref(errno_stdout)));
	}
	if (arg->save_stderr) {
		thr_grp.create_thread(bind(read_fd_thread, ref(fd_stderr),
		                           ref(arg->stderr_str),
		                           ref(errno_stderr)));
	}
	waitpid(arg->child_pid, &arg->status, 0);
	g_spawn_close_pid(arg->child_pid);
	thr_grp.join_all();
	cppcut_assert_equal(0, errno_stdout);
	cppcut_assert_equal(0, errno_stderr);
	cppcut_assert_equal(EXIT_SUCCESS, WEXITSTATUS(arg->status));
}

void testutil::exec_time_measure_tool(const char *arg, exec_command_info *exec_info)
{
	const gchar *cmd = "../src/cockroach-time-measure-tool";
	const char *argv[] = {cmd, arg, NULL};
	exec_info->argv = argv;
	exec_info->save_stdout = true;
	exec_command(exec_info);
}

void testutil::reset_time_list(void)
{
	exec_command_info exec_info;
	exec_time_measure_tool("reset", &exec_info);
}

void testutil::assert_measured_time(int expected_num_line,
                                    pid_t expected_pid)
{
	exec_command_info exec_info;
	exec_time_measure_tool("list", &exec_info);
	if (expected_num_line >= 0) {
		assert_measured_time_lines(expected_num_line,
		                           exec_info.stdout_str, expected_pid);
	}
}

void testutil::assert_measured_time_lines(int expected_num_line,
                                          string &lines, pid_t expected_pid)
{
	vector<string> split_lines;
	split(split_lines, lines,
	      is_any_of("\n"), token_compress_on);
	size_t num_lines = split_lines.size();

	// remove the last empty component
	if (num_lines > 0) {
		string &last_line = split_lines.back();
		if (last_line.empty())
			split_lines.pop_back();
	}

	cppcut_assert_equal(expected_num_line, (int)split_lines.size());
	BOOST_FOREACH(string line, split_lines)
		assert_measured_time_format(line, expected_pid);
}

void testutil::assert_measured_time_format(string &line, pid_t expected_pid)
{
	static const int NUM_TIME_MEASURE_TOKENS = 5;
	static const int IDX_PID_MEASURED_TIME_LIST = 3;

	vector<string> tokens;
	split(tokens, line, is_any_of(" "), token_compress_on);
	cppcut_assert_equal(NUM_TIME_MEASURE_TOKENS, (int)tokens.size());

	// pid
	string pid_str = (format("%d") % expected_pid).str();
	cut_assert_equal_string(pid_str.c_str(),
	                        tokens[IDX_PID_MEASURED_TIME_LIST].c_str());

}

void testutil::exec_helper(const char *recipe_path, const char *arg,
                             exec_command_info *exec_info)
{
	setenv("LD_PRELOAD", "../src/.libs/cockroach.so", 1);
	setenv("COCKROACH_RECIPE", recipe_path, 1);

	const char *cmd = ".libs/helper-bin";
	const char *argv[] = {cmd, arg, NULL};
	exec_info->argv = argv;
	exec_info->save_stdout = true;
	exec_command(exec_info);

}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
static void add_dot_libs_to_ld_library_path_if_needed(void)
{
	char *env = getenv("LD_LIBRARY_PATH");
	cut_fail("Now under construction...");
	/*if (env) {
		vector<string> paths = split(env, ":");
	}*/
}
