#ifndef utils_h
#define utils_h

#include <vector>
#include <string>
using namespace std;

#include <stdarg.h>

#define ROACH_ABORT() utils::abort()

#define ROACH_DBG(fmt, ...) \
utils::message(__FILE__, __LINE__, "DBG", fmt, ##__VA_ARGS__)

#define ROACH_INFO(fmt, ...) \
utils::message(__FILE__, __LINE__, "INFO", fmt, ##__VA_ARGS__)

#define ROACH_ERR(fmt, ...) \
utils::message(__FILE__, __LINE__, "ERR", fmt, ##__VA_ARGS__)

#define ROACH_BUG(fmt, ...) \
do { \
  utils::message(__FILE__, __LINE__, "BUG", fmt, ##__VA_ARGS__); \
  ROACH_ABORT(); \
} while(0)

typedef void (*one_line_parser_t)(const char *line, void *arg);

struct cockroach_original_func_addr_table_t {
	int (*printf)(const char *format, ...);
	int (*vprintf)(const char *format, va_list ap); 
};

class utils {
	static cockroach_original_func_addr_table_t *m_original_func_table;

	static void *get_func_addr(const char *symbol);
public:
	static void init_original_func_addr_table(void);
	static cockroach_original_func_addr_table_t
	  *get_original_func_addr_table(void);
	static vector<string> split(const char *line, const char separator = ' ');
	static string get_basename(string &path);
	static string get_basename(const char *path);
	static bool is_absolute_path(const char *path);
	static bool is_hex_number(const char *word);
	static bool read_one_line_loop(const char *path,
	                               one_line_parser_t parser, void *arg);
	static int get_page_size(void);
	static void message(const char *file, int line, const char *header,
	                    const char *fmt, ...);
	static unsigned long calc_func_distance(void (*func0)(void),
	                                        void (*func1)(void));
	static void abort(void);
	static pid_t get_tid(void);
};

#endif
