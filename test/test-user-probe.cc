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
	exec_command_info exec_info;
	assert_func_base("sum 5", "15", &exec_info);
	//assert_func("sum 0", "15");
	//target_probe_info probe_info(exec_info.child_pid,
	//                             recipe_file, "sum_up_to");
	uint32_t id;
	size_t size;
	void *data;
	testutil::assert_get_record_data(&id, &size, &data);
}

}
