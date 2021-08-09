#pragma once

#include <string_view>
#include <string>
#include "base/container/nullalloc.hpp"

namespace minote::base {

// Standard string, stored on stack by default and relocated to heap if large.
template<template<typename> typename Alloc = std::allocator>
using string = std::basic_string<char, std::char_traits<char>, Alloc<char>>;

// Static string. Relies entirely on small string optimization. Throws if SSO
// capacity is exceeded. Release-mode only.
#if _ITERATOR_DEBUG_LEVEL == 0
using sstring = string<NullAllocator>;
#else //_ITERATOR_DEBUG_LEVEL
using sstring = string<>;
#endif //_ITERATOR_DEBUG_LEVEL

// Read-only view into a string, used for function arguments. Pass by value when
// used as function argument.
using std::string_view;

namespace literals {

// Import sv literal
using namespace std::string_view_literals;

}

}
