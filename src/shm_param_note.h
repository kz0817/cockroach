#ifndef shm_param_note_h
#define shm_param_note_h

#include <string>
using namespace std;

class shm_param_note {
	bool   m_has_error;
	string m_recipe_file_path;
public:
	shm_param_note(void);
	virtual ~shm_param_note();

	void open(void);
	bool has_error(void) const;
	const string &get_recipe_file_path(void) const;
};

#endif

