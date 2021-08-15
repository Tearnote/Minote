#pragma once

#include <string_view>
#include <string>
#include "base/containers/nullalloc.hpp"

namespace minote::base {

// Standard string, stored on stack by default and relocated to heap if large.
using std::string;

// Static string. Relies entirely on small string optimization. Throws if SSO
// capacity is exceeded. Release-mode only.
#if _ITERATOR_DEBUG_LEVEL == 0
using sstring = string<NullAllocator<char>>;
#else //_ITERATOR_DEBUG_LEVEL
using sstring = string;
#endif //_ITERATOR_DEBUG_LEVEL

// Read-only view into a string, used for function arguments. Pass by value when
// used as function argument.
using std::string_view;

namespace literals {

// Import sv literal
using namespace std::string_view_literals;

}

}
