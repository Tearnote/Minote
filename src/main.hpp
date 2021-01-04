#pragma once

#include <string_view>
#include "base/version.hpp"

namespace minote {

using namespace std::string_view_literals;

constexpr auto AppTitle = "Minote"sv;
constexpr auto AppVersion = base::Version{0, 0, 0};

}
