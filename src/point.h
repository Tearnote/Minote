// Minote - point.h

#ifndef MINOTE_POINT_H
#define MINOTE_POINT_H

#include <string>
#include <ostream>
#include <sstream>

template<typename T>
struct Point {
	T x{0};
	T y{0};
};

template<typename T>
using Size = Point<T>;

template<typename T>
auto operator<<(std::ostream& out, const Point<T>& p) -> std::ostream&
{
	return out << "(" << p.x << ", " << p.y << ")";
}

template <typename T>
auto to_string( const T& value ) -> std::string
{
	std::ostringstream ss{};
	ss << value;
	return ss.str();
}

#endif //MINOTE_POINT_H
