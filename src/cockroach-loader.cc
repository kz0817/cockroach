#include <cstdio>
#include <cstdlib>
#include <string>
#include <set>
#include <vector>
#include <sstream>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

using namespace std;

typedef set<pid_t>              thread_id_set;
typedef thread_id_set::iterator thread_id_set_itr;
typedef vector<uint8_t>         code_block;

struct context {
	string cockroach_lib_path;
	string recipe_path;
	pid_t  target_pid;
	unsigned long install_trap_addr;
	unsigned long install_trap_orig_code;
	thread_id_set thread_ids;

	context(void)
	: target_pid(0),
	  install_trap_addr(0)
	{
	}
};

#define TRAP_INSTRUCTION 0xcc
#define TRAP_INSTRUCTION_SIZE 1
#define WAIT_ALL -1

#define PRINT_USAGE_AND_EXIT_IF_THE_LAST(i, argc) \
do { \
	if (i == argc - 1) { \
		print_usage(); \
		return EXIT_FAILURE; \
	} \
} while(0)

#define INC_INDEX_AND_UPDATE_ARG(i, argv, arg) \
do { \
	i++; \
	arg = argv[i]; \
} while(0)

#ifdef __x86_64__
static unsigned long get_program_counter(struct user_regs_struct *regs)
{
	return regs->rip;
}

void _cockroach_launcher_template(void)
{
	asm volatile("cockroach_launcher_template_begin:");
	asm volatile("cockroach_launcher_template_set_1st_arg:");
	asm volatile("mov $0xfedcba987654321,%rdi"); // filename
	asm volatile("cockroach_launcher_template_set_2nd_arg:");
	asm volatile("mov $0xfedcba987654321,%rsi"); // flags
	asm volatile("call *0x1(%rip)");             // call dlopen()
	asm volatile("int $3");
	asm volatile("cockroach_launcher_template_data:");
	// The address of dlopen()      [8byte]
	// The file name (cockroach.so)
	asm volatile("cockroach_launcher_template_end:");
}
extern "C" void cockroach_launcher_template_begin(void);
extern "C" void cockroach_launcher_template_set_1st_arg(void);
extern "C" void cockroach_launcher_template_set_2nd_arg(void);
extern "C" void cockroach_launcher_template_data(void);

static void get_launcher_code(code_block &code)
{
}

static unsigned long get_cockroach_launcher_addr(void)
{
	return 0x400000;
}
#endif // __x86_64__

/*
static int tkill(int tid, int signo)
{
	return syscall(SYS_tkill, tid, signo);
}*/

static string get_default_cockroach_lib_path(void)
{
	return "cockroach.so";
}

/*
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
}*/

static bool wait_signal(pid_t pid, int *status = NULL,
                        pid_t *changed_pid = NULL)
{
	int status_local;
	if (!status)
		status = &status_local;
	pid_t result = waitpid(pid, status, __WALL);
	if (result == -1) {
		printf("Failed to wait for the stop of the target: %d: %s\n",
		       pid, strerror(errno));
		return false;
	}
	if (changed_pid)
		*changed_pid = result;
	printf("wait result: target: %d, changed: %d, status: %08x (sig: %d)\n",
	       pid, result, *status, WSTOPSIG(*status));

	return true;
}

static bool attach(pid_t tid)
{
	if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) == -1) {
		printf("Failed to attach the process: %d: %s\n",
		       tid, strerror(errno));
		return false;
	}
	printf("Attached: %d\n", tid);
	return true;
}

