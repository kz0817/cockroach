#include <cstdio>
#include <cppcutter.h>
#include <glib.h>

#include <boost/format.hpp>
using namespace boost;

#include "testutil.h"
#include "user-probe.h"

namespace test_user_probe {

static const char *recipe_file = "fixtures/test-user-probe.recipe";

void setup(void)
{
}

void teardown(void)
{
}

static void
assert_func_base(const char *arg, const char *expected_stdout,
                 exec_command_info *exec_info)
{
	testutil::run_target_exe(recipe_file, arg, exec_info);
	cut_assert_equal_string(expected_stdout, exec_info->stdout_str.c_str());
}

static void assert_func(const char *arg, const char *expected_stdout)
{
	exec_command_info exec_info;
	assert_func_base(arg, expected_stdout, &exec_info);
}

static void assert_exec_data_record(size_t num_call = 1)
{
	// exec
	const unsigned long arg_num = 5;
	string expected_stdout;
	exec_command_info exec_info;
	string arg = (format("sum %ld %d") % arg_num % num_call).str();
	for (size_t i = 0; i < num_call; i++)
		expected_stdout += "15";
	assert_func_base(arg.c_str(), expected_stdout.c_str(), &exec_info);

	// check the output
	list<record_data_tool_output> tool_output_list;
	testutil::assert_get_record_data(tool_output_list);
	cppcut_assert_equal(num_call, tool_output_list.size());

	list<record_data_tool_output>::iterator it = tool_output_list.begin();
	for (; it != tool_output_list.end(); ++it) {
		record_data_tool_output &tool_out = *it;
		cppcut_assert_equal(sizeof(user_record_t), tool_out.size);
		user_record_t *record
		  = reinterpret_cast<user_record_t *>(tool_out.data);
		cppcut_assert_equal(arg_num, record->arg0);
	}
}

//
// Tests
//
void test_user_probe(void)
{
	assert_func("func1", "3");
}

void test_user_probe_with_init(void)
{
	assert_func("func1b", "3");
}

void test_save_instr_size_6(void)
{
	assert_func("func1b", "3");
}

void test_data_record(void)
{
	testutil::reset_record_data();
	assert_exec_data_record();
}

void test_data_record_many_times(void)
{
	int shm_window_sz = 1024*1024;
	int num_call = 10 * shm_window_sz / sizeof(user_record_t);
	char *env = getenv("LOOP_TEST_CALL_TIMES");
	if (env)
		num_call = strtol(env, NULL, 10);
	testutil::reset_record_data();
	assert_exec_data_record(num_call);
}

}
