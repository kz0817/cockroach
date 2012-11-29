#include <stdio.h>
#include <stdlib.h>

int funcX5(int a)
{
	return a * 5;
}

int main(int argc, char argv[])
{
	printf("Hello, World!\n");
	int x = funcX5(2);
	printf("X: %d\n", x);
	return EXIT_SUCCESS;
}
