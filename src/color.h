/**
 * Basic structs to represent colors
 * @file
 */

#ifndef MINOTE_COLOR_H
#define MINOTE_COLOR_H

/**
 * A struct that represents an RGB color triple. Valid values for each component
 * are 0.0 to 1.0.
 */
typedef struct Color3 {
	double r; ///< The red component
	double g; ///< The green component
	double b; ///< The blue component
} Color3;

/**
 * A struct that represents an RGBA color quad. Valid values for each
 * component are 0.0 to 1.0.
 */
typedef struct Color4 {
	double r; ///< The red component
	double g; ///< The green component
	double b; ///< The blue component
	double a; ///< The alpha component
} Color4;

#endif //MINOTE_COLOR_H
