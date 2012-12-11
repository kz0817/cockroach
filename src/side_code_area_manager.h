#ifndef side_code_area_manager_h
#define side_code_area_manager_h

#include <set>
using namespace std;

#include <stdint.h>
#include <pthread.h>

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
	static pthread_mutex_t m_mutex;
	static side_code_area_set_t &get_side_code_area_set(void);
	static side_code_area *m_curr_area;

#ifdef __x86_64__
	static bool is_within_rel32(unsigned long addr, unsigned long ref_addr);
	static unsigned long
	find_region_within_rel32(unsigned long addr0, unsigned long addr1,
	                         unsigned long ref_addr, size_t region_size);
#endif // __x86_64__
	static bool extract_address(string &line,
	                            unsigned long &addr0, unsigned long &addr1);
	static unsigned long get_page_boundary_ceil(unsigned long addr);
	static uint8_t *alloc_region(size_t region_size,
	                             size_t reserve_size = 0,
	                             void *request_addr = NULL);
public:
	static uint8_t *alloc(size_t size);
#ifdef __x86_64__
	static uint8_t *alloc_within_rel32(size_t size, unsigned long ref_addr);
#endif // __x86_64__
};

#endif
