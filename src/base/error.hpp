#pragma once

#include <string_view> // base/container/string.hpp would cause a circular dependency
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

auto const runtime_error_fmt = []<typename... Args>(Args&&... args) {
	return typed_error_fmt<runtime_error>(std::forward<Args>(args)...);
};

auto const logic_error_fmt = []<typename... Args>(Args&&... args) {
	return typed_error_fmt<logic_error>(std::forward<Args>(args)...);
};

}
