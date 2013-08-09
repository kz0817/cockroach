#include <cstdio>
#include <cstdlib>
#include <string>
#include <errno.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/user.h>

using namespace std;

struct context {
	string cockroach_lib_path;
	string recipe_path;
	pid_t  target_pid;
	struct user_regs_struct regs;

	context(void)
	: target_pid(0)
	{
	}
};

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

static bool attach(context *ctx)
{
	if (ptrace(PTRACE_ATTACH, ctx->target_pid, NULL, NULL)) {
		printf("Failed to attach the process: %d: %s\n",
		       ctx->target_pid, strerror(errno));
		return false;
	}

	// wait for the stop of the child
	siginfo_t info;
	int result = waitid(P_PID, ctx->target_pid, &info, WSTOPPED);
	if (result == -1) {
		printf("Failed to wait for the stop of the target: %d: %s\n",
		       ctx->target_pid, strerror(errno));
		return false;
	}
	if (info.si_code != CLD_TRAPPED) {
		printf("waitid() returned with the unexpected code: %d\n",
		       info.si_code);
		return false;
	}

	return true;
}

static bool get_register_info(context *ctx)
{
	if (ptrace(PTRACE_GETREGS, ctx->target_pid, NULL, &ctx->regs)) {
		printf("Failed to get register info: target: %d: %s\n",
		       ctx->target_pid, strerror(errno));
		return false;
	}
	printf("RSP: %p, RIP: %p\n",
	       (void *)ctx->regs.rsp, (void *)ctx->regs.rip);
	return true;
}

static void print_usage(void)
{
	printf("Usage\n");
	printf("\n");
	printf("$ cockroach-loader [options] --recipe recipe_path pid\n");
	printf("\n");
	printf("Options\n");
	printf("  --cockroach-lib-path: The path of cockroach.so\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	context ctx;
	for (int i = 1; i < argc; i++) {
		string arg = argv[i];
		if (arg == "--cockroach-lib-path") {
			PRINT_USAGE_AND_EXIT_IF_THE_LAST(i, argc);
			ctx.cockroach_lib_path = arg;

		} else if (arg == "--recipe") {
			PRINT_USAGE_AND_EXIT_IF_THE_LAST(i, argc);
			ctx.recipe_path = arg;
		} else
			ctx.target_pid = atoi(arg.c_str());
	}

	if (ctx.cockroach_lib_path.empty())
		ctx.cockroach_lib_path = get_default_cockroach_lib_path();
	if (ctx.target_pid == 0) {
		printf("ERROR: You have to specify target PID.\n\n");
		print_usage();
		return EXIT_FAILURE;
	}

	printf("cockroach loader\n");
	printf("lib path    : %s\n", ctx.cockroach_lib_path.c_str());
	printf("recipe path : %s\n", ctx.recipe_path.c_str());
	printf("target pid  : %d\n", ctx.target_pid);

	if (!attach(&ctx))
		return EXIT_FAILURE;
	if (!get_register_info(&ctx))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
