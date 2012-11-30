#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int funcX(int a, int b)
{
	//asm volatile("nop");
	asm volatile("int $3");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	return a * b;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		return EXIT_SUCCESS;
	char *first_arg = argv[1];

	if (strcmp(first_arg, "hello") == 0) {
		printf("Hello, World!\n");
	} else if (strcmp(first_arg, "funcX") == 0) {
		int x = funcX(2, 3);
		printf("%d\n", x);
	} else if (strcmp(first_arg, "memcpy") == 0) {
		int dest[2] = {0, 0};
		int src[2] = {0xff, 0xaa};
		memcpy(dest, src, 2);
		printf("copied: %02x %02x\n", dest[0], dest[1]);
	}
	else if (strcmp(first_arg, "hello2") == 0) {
		int count = 0;
		printf("Hello, World: %d\n", count++);
		printf("Hello, World: %d\n", count++);
	}
	return EXIT_SUCCESS;
}
