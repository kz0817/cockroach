#ifndef utils_h
#define utils_h

#include <vector>
#include <string>
using namespace std;

class utils {
public:
	static vector<string> split(const char *line, const char separator = ' ');
};

#endif
