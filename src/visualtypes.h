/**
 * Semantic structures for dealing with coordinates, sizes and colors
 * @file
 */

#ifndef MINOTE_VISUALTYPES_H
#define MINOTE_VISUALTYPES_H

/// An integer position in 2D space
typedef struct point2i {
	int x; ///< The x coordinate
	int y; ///< The y coordinate
} point2i;

/// An integer 2D size. Members should not be negative.
typedef point2i size2i;

/// An integer position in 3D space
typedef struct point3i {
	int x; ///< The x coordinate
	int y; ///< The y coordinate
	int z; ///< The z coordinate
} point3i;

/// An integer 3D size. Members should not be negative.
typedef point3i size3i;

/// A floating-point position in 2D space
typedef struct point2f {
	float x; ///< The x coordinate
	float y; ///< The y coordinate
} point2f;

/// A floating-point 2D size. Members should not be negative.
typedef point2f size2f;

/// A floating-point position in 3D space
typedef struct point3f {
	float x; ///< The x coordinate
	float y; ///< The y coordinate
	float z; ///< The z coordinate
} point3f;

/// A floating-point 3D size. Members should not be negative.
typedef point3f size3f;

/// An RGB color triple. Values higher than 1.0 represent HDR.
typedef struct color3 {
	float r; ///< The red component
	float g; ///< The green component
	float b; ///< The blue component
} color3;

/// An RGBA color quad. Values higher than 1.0 represent HDR.
typedef struct color4 {
	float r; ///< The red component
	float g; ///< The green component
	float b; ///< The blue component
	float a; ///< The alpha component
} color4;

/// White color convenience constant
#define Color4White ((color4){1.0f, 1.0f, 1.0f, 1.0f})

/// Fully transparent color convenience constant
#define Color4Clear ((color4){1.0f, 1.0f, 1.0f, 0.0f})

/**
 * Convert a ::Color3 from sRGB to Linear color space.
 * @param color Input color
 * @return Output color
 */
color3 color3ToLinear(color3 color);

#endif //MINOTE_VISUALTYPES_H
