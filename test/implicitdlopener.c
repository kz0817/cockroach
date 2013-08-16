#include "implicitopentarget.h"

int implicit_dlopener_extlib_2x(int a)
{
	return implicit_open_target_2x(a);
}

#ifdef __x86_64__
int implicit_dlopener_3x(int a)
{
	asm volatile("push %rax");
	asm volatile("pop %rax");
	return 3 * a;
}
#endif // __x86_64__

#ifdef __i386__
int implicit_dlopener_3x(int a)
{
	asm volatile("push %eax");
	asm volatile("pop %eax");
	return 3 * a;
}
#endif // __i386__
