#include <cstdio>
#include <vector>
#include <fstream>
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
// Class: target_probe_info
// ---------------------------------------------------------------------------
target_probe_info::target_probe_info(pid_t pid, const char *recipe_file,
                                     const char *target_func)
: m_pid(pid),
  m_recipe_file(recipe_file),
  m_target_func(target_func)
{
}

unsigned long target_probe_info::get_target_addr(void)
{
	ifstream ifs(m_recipe_file);
	if (!ifs)
		cut_fail("Failed to open: %s", m_recipe_file);
	string read_buffer;
	bool next_line_should_have_defs = false;
	while (getline(ifs, read_buffer)){
		vector<string> tokens;
		split(tokens, read_buffer, is_any_of(" "));

		if (next_line_should_have_defs) {
			const static size_t IDX_TARGET_ADDR = 3;
			if (tokens.size() <= IDX_TARGET_ADDR) {
				cut_fail("Unexpected line: %s\n",
				         read_buffer.c_str());
			}
			unsigned long addr;
			int num = sscanf(tokens[IDX_TARGET_ADDR].c_str(),
			                 "%lx", &addr);
			cppcut_assert_equal(1, num);
			return addr;
		}

		if (tokens.size() < 2)
			continue;
		if (tokens[0] != "#")
			continue;
		if (tokens[1] == m_target_func)
			next_line_should_have_defs = true;
	}
	cut_fail("Not found: %s in %s\n", m_target_func, m_recipe_file);
	return 0;
}

pid_t target_probe_info::get_pid(void)
{
	return m_pid;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void testutil::exec_command(exec_command_info *arg)
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

	// stderr is always saved
	string stderr_buf;
	thr_grp.create_thread(bind(read_fd_thread, ref(fd_stderr),
	                           ref(stderr_buf),
	                           ref(errno_stderr)));
	if (arg->save_stderr)
		arg->stderr_str = stderr_buf;

	waitpid(arg->child_pid, &arg->status, 0);
	g_spawn_close_pid(arg->child_pid);
	thr_grp.join_all();
	cppcut_assert_equal(0, errno_stdout);
	cppcut_assert_equal(0, errno_stderr);
	cppcut_assert_equal(1, WIFEXITED(arg->status),
	                    cut_message("%s", stderr_buf.c_str()));
	cppcut_assert_equal(EXIT_SUCCESS, WEXITSTATUS(arg->status),
	                    cut_message("%s", stderr_buf.c_str()));
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
                                    target_probe_info *probe_info)
{
	exec_command_info exec_info;
	exec_time_measure_tool("list", &exec_info);
	if (expected_num_line >= 0) {
		assert_measured_time_lines(expected_num_line,
		                           exec_info.stdout_str, probe_info);
	}
}

void testutil::assert_measured_time_lines(int expected_num_line,
                                          string &lines,
                                          target_probe_info *probe_info)
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
		assert_measured_time_format(line, probe_info);
}

void testutil::assert_measured_time_format(string &line,
                                           target_probe_info *probe_info)
{
	static const int NUM_TIME_MEASURE_TOKENS = 5;
	static const int IDX_TIME_MEASURED_TIME_LIST        = 0;
	static const int IDX_TARGET_ADDR_MEASURED_TIME_LIST = 1;
	//static const int IDX_RET_ADDR_MEASURED_TIME_LIST    = 2;
	static const int IDX_PID_MEASURED_TIME_LIST         = 3;

	vector<string> tokens;
	split(tokens, line, is_any_of(" "), token_compress_on);
	cppcut_assert_equal(NUM_TIME_MEASURE_TOKENS, (int)tokens.size());

	// time
	double dt = atof(tokens[IDX_TIME_MEASURED_TIME_LIST].c_str());
	cppcut_assert_equal(true, dt > 1.0e-6);

	// target address (just compare below a page file size)
	unsigned long expected_target_addr = probe_info->get_target_addr();
	expected_target_addr &= (get_page_size() - 1);
	unsigned long actual_target_addr;
	sscanf(tokens[IDX_TARGET_ADDR_MEASURED_TIME_LIST].c_str(),
	       "%lx", &actual_target_addr);
	actual_target_addr &= (get_page_size() - 1);
	cppcut_assert_equal(expected_target_addr, actual_target_addr);

	// pid
	string pid_str = (format("%d") % probe_info->get_pid()).str();
	cut_assert_equal_string(pid_str.c_str(),
	                        tokens[IDX_PID_MEASURED_TIME_LIST].c_str());

}

void testutil::run_target_exe(const char *recipe_path, string arg_str,
                              exec_command_info *exec_info)
{
	add_test_libs_dir_to_ld_library_path_if_needed();
	setenv("LD_PRELOAD", "../src/.libs/cockroach.so", 1);
	setenv("COCKROACH_RECIPE", recipe_path, 1);

	// split arg
	vector<string> arg_vect;
	split(arg_vect, arg_str, is_any_of(" "));

	static const char *cmd = ".libs/target-exe";
	const char *argv[arg_vect.size() + 2];
	argv[0] = cmd;
	for (size_t i = 0; i < arg_vect.size(); i++)
		argv[1+i] = arg_vect[i].c_str();
	argv[arg_vect.size()+1] = NULL;

	exec_info->argv = argv;
	exec_info->save_stdout = true;
	exec_command(exec_info);
}

long testutil::get_page_size(void)
{
	return sysconf(_SC_PAGESIZE);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
void testutil::add_test_libs_dir_to_ld_library_path_if_needed(void)
{
	const static char *TEST_LIBS_DIR = ".libs";
	char *env = getenv("LD_LIBRARY_PATH");
	if (!env) {
		setenv("LD_LIBRARY_PATH", TEST_LIBS_DIR, 1);
		return;
	}

	// check if the path has already been set
	set<string> path_set;
	split(path_set, env, is_any_of(":"));
	set<string>::iterator path_it = path_set.find("");
	if (path_it != path_set.end() && *path_it == TEST_LIBS_DIR)
		return; // already set

	// add the test libs path
	string new_path = env;
	new_path += ":";
	new_path += TEST_LIBS_DIR;
	setenv("LD_LIBRARY_PATH", new_path.c_str(), 1);
}
