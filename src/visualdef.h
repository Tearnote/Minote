/**
 * Semantic structures for dealing with coordinates, sizes and colors
 * @file
 */

#ifndef MINOTE_VISUALDEF_H
#define MINOTE_VISUALDEF_H

/// An integer position in 2D space
typedef struct Point2i {
	int x; ///< The x coordinate
	int y; ///< The y coordinate
} Point2i;

/// An integer 2D size. Members should not be negative.
typedef Point2i Size2i;

/// An integer position in 3D space
typedef struct Point3i {
	int x; ///< The x coordinate
	int y; ///< The y coordinate
	int z; ///< The z coordinate
} Point3i;

/// An integer 3D size. Members should not be negative.
typedef Point3i Size3i;

/// A floating-point position in 2D space
typedef struct Point2f {
	float x; ///< The x coordinate
	float y; ///< The y coordinate
} Point2f;

/// A floating-point 2D size. Members should not be negative.
typedef Point2f Size2f;

/// A floating-point position in 3D space
typedef struct Point3f {
	float x; ///< The x coordinate
	float y; ///< The y coordinate
	float z; ///< The z coordinate
} Point3f;

/// A floating-point 3D size. Members should not be negative.
typedef Point3f Size3f;

/// An RGB color triple. Values higher than 1.0 represent HDR.
typedef struct Color3 {
	float r; ///< The red component
	float g; ///< The green component
	float b; ///< The blue component
} Color3;

/// An RGBA color quad. Values higher than 1.0 represent HDR.
typedef struct Color4 {
	float r; ///< The red component
	float g; ///< The green component
	float b; ///< The blue component
	float a; ///< The alpha component
} Color4;

#endif //MINOTE_VISUALDEF_H
