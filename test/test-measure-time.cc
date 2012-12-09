#include <cstdio>
#include <cppcutter.h>
#include <glib.h>
#include "testutil.h"

namespace test_measure_time {

static const char *recipe_file = "fixtures/test-measure-time.recipe";

void setup(void)
{
	testutil::reset_time_list();
}

void teardown(void)
{
}

static void assert_func(const char *target_func, const char *expected_stdout)
{
	exec_command_info exec_info;
	testutil::exec_test_helper(recipe_file, target_func, &exec_info);
	cut_assert_equal_string(expected_stdout, exec_info.stdout_str.c_str());

	target_probe_info probe_info(exec_info.child_pid,
	                             recipe_file, target_func);
	testutil::assert_measured_time(1, &probe_info);
}

// rel 32bit
void test_func1(void)
{
	assert_func("func1", "3");
}

// abs 64bit
void test_func1x(void)
{
	assert_func("func1x", "3");
}

}
