#pragma once
#include <sstream>

namespace cloopenwebrtc {

	template<class T>
	static bool ToString(const T& val, std::string *str)
	{
		std::ostringstream oss;
		oss << std::boolalpha << val;
		*str = oss.str();
		return !oss.fail();
	}
	template<typename T>
	static inline std::string ToString(const T& val) {
		std::string str;
		ToString(val, &str);
		return str;
	}

}

