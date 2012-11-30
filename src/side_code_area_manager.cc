#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <errno.h>
#include "utils.h"
#include "side_code_area_manager.h"

side_code_area::side_code_area(unsigned long _start_addr, unsigned long _length)
: start_addr(_start_addr),
  length(_length),
  idx(0)
{
}

uint8_t *side_code_area::get_head_addr(void)
{
	return reinterpret_cast<uint8_t *>(start_addr + idx);
}

bool cmp_side_code_area::operator()(const side_code_area *a,
                                    const side_code_area *b) const
{
	return a->start_addr < b->start_addr;
}

// static member
side_code_area *side_code_area_manager::m_curr_area = NULL;

side_code_area_set_t &side_code_area_manager::get_side_code_area_set(void)
{
	static side_code_area_set_t side_code_area_set;
	return side_code_area_set;
}

uint8_t *side_code_area_manager::alloc(size_t size)
{
	uint8_t *ret;
	// TODO: be MT safe
	if (m_curr_area && (m_curr_area->length - m_curr_area->idx >= size)) {
		ret = m_curr_area->get_head_addr();
		m_curr_area->idx += size;
		return ret;
	}

	int page_size = utils::get_page_size();
	int map_size = (size + page_size - 1) / page_size;
	map_size *= page_size;
	int prot = PROT_EXEC|PROT_READ|PROT_WRITE;
	void *ptr = mmap(NULL, map_size, prot,
	                 MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED) {
		ROACH_ERR("Failed to mmap: %d\n", errno);
		abort();
	}

	unsigned long addr = reinterpret_cast<unsigned long>(ptr);
	side_code_area *area = new side_code_area(addr, map_size);
	get_side_code_area_set().insert(area);
	m_curr_area = area;

	ret = m_curr_area->get_head_addr();
	m_curr_area->idx += size;
	return ret;
}
