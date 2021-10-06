#pragma once

#include <string_view>
#include <string>

namespace minote::base {

// Standard string, stored on stack by default and relocated to heap if large.
using std::string;

// Read-only view into a string, used for function arguments. Pass by value when
// used as function argument.
using std::string_view;

using std::to_string;

namespace literals {

// Import sv literal
using namespace std::string_view_literals;

}

}
