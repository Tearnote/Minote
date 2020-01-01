/**
 * Semantic structures for dealing with coordinates, sizes and colors
 * @file
 */

#ifndef MINOTE_VISUALDEF_H
#define MINOTE_VISUALDEF_H

/// Struct describing an integer position in 2D space
typedef struct Point2i {
	int x; ///< The x coordinate
	int y; ///< The y coordinate
} Point2i;

/// Struct describing an integer 2D size. Members should not be negative.
typedef Point2i Size2i;

/// Struct describing an integer position in 3D space
typedef struct Point3i {
	int x; ///< The x coordinate
	int y; ///< The y coordinate
	int z; ///< The z coordinate
} Point3i;

/// Struct describing an integer 3D size. Members should not be negative.
typedef Point3i Size3i;

/// Struct describing a floating-point position in 2D space
typedef struct Point2f {
	float x; ///< The x coordinate
	float y; ///< The y coordinate
} Point2f;

/// Struct describing a floating-point 2D size. Members should not be negative.
typedef Point2f Size2f;

/// Struct describing a floating-point position in 3D space
typedef struct Point3f {
	float x; ///< The x coordinate
	float y; ///< The y coordinate
	float z; ///< The z coordinate
} Point3f;

/// Struct describing a floating-point 3D size. Members should not be negative.
typedef Point3f Size3f;

/**
 * A struct that represents an RGB color triple. Values higher than 1.0
 * represent HDR.
 */
typedef struct Color3 {
	float r; ///< The red component
	float g; ///< The green component
	float b; ///< The blue component
} Color3;

/**
 * A struct that represents an RGBA color quad. Values higher than 1.0
 * represent HDR.
 */
typedef struct Color4 {
	float r; ///< The red component
	float g; ///< The green component
	float b; ///< The blue component
	float a; ///< The alpha component
} Color4;

#endif //MINOTE_VISUALDEF_H
