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

string utils::get_basename(const char *path)
{
	string str(path);
	return get_basename(str);
}

string utils::get_basename(string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == string::npos)
		return path;
	return path.substr(pos+1);
}
