#include <cstdio>
#include <cstdlib>
using namespace std;

#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h> 
#include <syscall.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include "utils.h"

vector<string> utils::split(const char *line, const char separator)
{
	vector<string> tokens;

	// seek to the non-space charactor
	string str;
	for (const char *ptr = line; *ptr != '\0'; ptr++) {
		if (*ptr == '\n' || *ptr == '\r')
			break;
		if (*ptr == separator) {
			if (str.size() > 0) {
				tokens.push_back(str);
				str.erase();
			} else {
				; // just ignore
			}
		} else {
			str += *ptr;
		}
	}
	if (str.size() > 0)
		tokens.push_back(str);

	return tokens;
}

string utils::get_basename(const char *path)
{
	string str(path);
	return get_basename(str);
}

string utils::get_basename(string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == string::npos)
		return path;
	return path.substr(pos+1);
}

bool utils::is_absolute_path(const char *path)
{
	if (!path)
		return false;
	return (path[0] == '/');
}

bool utils::is_absolute_path(string &path)
{
	return is_absolute_path(path.c_str());
}

bool utils::read_one_line_loop(const char *path,
                               one_line_parser_t parser, void *arg)
{
	FILE *fp = fopen(path, "r");
	if (!fp) {
		return false;
	}

	static const int MAX_CHARS_ONE_LINE = 4096;
	char line[MAX_CHARS_ONE_LINE];
	while (true) {
		char *ret = fgets(line, MAX_CHARS_ONE_LINE, fp);
		if (!ret)
			break;
		(*parser)(line, arg);
	}
	fclose(fp);
	return true;
}

bool utils::is_hex_number(const char *word)
{
	for (const char *ptr = word; *ptr; ptr++) {
		if (*ptr < '0')
			return false;
		if (*ptr > 'f')
			return false;
		if (*ptr > '9' && *ptr < 'a')
			return false;
	}
	return true;
}

bool utils::is_hex_number(string &word)
{
	return is_hex_number(word.c_str());
}

int utils::get_page_size(void)
{
	static int s_page_size = 0;
	if (!s_page_size)
		s_page_size = sysconf(_SC_PAGESIZE);
	return s_page_size;
}

void utils::message(const char *file, int line, const char *header,
                    const char *fmt, ...)
{
	FILE *fp = stderr;
	fprintf(fp, "[%s] <%s:%d> ", header, file, line);
	va_list ap;
	va_start(ap, fmt); 
	vfprintf(fp, fmt, ap);
	va_end(ap);
}

unsigned long
utils::calc_func_distance(void (*func0)(void), void (*func1)(void))
{
	return (unsigned long)func1 - (unsigned long)func0;
}

void utils::abort(void)
{
#if defined(__x86_64__) || defined(__i386__)
	asm volatile("int $3");
#else
#error "Not implemented"
#endif
}

pid_t utils::get_tid(void)
{
	static __thread pid_t g_tls_thread_id = 0;
	if (g_tls_thread_id ==  0)
		g_tls_thread_id = syscall(SYS_gettid);
	return g_tls_thread_id;
}

string utils::get_self_exe_name(void)
{
	const static int BUF_LEN = PATH_MAX;
	char buf[BUF_LEN];
	ssize_t len = readlink("/proc/self/exe", buf, BUF_LEN);
	if (len == -1) {
		ROACH_ERR("Failed to readlink(\"/proc/self/exe\"): %d\n",
		          errno);
		ROACH_ABORT();
	}
	return string(buf, len);
}
