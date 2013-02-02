#include "cockroach.h"

static cockroach roach_obj;

// --------------------------------------------------------------------------
// wrapper functions
// --------------------------------------------------------------------------
/**
 * dlopen() wrapper to call dlopen_hook_start() if necesary.
 *
 * @param filename of dynamic library (full path).
 * @param flag dlopen() flags.
 * @returns an opaque "handle" for the dynamic library.
 */
extern "C"
void *dlopen(const char *filename, int flag)
{
	void *handle = (*roach_obj.m_orig_dlopen)(filename, flag);
	if (handle == NULL)
		return NULL;
	return roach_obj.dlopen_hook(filename, flag, handle);
}

/**
 * just calling glibc dlclose().
 */
extern "C"
int dlclose(void *handle) __THROW
{
	return (*roach_obj.m_orig_dlclose)(handle);
}
