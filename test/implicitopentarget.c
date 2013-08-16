#ifdef __x86_64__
int implicit_open_target_2x(int a)
{
	asm volatile("push %rax");
	asm volatile("pop %rax");
	return a * 2;
}
#endif // __x86_64__

#ifdef __i386__
int implicit_open_target_2x(int a)
{
	asm volatile("push %eax");
	asm volatile("pop %eax");
	return a * 2;
}
#endif // __i386__
