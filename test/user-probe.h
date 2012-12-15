#ifndef user_probe_h
#define user_probe_h

struct user_data {
	int id;
	int call_times;
};

struct user_record_t {
	int call_times;
	unsigned long arg0;
};

#endif // user_probe_h
