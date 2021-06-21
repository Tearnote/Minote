#pragma once

#include <stdexcept>
#include <string_view> // base/container/string.hpp would cause a circular dependency
#include "base/format.hpp"

namespace minote::base {
	
using std::runtime_error;

template<typename... Args>
inline auto runtime_error_fmt(std::string_view fmt, Args&&... args) -> runtime_error {
	
	return runtime_error(format(fmt, args...));
	
}

}
