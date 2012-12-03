#ifndef opecode_h
#define opecode_h

#include <stdint.h>

#ifdef __x86_64__

class opecode {
	int m_length;
	uint8_t *m_code;
public:
	opecode(void);
	virtual ~opecode();
};

#endif // __x86_64__

#endif
