/**
 * Semantic structures for dealing with coordinates and sizes.
 * @file
 */

#ifndef MINOTE_POINT_H
#define MINOTE_POINT_H

/**
 * Struct describing an integer position in 2D space.
 */
typedef struct Point2i Point2i;
struct Point2i {
	int x;
	int y;
};

/**
 * Struct describing an integer 2D size. Members should not be negative.
 */
typedef Point2i Size2i;

/**
 * Struct describing an integer position in 3D space.
 */
typedef struct Point3i Point3i;
struct Point3i {
	int x;
	int y;
	int z;
};

/**
 * Struct describing an integer 3D size. Members should not be negative.
 */
typedef Point3i Size3i;

/**
 * Struct describing a floating-point position in 2D space.
 */
typedef struct Point2f Point2f;
struct Point2f {
	double x;
	double y;
};

/**
 * Struct describing a floating-point 2D size. Members should not be negative.
 */
typedef Point2f Size2f;

/**
 * Struct describing a floating-point position in 3D space.
 */
typedef struct Point3f Point3f;
struct Point3f {
	double x;
	double y;
	double z;
};

/**
 * Struct describing a floating-point 3D size. Members should not be negative.
 */
typedef Point3f Size3f;

#endif //MINOTE_POINT_H
