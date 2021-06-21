#pragma once

#include <string_view>
#include <string>
#include "base/memory/nullalloc.hpp"

namespace minote::base {

// Import sv literal
using namespace std::string_view_literals;

// Standard string, stored on stack by default and relocated to heap if large.
template<template<typename> typename Alloc = std::allocator>
using string = std::basic_string<char, std::char_traits<char>, Alloc<char>>;

// Static string. Relies entirely on small string optimization. Throws if SSO
// capacity is exceeded.
using sstring = string<NullAllocator>;

// Read-only view into a string, used for function arguments. Pass by value when
// used as function argument.
using std::string_view;

}
