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
	string std_out = testutil::exec_helper(recipe_file, "func1");
	cut_assert_equal_string("3", std_out.c_str());
	testutil::assert_measured_time(1);
}

}
