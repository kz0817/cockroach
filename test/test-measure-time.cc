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
	exec_command_info exec_info;
	testutil::exec_helper(recipe_file, "func1", &exec_info);
	cut_assert_equal_string("3", exec_info.stdout_str.c_str());
	testutil::assert_measured_time(1, exec_info.child_pid);
}

}
