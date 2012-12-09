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

void test_func1(void)
{
	const char *target_func = "func1";
	exec_command_info exec_info;
	testutil::exec_test_helper(recipe_file, target_func, &exec_info);
	cut_assert_equal_string("3", exec_info.stdout_str.c_str());

	target_probe_info probe_info;
	probe_info.pid = exec_info.child_pid;
	probe_info.recipe_file = recipe_file;
	probe_info.target_func = target_func;
	testutil::assert_measured_time(1, &probe_info);
}

}
