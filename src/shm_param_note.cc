#include "shm_param_note.h"

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
shm_param_note::shm_param_note(void)
: m_has_error(true)
{
}

shm_param_note::~shm_param_note(void)
{
}

void shm_param_note::open(void)
{
}

bool shm_param_note::has_error(void) const
{
	return m_has_error;
}

const string &shm_param_note::get_recipe_file_path(void) const
{
	return m_recipe_file_path;
}
