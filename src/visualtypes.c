/**
 * Implementation of visualdef.h
 * @file
 */

#include "visualtypes.h"

#include <tgmath.h>

color3 color3ToLinear(color3 color)
{
	return (color3){
		pow(color.r, 2.2),
		pow(color.g, 2.2),
		pow(color.b, 2.2)
	};
}
