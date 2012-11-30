#ifndef side_code_area_manager_h
#define side_code_area_manager_h

#include <set>
using namespace std;

#include <stdint.h>

struct side_code_area {
	unsigned long start_addr;
	unsigned long length;
	unsigned long idx;

	side_code_area(unsigned long _start_addr, unsigned long _length);
	uint8_t *get_head_addr(void);
};

struct cmp_side_code_area
{
	bool operator()(const side_code_area *a, const side_code_area *b) const;
};

typedef set<side_code_area *, cmp_side_code_area> side_code_area_set_t;
typedef side_code_area_set_t::iterator side_code_area_set_itr;

class side_code_area_manager {
	static side_code_area_set_t &get_side_code_area_set(void);
	static side_code_area *m_curr_area;
public:
	static uint8_t *alloc(size_t size);
};

#endif
