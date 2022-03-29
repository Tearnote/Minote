#pragma once

#include <string_view> // base/containers/string.hpp would cause a circular dependency
#include <functional>
#include <stdexcept>
#include <utility>
#include "base/format.hpp"

namespace minote::base {

using std::runtime_error;
using std::logic_error;

template<typename Err, typename... Args>
inline auto typed_error_fmt(std::string_view fmt, Args&&... args) -> Err {
	
	return Err(format(fmt, std::forward<Args>(args)...));
	
}

template<typename... Args>
inline auto runtime_error_fmt(std::string_view fmt, Args&&... args) {
	return typed_error_fmt<runtime_error>(fmt, std::forward<Args>(args)...);
};

template<typename... Args>
inline auto logic_error_fmt(std::string_view fmt, Args&&... args) {
	return typed_error_fmt<logic_error>(fmt, std::forward<Args>(args)...);
};

}
