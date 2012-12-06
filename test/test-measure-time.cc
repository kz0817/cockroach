#include <cstdio>
#include <cppcutter.h>
#include <glib.h>
#include "testutil.h"

namespace test_measure_time {

void setup(void)
{
	testutil::reset_time_list();
}

void teardown(void)
{
}

void test_func1(void)
{
	gboolean ret;
	const gchar *command_line = "./helper-bin func1";
	gchar *std_output;
	gint exit_status;
	ret = g_spawn_command_line_sync(command_line, &std_output,
	                                NULL, &exit_status, NULL);
	cppcut_assert_equal(TRUE, ret);
	cppcut_assert_equal(0, EXIT_SUCCESS);
	cut_assert_equal_string("3", std_output);
}

}
