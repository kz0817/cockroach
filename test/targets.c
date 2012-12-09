#include <stdio.h>
#include <stdlib.h>

// test for rel 32bit probe
int func1(int a, int b)
{
	return (a + b) * a;
}

// test for abs 64bit probe
int func1x(int a, int b)
{
	return (a + b) * a;
}
