#include <cstdio>
#include <cstdlib>
#include <string>

using namespace std;

#define PRINT_USAGE_AND_EXIT_IF_THE_LAST(i, argc) \
do { \
	if (i == argc - 1) { \
		print_usage(); \
		return EXIT_FAILURE; \
	} \
} while(0)

static string get_default_cockroach_lib_path(void)
{
	return "AHO";
}

static void print_usage(void)
{
	printf("Usage\n");
	printf("\n");
	printf("$ cockroach-loader [options] --recipe recipe_path eID\n");
	printf("\n");
	printf("Options\n");
	printf("--cockroach-lib-path: The path of cockroach.so\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	string cockroach_lib_path;
	string recipe_path;
	pid_t  target_pid;
	for (int i = 1; i < argc; i++) {
		string arg = argv[i];
		if (arg == "--cockroach-lib-path") {
			PRINT_USAGE_AND_EXIT_IF_THE_LAST(i, argc);
			cockroach_lib_path = arg;

		} else if (arg == "--recipe") {
			PRINT_USAGE_AND_EXIT_IF_THE_LAST(i, argc);
			recipe_path = arg;
		} else
			target_pid = atoi(arg.c_str());
	}

	if (cockroach_lib_path.empty())
		cockroach_lib_path = get_default_cockroach_lib_path();

	printf("cockroach loader\n");
	printf("lib path    : %s\n", cockroach_lib_path.c_str());
	printf("recipe path : %s\n", recipe_path.c_str());
	printf("target pid  : %d\n", target_pid);

	return EXIT_SUCCESS;
}
