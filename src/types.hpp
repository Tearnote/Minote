#pragma once

#include <cstddef>
#include <cstdint>

namespace minote {

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint = std::uint32_t;
using uint64 = std::uint64_t;
using int8 = std::int8_t;
using int16 = std::int16_t;
// int32 = int
using int64 = std::int64_t;
using usize = std::size_t;
using isize = std::ptrdiff_t;

namespace type_literals {

// usize integer literal
consteval auto operator ""_zu(unsigned long long val) -> usize { return val; }

}

// Sanity check

static_assert(sizeof(uint8) == 1);
static_assert(sizeof(uint16) == 2);
static_assert(sizeof(uint) == 4);
static_assert(sizeof(uint64) == 8);
static_assert(sizeof(int8) == 1);
static_assert(sizeof(int16) == 2);
static_assert(sizeof(int) == 4);
static_assert(sizeof(int64) == 8);

}
