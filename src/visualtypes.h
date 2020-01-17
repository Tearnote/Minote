/**
 * Semantic structures for dealing with coordinates, sizes and colors
 * @file
 * Alignment is ensured so that the x/y/z members and array access can be used
 * interchangeably.
 */

#ifndef MINOTE_VISUALTYPES_H
#define MINOTE_VISUALTYPES_H

/// An integer position in 2D space
typedef union point2i {
	struct {
		int x; ///< The x coordinate
		int y; ///< The y coordinate
	};
	int arr[2]; ///< array representation
} point2i;

/// An integer 2D size. Members should not be negative.
typedef point2i size2i;

/// An integer position in 3D space
typedef union point3i {
	struct {
		int x; ///< The x coordinate
		int y; ///< The y coordinate
		int z; ///< The z coordinate
	};
	int arr[3]; ///< array representation
} point3i;

/// An integer 3D size. Members should not be negative.
typedef point3i size3i;

/// A floating-point position in 2D space
typedef union point2f {
	struct {
		float x; ///< The x coordinate
		float y; ///< The y coordinate
	};
	float arr[2]; ///< array representation
} point2f;

/// A floating-point 2D size. Members should not be negative.
typedef point2f size2f;

/// A floating-point position in 3D space
typedef union point3f {
	struct {
		float x; ///< The x coordinate
		float y; ///< The y coordinate
		float z; ///< The z coordinate
	};
	float arr[3]; ///< array representation
} point3f;

/// A floating-point 3D size. Members should not be negative.
typedef point3f size3f;

/// An RGB color triple. Values higher than 1.0 represent HDR.
typedef union color3 {
	struct {
		float r; ///< The red component
		float g; ///< The green component
		float b; ///< The blue component
	};
	float arr[3]; ///< array representation
} color3;

/// An RGBA color quad. Values higher than 1.0 represent HDR.
typedef union color4 {
	struct {
		float r; ///< The red component
		float g; ///< The green component
		float b; ///< The blue component
		float a; ///< The alpha component
	};
	float arr[4]; ///< array representation
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
