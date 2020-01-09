/**
 * Implementation of visualdef.h
 * @file
 */

#include "visualtypes.h"

#include <tgmath.h>

Color3 color3ToLinear(Color3 color)
{
	return (Color3){
		pow(color.r, 2.2),
		pow(color.g, 2.2),
		pow(color.b, 2.2)
	};
}
