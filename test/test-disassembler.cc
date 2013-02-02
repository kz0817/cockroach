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
#ifdef __x86_64__

void test_mov_Ev_Iz(void)
{
	g_ope = disassembler::parse((uint8_t *)target_mov_Ev_Iz);
	cppcut_assert_equal(7, g_ope->get_length());
}

#endif //  __x86_64__

} // namespace test_disassember

