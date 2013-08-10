#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <errno.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

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

static int tkill(int tid, int signo)
{
	return syscall(SYS_tkill, tid, signo);
}

static string get_default_cockroach_lib_path(void)
{
	return "AHO";
}

static string get_child_code_string(int code)
{
	string str;
	switch(code) {
	case CLD_EXITED:
		str = "CLD_EXITED";
		break;
	case CLD_KILLED:
		str = "CLD_KILLED";
		break;
	case CLD_DUMPED:
		str = "CLD_DUMPED";
		break;
	case CLD_STOPPED:
		str = "CLD_STOPPED";
		break;
	case CLD_TRAPPED:
		str = "CLD_TRAPPED";
		break;
	case CLD_CONTINUED:
		str = "CLD_CONTINUED";
		break;
	default:
		stringstream ss;
		ss << "Unknown child code: " << code;
		str = ss.str();
		break;
	}
	return str;
}

static bool wait_signal(context *ctx)
{
	siginfo_t info;
	int result = waitid(P_PID, ctx->target_pid, &info, WEXITED|WSTOPPED);
	if (result == -1) {
		printf("Failed to wait for the stop of the target: %d: %s\n",
		       ctx->target_pid, strerror(errno));
		return false;
	}
	if (info.si_code != CLD_TRAPPED) {
		string code_string = get_child_code_string(info.si_code);
		printf("waitid() returned with the unexpected code: %s\n",
		       code_string.c_str());
		return false;
	}

	return true;
}

static bool attach(context *ctx)
{
	if (ptrace(PTRACE_ATTACH, ctx->target_pid, NULL, NULL)) {
		printf("Failed to attach the process: %d: %s\n",
		       ctx->target_pid, strerror(errno));
		return false;
	}

	// wait for the stop of the child
	if (!wait_signal(ctx))
		return false;

	return true;
}

static bool get_register_info(context *ctx)
{
	if (ptrace(PTRACE_GETREGS, ctx->target_pid, NULL, &ctx->regs)) {
		printf("Failed to get register info. target: %d: %s\n",
		       ctx->target_pid, strerror(errno));
		return false;
	}
	printf("RSP: %p, RIP: %p\n",
	       (void *)ctx->regs.rsp, (void *)ctx->regs.rip);
	return true;
}

static bool send_signal_and_wait(context *ctx)
{
	int signo = SIGUSR2;
	printf("Send signal: %d (tid: %d)\n", signo, ctx->target_pid);
	if (tkill(ctx->target_pid, signo) == -1) {
		printf("Failed to send signal. target: %d: %s. signo: %d\n",
		       ctx->target_pid, strerror(errno), signo);
		return false;
	}
	if (ptrace(PTRACE_CONT, ctx->target_pid, NULL, NULL) == -1) {
		printf("Failed: PTRACE_CONT. target: %d: %s. signo: %d\n",
		       ctx->target_pid, strerror(errno), signo);
		return false;
	}
	if (!wait_signal(ctx))
		return false;
	printf("  => stopped.\n");
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
	if (!send_signal_and_wait(&ctx))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
