#ifndef shm_param_note_h
#define shm_param_note_h

#include <string>
using namespace std;

class shm_param_note {
	static const int LEN_RECIPE_FILE_PATH_SZ;

	bool   m_has_error;
	string m_recipe_file_path;

	string get_shm_name(pid_t pid = 0);
	bool shm_write(int fd, const void *buf, size_t count);
	bool shm_read(int fd, void *buf, size_t count);
public:
	shm_param_note(void);
	virtual ~shm_param_note();

	void open(void);
	bool remove(void);
	bool has_error(void) const;
	bool create(const string &recipe_file_path, pid_t target_pid);
	const string &get_recipe_file_path(void) const;
};

#endif

