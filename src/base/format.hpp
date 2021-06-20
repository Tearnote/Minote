#pragma once

#include "fmt/printf.h"
#include "base/string.hpp"

namespace minote::base {

namespace detail {

template<template<typename> typename Alloc>
using format_buffer = fmt::basic_memory_buffer<char, fmt::inline_buffer_size, Alloc<char>>;

template<template<typename> typename Alloc>
auto vformat(fmt::string_view fmt, fmt::printf_args args) {
	
	auto buf = format_buffer<Alloc>();
	fmt::vprintf(buf, fmt, args);
	return string<Alloc>(buf.data(), buf.size());
	
}

}

template<template<typename> typename Alloc, typename ...Args>
auto format(fmt::string_view fmt, Args const& ...args) -> string<Alloc> {
	
	return detail::vformat<Alloc>(fmt, fmt::make_printf_args(args...));
	
}

}