static bool attach_all_threads(context *ctx)
{
	// get all pids of threads
	stringstream proc_task_path;
	proc_task_path << "/proc/";
	proc_task_path << ctx->target_pid;
	proc_task_path << "/task";
	DIR *dir = opendir(proc_task_path.str().c_str());
	if (!dir) {
		printf("Failed: open dir: %s: %s\n",
		       proc_task_path.str().c_str(), strerror(errno));
		return false;
	}

	errno = 0;
	while (true) {
		struct dirent *entry = readdir(dir);
		if (!entry) {
			if (!errno)
				break;
			printf("Failed: read dir entry: %s: %s\n",
			       proc_task_path.str().c_str(), strerror(errno));
			if (closedir(dir) == -1) {
				printf("Failed: close dir: %s: %s\n",
				       proc_task_path.str().c_str(),
				       strerror(errno));
			}
			return false;
		}
		string dir_name(entry->d_name);
		if (dir_name == "." || dir_name == "..")
			continue;
		int tid = atoi(entry->d_name); printf("Found thread: %d\n", tid);
		ctx->thread_ids.insert(tid);
	}
	if (closedir(dir) == -1) {
		printf("Failed: close dir: %s: %s\n",
		       proc_task_path.str().c_str(), strerror(errno));
		return false;
	}

	// attach all threads
	int status;
	pid_t changed_pid;
	thread_id_set_itr tid = ctx->thread_ids.begin();
	for (; tid != ctx->thread_ids.end(); ++tid) {
		// The main thread has already been attached
		if (*tid == ctx->target_pid)
			continue;
		if (!attach(*tid))
			return false;
		do {
			if (!wait_signal(*tid, &status, &changed_pid))
				return false;
		} while (changed_pid != *tid);
	}

	return true;
}

static bool get_register_info(pid_t pid, struct user_regs_struct *regs)
{
	if (ptrace(PTRACE_GETREGS, pid, NULL, regs) == -1) {
		printf("Failed to get register info. target: %d: %s\n",
		       pid, strerror(errno));
		return false;
	}
	return true;
}

/*
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
	int status;
	if (!wait_signal(ctx, &status))
		return false;
	printf("  => stopped. (%d)\n", status);
	return true;
}*/

static bool install_trap(context *ctx)
{
	// get the original code where the trap instruction is installed.
	void *addr = (void *)ctx->install_trap_addr;
	errno = 0;
	long orig_code = ptrace(PTRACE_PEEKTEXT, ctx->target_pid, addr, NULL);
	if (errno && orig_code == -1) {
		printf("Failed to PEEK_TEXT. target: %d: %s\n",
		       ctx->target_pid, strerror(errno));
		return false;
	}
	printf("code @ install-trap addr: %lx @ %p\n", orig_code, addr);

	// install trap
	long trap_code = orig_code;
	*((uint8_t *)&trap_code) = TRAP_INSTRUCTION;
	if (ptrace(PTRACE_POKETEXT, ctx->target_pid, addr, trap_code) == -1) {
		printf("Failed to POKETEXT. target: %d: %s\n",
		       ctx->target_pid, strerror(errno));
		return false;
	}

	ctx->install_trap_orig_code = orig_code;

	return true;
}

static bool resume_thread(pid_t tid, int signo = 0)
{
	if (ptrace(PTRACE_CONT, tid, NULL, signo) == -1) {
		printf("Failed: PTRACE_CONT. target: %d: %s.\n",
		       tid, strerror(errno));
		return false;
	}
	printf("Resumed: %d (signo: %d)\n", tid, signo);
	return true;
}

static bool resume_all_thread(context *ctx)
{
	thread_id_set_itr tid = ctx->thread_ids.begin();
	for (; tid != ctx->thread_ids.end(); ++tid) {
		if (!resume_thread(*tid))
			return false;
	}
	return true;
}

static bool replace_one_word(context *ctx, pid_t pid,
                             unsigned long addr, unsigned long data)
{
	if (ptrace(PTRACE_POKETEXT, pid, addr, data) == -1) {
		printf("Failed to POKETEXT. target: %d: %s, "
		       "addr: %lx, data: %lx\n",
		       pid, strerror(errno), addr, data);
		return false;
	}
	return true;
}

