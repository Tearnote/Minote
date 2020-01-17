/**
 * Implementation of visualdef.h
 * @file
 */

#include "visualtypes.h"

#include <tgmath.h>

#include <assert.h>
#include <stdalign.h>
static_assert(alignof(int) <= sizeof(int), u8"int structs not usable with OpenGL");
static_assert(alignof(float) <= sizeof(float), u8"float structs not usable with OpenGL");

color3 color3ToLinear(color3 color)
{
	return (color3){
		pow(color.r, 2.2),
		pow(color.g, 2.2),
		pow(color.b, 2.2)
	};
}
