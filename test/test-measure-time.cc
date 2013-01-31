#include <cstdio>
#include <cppcutter.h>
#include <glib.h>

#include <boost/format.hpp>
using namespace boost;

#include "testutil.h"
#include "cockroach-time-measure.h"

namespace test_measure_time {

static const char *g_default_recipe_file = "fixtures/test-measure-time.recipe";
static const char *g_recipe_file = NULL;
static int g_expected_num_lines = 0;

void setup(void)
{
	testutil::reset_time_list();
	g_recipe_file = g_default_recipe_file;
	g_expected_num_lines = 1;
}

void teardown(void)
{
}

static void assert_func_base(const char *arg, const char *expected_stdout,
                             exec_command_info *exec_info)
{
	testutil::run_target_exe(g_recipe_file, arg, exec_info);
	cut_assert_equal_string(expected_stdout, exec_info->stdout_str.c_str());
}

static void _assert_func(const char *arg, const char *expected_stdout)
{
	exec_command_info exec_info;
	assert_func_base(arg, expected_stdout, &exec_info);
	target_probe_info probe_info(exec_info.child_pid, g_recipe_file, arg);
	testutil::assert_measured_time(g_expected_num_lines, &probe_info);
}
#define assert_func(A,E) cut_trace(_assert_func(A,E))

static void assert_exec_sum_and_chk(size_t num_call = 1)
{
	// exec
	const unsigned long arg_num = 5;
	string expected_stdout;
	exec_command_info exec_info;
	string arg = (format("sum %ld %d") % arg_num % num_call).str();
	for (size_t i = 0; i < num_call; i++)
		expected_stdout += "15";
	assert_func_base(arg.c_str(), expected_stdout.c_str(), &exec_info);

	target_probe_info probe_info(exec_info.child_pid,
	                             g_recipe_file, "sum_up_to");
	testutil::assert_measured_time(num_call, &probe_info);
}


// rel 32bit
void test_save_instr_size_0(void)
{
	assert_func("func1", "3");
}

void test_no_save_instr_size(void)
{
	assert_func("func1a", "3");
}

void test_save_instr_size_6(void)
{
	assert_func("func1b", "3");
}

// abs 64bit
void test_abs64(void)
{
	assert_func("func2", "3");
}

// symbol in the executable
void test_target_in_exe(void)
{
	assert_func("funcX", "-30");
}

void test_call_many_times(void)
{
	const int shm_window_sz = 1024*1024;
	int num_call = 10 * shm_window_sz / MEASURED_TIME_SHM_SLOT_SIZE;
	char *env = getenv("LOOP_TEST_CALL_TIMES");
	if (env)
		num_call = strtol(env, NULL, 10);
	assert_exec_sum_and_chk(num_call);
}

// target_exe
void test_target_exe(void)
{
	g_recipe_file = "fixtures/test-measure-time-target-exe.recipe";
	test_save_instr_size_0();
}

void test_target_exe_different_name(void)
{
	g_recipe_file = "fixtures/test-measure-time-no-target-exe.recipe";
	g_expected_num_lines = 0;
	test_save_instr_size_0();
}

void test_target_exe_abs(void)
{
	g_recipe_file = "fixtures/test-measure-time-target-exe-abs.recipe";
	test_save_instr_size_0();
}

void test_target_exe_abs_different_name(void)
{
	g_recipe_file = "fixtures/test-measure-time-no-target-exe-abs.recipe";
	g_expected_num_lines = 0;
	test_save_instr_size_0();
}

// dlopen
void test_dlopen(void)
{
	assert_func("implicit_dlopener_3x", "6");
}

void test_dlopen_extlib(void)
{
	assert_func("implicit_open_target_2x", "4");
}

}
