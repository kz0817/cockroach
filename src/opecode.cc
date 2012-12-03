#include <cstdio>
#include "opecode.h"

#ifdef __x86_64__

opecode::opecode(void)
: m_length(0),
  m_code(NULL)
{
}

opecode::~opecode()
{
	if (m_code)
		delete m_code;
}

#endif // __x86_64__