static bool install_cockroach(context *ctx, pid_t pid)
{
	unsigned long launcher_addr = get_cockroach_launcher_addr();
	code_block launcher_code;
	get_launcher_code(launcher_code);
	printf("Install cockroach. launcher: %lx, code size: %zd\n",
	       launcher_addr, launcher_code.size());

	const size_t word_size = sizeof(long);
	size_t written_size = 0;
	while (written_size < launcher_code.size()) {
		unsigned long addr = launcher_addr + written_size;
		unsigned long data = 0;
		for (size_t i = 0; i < word_size; i++) {
			uint8_t *data8 = (uint8_t *)&data;
			data8[i] = launcher_code[written_size + i];
		}
		if (!replace_one_word(ctx, pid, addr, data))
			return false;
		written_size += word_size;
	}
	return true;
}

static bool is_install_trap(context *ctx, pid_t pid, bool *is_expected)
{
	*is_expected = false;
	struct user_regs_struct regs;
	if (!get_register_info(pid, &regs))
		return false;
	unsigned long program_counter = get_program_counter(&regs);
	unsigned long expected_addr =
	   ctx->install_trap_addr + TRAP_INSTRUCTION_SIZE;
	if (program_counter != expected_addr) {
		printf("Unexpected break point: %lx\n",
		       program_counter);
		return true;
	}

	*is_expected = true;
	return true;
}

static bool wait_install_trap(context *ctx)
{
	int status;
	pid_t changed_pid;
	while (true) {
		printf("waiting for SIGTRAP.\n");
		int signo = 0;
		if (!wait_signal(WAIT_ALL, &status, &changed_pid))
			return false;
		if (WIFSTOPPED(status)) {
			signo = WSTOPSIG(status);
			if (signo == SIGTRAP) {
				bool expected = false;
				pid_t pid = changed_pid;
				if (!is_install_trap(ctx, pid, &expected))
					return false;
				if (expected)
					break;
			}
		} else if (WIFEXITED(status)) {
			int exit_code = WEXITSTATUS(status);
			printf("The target exited: exit code: %d\n", exit_code);
			return false;
		} else if (WIFSIGNALED(status)) {
			int exit_signo = WTERMSIG(status);
			printf("The target exited by signal: %d\n", exit_signo);
			return false;
		} else {
			printf("Unexpected wait result.\n");
			return false;
		}

		// resume the thread with the signal injection
		if (!resume_thread(changed_pid, signo))
			return false;
	}

	return install_cockroach(ctx, changed_pid);
}

static void print_usage(void)
{
	printf("Usage\n");
	printf("\n");
	printf("$ cockroach-loader [options] --recipe recipe_path pid\n");
	printf("\n");
	printf("Options\n");
	printf("  --install-trap-addr:\n");
	printf("    The hex. address for the installation trap.\n");
	printf("  --cockroach-lib-path:\n");
	printf("     The path of cockroach.so\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	context ctx;
	for (int i = 1; i < argc; i++) {
		string arg = argv[i];
		if (arg == "--cockroach-lib-path") {
			PRINT_USAGE_AND_EXIT_IF_THE_LAST(i, argc);
			INC_INDEX_AND_UPDATE_ARG(i, argv, arg);
			ctx.cockroach_lib_path = arg;

		} else if (arg == "--install-trap-addr") {
			PRINT_USAGE_AND_EXIT_IF_THE_LAST(i, argc);
			INC_INDEX_AND_UPDATE_ARG(i, argv, arg);
			sscanf(arg.c_str(), "%lx", &ctx.install_trap_addr);
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

	printf("cockroach loader (build:  %s %s)\n", __DATE__, __TIME__);
	printf("lib path    : %s\n", ctx.cockroach_lib_path.c_str());
	printf("recipe path : %s\n", ctx.recipe_path.c_str());
	printf("target pid  : %d\n", ctx.target_pid);
	printf("install trap: %lx\n", ctx.install_trap_addr);

	// attach only the main thread to recieve SIGCHLD
	if (!attach(ctx.target_pid))
		return EXIT_FAILURE;
	// wait for the stop of the child
	if (!wait_signal(ctx.target_pid))
		return false;

	if (!attach_all_threads(&ctx))
		return EXIT_FAILURE;
	if (!install_trap(&ctx))
		return EXIT_FAILURE;
	if (!resume_all_thread(&ctx))
		return EXIT_FAILURE;
	if (!wait_install_trap(&ctx))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
