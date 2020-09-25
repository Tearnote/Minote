/**
 * Implementation of visualdef.h
 * @file
 */

#include "basetypes.hpp"

#include <cmath>

static_assert(alignof(int) <= sizeof(int), u8"int structs not usable with OpenGL");
static_assert(alignof(float) <= sizeof(float), u8"float structs not usable with OpenGL");

color3 color3ToLinear(color3 color)
{
	return (color3){
		std::pow(color.r, 2.2f),
		std::pow(color.g, 2.2f),
		std::pow(color.b, 2.2f)
	};
}
