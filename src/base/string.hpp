#pragma once

#include <string_view>
#include <string>

namespace minote::base {

using namespace std::string_view_literals;

template<template<typename> typename Alloc>
using string = std::basic_string<char, std::char_traits<char>, Alloc<char>>;

using string_view = std::string_view;

}
