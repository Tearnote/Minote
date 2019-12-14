// Minote - point.h

#ifndef MINOTE_POINT_H
#define MINOTE_POINT_H

#include <string>
#include "fmt/core.h"

template<typename T>
struct Point {
	T x{0};
	T y{0};
};

template<typename T>
using Size = Point<T>;

template <typename T>
auto to_string(const Point<T>& p) -> std::string
{
	return fmt::format("({}, {})", p.x, p.y);
}

#endif //MINOTE_POINT_H
