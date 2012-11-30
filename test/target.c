#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int funcX5(int a)
{
	return a * 5;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		return EXIT_SUCCESS;
	char *first_arg = argv[1];

	if (strcmp(first_arg, "hello") == 0) {
		printf("Hello, World!\n");
	} else if (strcmp(first_arg, "funcX5") == 0) {
		int x = funcX5(2);
		printf("%d\n", x);
	}
	return EXIT_SUCCESS;
}
