#include "utils.h"

vector<string> utils::split(const char *line, const char separator)
{
	vector<string> tokens;

	// seek to the non-space charactor
	string str;
	for (const char *ptr = line; *ptr != '\0'; ptr++) {
		if (*ptr == '\n' || *ptr == '\r')
			break;
		if (*ptr == separator) {
			if (str.size() > 0) {
				tokens.push_back(str);
				str.erase();
			} else {
				; // just ignore
			}
		} else {
			str += *ptr;
		}
	}
	if (str.size() > 0)
		tokens.push_back(str);

	return tokens;
}

