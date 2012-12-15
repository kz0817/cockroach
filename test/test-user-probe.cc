#include <cstdio>
#include <cppcutter.h>
#include <glib.h>
#include "testutil.h"

namespace test_user_probe {

static const char *recipe_file = "fixtures/test-user-probe.recipe";

void setup(void)
{
}

void teardown(void)
{
}

static void assert_func(const char *target_func, const char *expected_stdout)
{
	exec_command_info exec_info;
	testutil::run_target_exe(recipe_file, target_func, &exec_info);
	cut_assert_equal_string(expected_stdout, exec_info.stdout_str.c_str());
}

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
	assert_func("sum 5", "15");
	//assert_func("sum 0", "15");
	
	/*target_probe_info probe_info(exec_info.child_pid,
	                             recipe_file, target_func);
	testutil::assert_measured_time(1, &probe_info);
	*/
}


}
