#include <stdio.h>
#include <stdlib.h>

// test for rel 32bit probe
int sum_up_to(int max)
{
	int sum;
	int i = 0;
	for (i  = 1; i <= max; i++)
		sum += i;
	return sum;
}

// test for rel 32bit probe
int func1(int a, int b)
{
	return (a + b) * a;
}

int func1a(int a, int b)
{
	return (a + b) * a;
}

int func1b(int a, int b)
{
	return (a + b) * a;
}

// This can be used abs 64bit probe (bacause function size is larger than 12B)
int func2(int a, int b)
{
	return (a + b) * (a + a) * a / b;
}

