int implicit_open_target_2x(int a)
{
	asm volatile("push %rax");
	asm volatile("pop %rax");
	return a * 2;
}
