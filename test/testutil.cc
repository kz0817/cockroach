#include <cstdio>
#include <vector>
#include <fstream>
#include <set>
using namespace std;

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
using namespace boost;

#include <sys/types.h> 
#include <sys/wait.h>
#include <cppcutter.h>
#include <gcutter.h>
#include <glib.h>

#include "testutil.h"

typedef char_separator<char> char_separator_t;

// ---------------------------------------------------------------------------
// struct: record_data_tool_output
// ---------------------------------------------------------------------------
record_data_tool_output::record_data_tool_output(void)
: id(0),
  size(0),
  data(NULL)
{
}

record_data_tool_output::~record_data_tool_output()
{
	if (data)
		delete [] data;
}

// ---------------------------------------------------------------------------
// struct: exec_command_info
// ---------------------------------------------------------------------------
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
map<int, string> testutil::signal_name_map;

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
	gboolean expected = TRUE;
	cppcut_assert_equal(expected, ret);
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
	cut_assert_equal_int(1, WIFEXITED(arg->status),
	                    cut_message("%s\n%s", stderr_buf.c_str(),
	                                get_exit_info(arg->status).c_str()));
	cppcut_assert_equal(EXIT_SUCCESS, WEXITSTATUS(arg->status),
	                    cut_message("%s\n%s", stderr_buf.c_str(),
	                                get_exit_info(arg->status).c_str()));
}

void
testutil::exec_time_measure_tool(const char *arg, exec_command_info *exec_info)
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
	int num_lines = 0;
	char_separator_t sep("\n");
	tokenizer<char_separator_t> tokens(lines, sep);
	tokenizer<char_separator_t>::iterator it;
	for (it = tokens.begin(); it != tokens.end(); ++it, num_lines++) {
		string line = *it;
		assert_measured_time_format(line, probe_info);
	}
	cppcut_assert_equal(expected_num_line, num_lines);
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
	//cppcut_assert_equal(true, dt > 1.0e-6);
	cppcut_assert_equal(true, dt >= 0);

	// target address (just compare below a page file size)
	unsigned long expected_target_addr = probe_info->get_target_addr();
	expected_target_addr &= (get_page_size() - 1);
	unsigned long actual_target_addr =
	  strtol(tokens[IDX_TARGET_ADDR_MEASURED_TIME_LIST].c_str(), NULL, 16);
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

void
testutil::assert_parse_record_data_output
  (string *line, list<record_data_tool_output> &tool_output_list)
{
	tool_output_list.push_back(record_data_tool_output());
	record_data_tool_output &tool_out = tool_output_list.back();
	static const int NUM_ITEM_HEADER_ELEM = 2;
	int num_item_header_elem = sscanf(line[0].c_str(), "%x %zd",
	                                  &tool_out.id, &tool_out.size);
	cppcut_assert_equal(NUM_ITEM_HEADER_ELEM, num_item_header_elem);

	uint8_t *buf = new uint8_t[tool_out.size];
	char_separator_t sep_spc(" ");
	tokenizer<char_separator_t> raw_data(line[1], sep_spc);
	size_t idx = 0;
	tokenizer<char_separator_t>::iterator it;
	for (it = raw_data.begin(); it != raw_data.end(); ++it, idx++) {
		cppcut_assert_equal(true, idx < tool_out.size);
		buf[idx] = strtol(it->c_str(), NULL, 16);
	}
	tool_out.data = buf;
}

void testutil::assert_get_record_data(list<record_data_tool_output> &tool_output_list)
{
	const gchar *cmd = "../src/cockroach-record-data-tool";
	const char *argv[] = {cmd, "list", "--dump", NULL};
	exec_command_info exec_info;
	exec_info.argv = argv;
	exec_info.save_stdout = true;
	exec_command(&exec_info);

	// extract the last two lines
	string line[2];
	char_separator_t sep("\n");
	tokenizer<char_separator_t> tokens(exec_info.stdout_str, sep);

	int num_lines = 0;
	tokenizer<char_separator_t>::iterator it = tokens.begin();
	for (; it != tokens.end(); ++it, num_lines++) {
		line[0] = *it;
		++it;
		cppcut_assert_equal(false, it == tokens.end());
		line[1] = *it;
		assert_parse_record_data_output(line, tool_output_list);
	}
}

void
testutil::exec_record_data_tool(const char *arg, exec_command_info *exec_info)
{
	const gchar *cmd = "../src/cockroach-record-data-tool";
	const char *argv[] = {cmd, arg, NULL};
	exec_info->argv = argv;
	exec_info->save_stdout = true;
	exec_command(exec_info);
}

void testutil::reset_record_data(void)
{
	exec_command_info exec_info;
	exec_record_data_tool("reset", &exec_info);
}

string testutil::get_exit_info(int status)
{
	ostringstream ss;
	ss << "<< Exit information>>" << endl;
	if (WIFEXITED(status))
		ss << "Normal exit: Yes, code: " << WEXITSTATUS(status) << endl;
	else
		ss << "Normal exit: No" << endl;

	if (WIFSIGNALED(status)) {
		int signo = WTERMSIG(status);
		ss << "Signal: Yes, signal no.: " << signo << " ("
		   << get_signal_name(signo) << ")" << endl;
	} else
		ss << "Signal: No" << endl;
	return ss.str();
}

const string &testutil::get_signal_name(int signo)
{
	if (signal_name_map.empty()) {
		signal_name_map[SIGHUP]  = "SIGHUP";
		signal_name_map[SIGINT]  = "SIGINT";
		signal_name_map[SIGQUIT] = "SIGQUIT";
		signal_name_map[SIGILL]  = "SIGILL";
		signal_name_map[SIGTRAP] = "SIGTRAP";
		signal_name_map[SIGABRT] = "SIGABRT";
		signal_name_map[SIGBUS]  = "SIGBUS";
		signal_name_map[SIGFPE]  = "SIGFPE";
		signal_name_map[SIGKILL] = "SIGKILL";
		signal_name_map[SIGUSR1] = "SIGUSR1";
		signal_name_map[SIGSEGV] = "SIGSEGV";
		signal_name_map[SIGUSR2] = "SIGUSR2";
		signal_name_map[SIGPIPE] = "SIGPIPE";
		signal_name_map[SIGALRM] = "SIGALRM";
		signal_name_map[SIGTERM] = "SIGTERM";
		signal_name_map[SIGCHLD] = "SIGCHLD";
		signal_name_map[SIGCONT] = "SIGCONT";
		signal_name_map[SIGSTOP] = "SIGSTOP";
	}
	map<int,string>::iterator it = signal_name_map.find(signo);
	if (it == signal_name_map.end()) {
		static string unknown = "Unknown";
		return unknown;
	}
	return it->second;
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
