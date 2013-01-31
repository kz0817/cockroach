#include "implicitopentarget.h"

int implicit_dlopener_extlib_2x(int a)
{
	return implicit_open_target_2x(a);
}

int implicit_dlopener_3x(int a)
{
	asm volatile("push %rax");
	asm volatile("pop %rax");
	return 3 * a;
}
