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
	const unsigned long arg_num = 5;
	string arg = (format("sum %ld") % arg_num).str();
	exec_command_info exec_info;
	assert_func_base(arg.c_str(), "15", &exec_info);

	// check the output
	record_data_tool_output tool_out;
	testutil::assert_get_record_data(&tool_out);
	cppcut_assert_equal(sizeof(user_record_t), tool_out.size);
	user_record_t *record
	  = reinterpret_cast<user_record_t *>(tool_out.data);
	cppcut_assert_equal(record->arg0, arg_num);
}

}
