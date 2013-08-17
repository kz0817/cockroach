#include <cstdio>
#include <cppcutter.h>

#include "test-disassembler-target.h"
#include "disassembler.h"

namespace test_disassember {

opecode *g_ope = NULL;

void setup(void)
{
}

void teardown(void)
{
	if (g_ope) {
		delete g_ope;
		g_ope = NULL;
	}
}

static void _assert_opecode(
  opecode *ope, int expected_len, mod_type mod, register_type reg,
  register_type r_m = REG_NONE)
{
	cppcut_assert_equal(mod, g_ope->get_mod_rm().mod);
	cppcut_assert_equal(reg, g_ope->get_mod_rm().reg);
	cppcut_assert_equal(r_m, g_ope->get_mod_rm().r_m);
	cppcut_assert_equal(expected_len, g_ope->get_length());
}
#define assert_opecode(OPE, LEN, MOD_TYPE, REG, ...) \
cut_trace(_assert_opecode(OPE, LEN, MOD_TYPE, REG, ##__VA_ARGS__))

// ---------------------------------------------------------------------------
// Test code
// ---------------------------------------------------------------------------
#if defined(__x86_64__) || defined(__i386__)

void test_mov_Ev_Iz(void)
{
	g_ope = disassembler::parse((uint8_t *)target_mov_Ev_Iz);

	// movl   $0x5,0x18(%rsi)
	// c7 46 18 05 00 00 00
	cppcut_assert_equal(MOD_REG_INDIRECT_DISP8, g_ope->get_mod_rm().mod);
	cppcut_assert_equal(REG_SI, g_ope->get_mod_rm().r_m);
	cppcut_assert_equal(DISP8, g_ope->get_disp().type);
	cppcut_assert_equal((uint32_t)0x18, g_ope->get_disp().value);
	cppcut_assert_equal(IMM32, g_ope->get_immediate().type);
	cppcut_assert_equal((uint64_t)0x5, g_ope->get_immediate().value);
	cppcut_assert_equal(7, g_ope->get_length());
}

void test_mov_Gv_Ev(void)
{
	g_ope = disassembler::parse((uint8_t *)target_mov_Gv_Ev);

	// [x86_64]
	// mov    0x8(%rbp),%rdx
	// 48 8b 55 08

	// [i386]
	// mov    0x8(%ebp),%edx
	// 8b 55 08 

	cppcut_assert_equal(MOD_REG_INDIRECT_DISP8, g_ope->get_mod_rm().mod);
	cppcut_assert_equal(REG_BP, g_ope->get_mod_rm().r_m);
	cppcut_assert_equal(DISP8, g_ope->get_disp().type);
	cppcut_assert_equal((uint32_t)0x8, g_ope->get_disp().value);
	cppcut_assert_equal(REG_DX, g_ope->get_mod_rm().reg);
#if __x86_64__
	int expected_len = 4;
#endif
#if __i386__
	int expected_len = 3;
#endif
	cppcut_assert_equal(expected_len, g_ope->get_length());
}

void test_mov_pop_rAX_r8(void)
{
	g_ope = disassembler::parse((uint8_t *)target_pop_rAX_r8);
	assert_opecode(g_ope, 1, MOD_REG_NONE, REG_AX);
}

void test_mov_pop_rBP_r8(void)
{
	g_ope = disassembler::parse((uint8_t *)target_pop_rBP_r13);
	assert_opecode(g_ope, 1, MOD_REG_NONE, REG_BP);
}

#endif // defined(__x86_64__) || defined(__i386__)

} // namespace test_disassember

