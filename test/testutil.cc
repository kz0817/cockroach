#include "testutil.h"

#include <cppcutter.h>
#include <glib.h>

void testutil::reset_time_list(void)
{
	gboolean ret;
	const gchar *command_line = "../src/cockroach-time-measure-tool reset";
	gchar *std_output;
	gint exit_status;
	ret = g_spawn_command_line_sync(command_line, &std_output,
	                                NULL, &exit_status, NULL);
	cppcut_assert_equal(TRUE, ret);
	cppcut_assert_equal(0, EXIT_SUCCESS);
}
