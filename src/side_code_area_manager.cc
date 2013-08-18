#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sys/mman.h>
#include <errno.h>
#include "utils.h"
#include "side_code_area_manager.h"

static const unsigned long REGION_NOT_FOUND = 0;
static const unsigned long REGION_START_ADDR = 0x10000;

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

//
// static member
//
side_code_area *side_code_area_manager::m_curr_area = NULL;
pthread_mutex_t side_code_area_manager::m_mutex = PTHREAD_MUTEX_INITIALIZER;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
side_code_area_set_t &side_code_area_manager::get_side_code_area_set(void)
{
	static side_code_area_set_t side_code_area_set;
	return side_code_area_set;
}

uint8_t *side_code_area_manager::alloc(size_t size)
{
	uint8_t *ret;
	pthread_mutex_lock(&m_mutex);
	if (m_curr_area && (m_curr_area->length - m_curr_area->idx >= size)) {
		ret = m_curr_area->get_head_addr();
		m_curr_area->idx += size;
		pthread_mutex_unlock(&m_mutex);
		return ret;
	}
	pthread_mutex_unlock(&m_mutex);

	size_t region_size = utils::get_page_size();
	return alloc_region(region_size, size);
}

#if defined(__x86_64__) || defined(__i386__)
uint8_t *
side_code_area_manager::alloc_within_rel32(size_t size, unsigned long ref_addr)
{
	// check if current buffer can be used
	pthread_mutex_lock(&m_mutex);
	if (m_curr_area && (m_curr_area->length - m_curr_area->idx >= size)) {
		uint8_t *head_addr_ptr = m_curr_area->get_head_addr();
		unsigned long head_addr =
		  reinterpret_cast<unsigned long>(head_addr_ptr);
		if (is_within_rel32(head_addr, ref_addr)) {
			m_curr_area->idx += size;
			pthread_mutex_unlock(&m_mutex);
			return head_addr_ptr;
		}
	}
	pthread_mutex_unlock(&m_mutex);

	// try to allocate region within +/-2G from 'addr'
	static const char *fname = "/proc/self/maps";
	ifstream ifs(fname);
	if (!ifs) {
		ROACH_ERR("Failed to open: %s", fname);
		ROACH_ABORT();
	}

	string line;
	int page_size = utils::get_page_size();
	unsigned long alloc_addr = REGION_NOT_FOUND;;
	unsigned long prev_mem_region_end = REGION_START_ADDR;
	unsigned long region_size = page_size;
	while (getline(ifs, line)) {
		unsigned long addr0, addr1;
		if (!extract_address(line, addr0, addr1))
			continue;
		if (addr0 == prev_mem_region_end) {
			prev_mem_region_end = addr1;
			continue;
		}
		alloc_addr = find_region_within_rel32(prev_mem_region_end,
		                                      addr0, ref_addr,
		                                      region_size);
		if (alloc_addr == REGION_NOT_FOUND) {
			prev_mem_region_end = addr1;
			continue;
		}
		break;
	}
	if (alloc_addr == REGION_NOT_FOUND) {
		ROACH_ERR("Failed to find address to be allocated: %lx, %zd\n",
		          ref_addr, size);
		ROACH_ABORT();
	}

	uint8_t *new_addr;
	new_addr = alloc_region(region_size, size,
	                        reinterpret_cast<uint8_t *>(alloc_addr));
	return new_addr;
}
#endif // defined(__x86_64__) || defined(__i386__)

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
#if defined(__x86_64__) || defined(__i386__)
bool
side_code_area_manager::is_within_rel32(unsigned long addr,
                                        unsigned long ref_addr)
{
	if (addr < ref_addr) {
		if (ref_addr < 0x80000000UL)
			return true;
		return (addr >= ref_addr - 0x80000000UL);
	} else
		return (addr <= ref_addr + 0x7fffffffUL);
}

unsigned long
side_code_area_manager::find_region_within_rel32(unsigned long addr0,
                                                 unsigned long addr1,
                                                 unsigned long ref_addr,
                                                 size_t region_size)
{
	// check arguments
	if (addr0 >= addr1)
		ROACH_BUG("addr0: %lx >= addr1: %lx\n", addr0, addr1);

	if (addr1 - addr0 < region_size)
		return REGION_NOT_FOUND;

	unsigned long mid_addr = 0;
	if (addr1 < ref_addr) {
		if (!is_within_rel32(addr1 - region_size, ref_addr))
			return REGION_NOT_FOUND;
		if (is_within_rel32(addr0, ref_addr))
			return addr0;
		// we allocate a relocation block at 1GB lower from
		// the target code.
		mid_addr = get_page_boundary_ceil(ref_addr - 0x40000000UL);
		if (mid_addr + region_size <= addr1)
			return mid_addr;
	} else {
		if (!is_within_rel32(addr0 + region_size, ref_addr))
			return REGION_NOT_FOUND;
		return addr0;
	}

	return REGION_NOT_FOUND;
}
#endif // defined(__x86_64__) || defined(__i386__)

bool
side_code_area_manager::extract_address(string &line,
                                        unsigned long &addr0,
                                        unsigned long &addr1)
{
	int num = sscanf(line.c_str(), "%lx-%lx", &addr0, &addr1);
	if (num != 2)
		return false;
	return true;
}

unsigned long
side_code_area_manager::get_page_boundary_ceil(unsigned long addr)
{
	unsigned long page_size = utils::get_page_size();
	unsigned long ret = (addr + page_size - 1) / page_size;
	ret *= page_size;
	return ret;
}

uint8_t *side_code_area_manager::alloc_region(size_t region_size,
                                              size_t reserve_size,
                                              void *request_addr)
{
	int page_size = utils::get_page_size();
	int map_size = (region_size + page_size - 1) / page_size;
	map_size *= page_size;
	int prot = PROT_EXEC|PROT_READ|PROT_WRITE;
	void *ptr = mmap(request_addr, map_size, prot,
	                 MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	// FIXME: mmap() with alloc_addr may fail if the other
	//        thread allocates memory region we will allocate.
	if (ptr == MAP_FAILED) {
		ROACH_ERR("Failed to mmap: %d\n", errno);
		ROACH_ABORT();
	}

	unsigned long addr = reinterpret_cast<unsigned long>(ptr);
	side_code_area *area = new side_code_area(addr, map_size);
	pthread_mutex_lock(&m_mutex);
	get_side_code_area_set().insert(area);
	m_curr_area = area;

	uint8_t *ret = m_curr_area->get_head_addr();
	m_curr_area->idx += reserve_size;
	pthread_mutex_unlock(&m_mutex);
	return ret;
}
