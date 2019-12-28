/**
 * Semantic structures for dealing with coordinates and sizes
 * @file
 */

#ifndef MINOTE_POINT_H
#define MINOTE_POINT_H

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
	double x; ///< The x coordinate
	double y; ///< The y coordinate
} Point2f;

/// Struct describing a floating-point 2D size. Members should not be negative.
typedef Point2f Size2f;

/// Struct describing a floating-point position in 3D space
typedef struct Point3f {
	double x; ///< The x coordinate
	double y; ///< The y coordinate
	double z; ///< The z coordinate
} Point3f;

/// Struct describing a floating-point 3D size. Members should not be negative.
typedef Point3f Size3f;

#endif //MINOTE_POINT_H
