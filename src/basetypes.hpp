/**
 * Semantic structures for dealing with coordinates, sizes and colors
 * @file
 * Alignment is ensured so that the x/y/z members and array access can be used
 * interchangeably.
 */

#ifndef MINOTE_BASETYPES_H
#define MINOTE_BASETYPES_H

/// An integer position in 2D space
typedef struct point2i {
		int x;
		int y;
} point2i;

/// An integer 2D size. Members should not be negative.
typedef point2i size2i;

/// An integer position in 3D space
typedef struct point3i {
		int x;
		int y;
		int z;
} point3i;

/// An integer 3D size. Members should not be negative.
typedef point3i size3i;

/// A floating-point position in 2D space
typedef struct point2f {
		float x;
		float y;
} point2f;

/// A floating-point 2D size. Members should not be negative.
typedef point2f size2f;

/// A floating-point position in 3D space
typedef struct point3f {
		float x;
		float y;
		float z;
} point3f;

/// A floating-point 3D size. Members should not be negative.
typedef point3f size3f;

/// A standard 4-element vector type
typedef struct point4f {
		float x;
		float y;
		float z;
		float w;
} point4f;

/// An RGB color triple. Values higher than 1.0 represent HDR.
typedef struct color3 {
		float r;
		float g;
		float b;
} color3;

/// An RGBA color quad. Values higher than 1.0 represent HDR.
typedef struct color4 {
		float r;
		float g;
		float b;
		float a;
} color4;

/// White color convenience constant
#define Color4White ((color4){1.0f, 1.0f, 1.0f, 1.0f})

/// Black color convenience constant
#define Color4Black ((color4){0.0f, 0.0f, 0.0f, 1.0f})

/// Fully transparent color convenience constant
#define Color4Clear ((color4){1.0f, 1.0f, 1.0f, 0.0f})

/**
 * Convert a ::color3 from sRGB to Linear color space.
 * @param color Input color
 * @return Output color
 */
color3 color3ToLinear(color3 color);

#endif //MINOTE_BASETYPES_H
