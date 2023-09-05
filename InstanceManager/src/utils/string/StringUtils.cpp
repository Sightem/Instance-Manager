#include "utils/string/StringUtils.h"

#define NOMINMAX
#include <windows.h>

#include <vector>

namespace StringUtils {
	bool ContainsOnly(const std::string& s, char c) {
		return s.find_first_not_of(c) == std::string::npos;
	}

}// namespace StringUtils