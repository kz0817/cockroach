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
	cppcut_assert_equal(MOD_REG_NONE, g_ope->get_mod_rm().mod);
	cppcut_assert_equal(REG_AX, g_ope->get_mod_rm().reg);
	cppcut_assert_equal(1, g_ope->get_length());
}

#endif // defined(__x86_64__) || defined(__i386__)

} // namespace test_disassember

