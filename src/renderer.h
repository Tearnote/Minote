/**
 * System for drawing inside of a window using primitives and ::Model instances
 * @file
 */

#ifndef MINOTE_RENDERER_H
#define MINOTE_RENDERER_H

#include <stddef.h>
#include "linmath/linmath.h"
#include "basetypes.h"

/// Convenient 4x4 matrix for when you want to perform no transform
#define IdentityMatrix ((mat4x4){{1.0f, 0.0f, 0.0f, 0.0f}, \
                                 {0.0f, 1.0f, 0.0f, 0.0f}, \
                                 {0.0f, 0.0f, 1.0f, 0.0f}, \
                                 {0.0f, 0.0f, 0.0f, 1.0f}})

/**
 * Initialize the renderer system. windowInit() must have been called first.
 * This can be called on a different thread than windowInit(), and the
 * calling thread becomes bound to the OpenGL context.
 */
void rendererInit(void);

/**
 * Cleanup the renderer system. No other renderer functions cannot be used
 * until rendererInit() is called again.
 */
void rendererCleanup(void);

/**
 * Clear all buffers to a specified color. This color is also used for Phong
 * ambient reflection.
 * @param color Color to clear with
 */
void rendererClear(color3 color);

/**
 * Prepare for rendering a new frame. You should call rendererClear() afterwards
 * to initialize the framebuffer to a known state.
 */
void rendererFrameBegin(void);

/**
 * Present the rendered image to the screen. This call blocks until the
 * display's next vertical refresh.
 */
void rendererFrameEnd(void);

////////////////////////////////////////////////////////////////////////////////

/// Data of a single mesh vertex of ::ModelFlat
typedef struct VertexFlat {
	point3f pos; ///< in model space
	color4 color;
} VertexFlat;

/// Data of a single mesh vertex of ::ModelPhong
typedef VertexFlat VertexPhong;

/// Material properties of a ::ModelPhong
typedef struct MaterialPhong {
	float ambient;
	float diffuse;
	float specular;
	float shine; ///< Smoothness of surface (inverse of specular highlight size)
} MaterialPhong;

/// Drawable object. You can obtain an instance with one of modelCreate*().
typedef struct Model Model;

/**
 * Create a ::Model instance with flat shading. The provided mesh is uploaded
 * to the GPU and the model is ready to draw with modelDraw().
 * @param name Human-readable name for reference
 * @param numVertices Number of elements in @a vertices array
 * @param vertices Model mesh as an array of ::VertexFlat structs
 * @return Newly created ::Model. Must be destroyed with modelDestroy()
 */
Model* modelCreateFlat(const char* name,
	size_t numVertices, VertexFlat vertices[]);

/**
 * Create a ::Model instance with Phong shading. The provided mesh is uploaded
 * to the GPU and the model is ready to draw with modelDraw().
 * @param name Human-readable name for reference
 * @param numVertices Number of elements in @a vertices array
 * @param vertices Model mesh as an array of ::VertexPhong structs
 * @param material Material data for the Phong shading model
 * @return Newly created ::Model. Must be destroyed with modelDestroy()
 */
Model* modelCreatePhong(const char* name,
	size_t numVertices, VertexPhong vertices[], MaterialPhong material);

/**
 * Destroy a ::Model instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param m The ::Model object to destroy
 */
void modelDestroy(Model* m);

/**
 * Draw a ::Model on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param m The ::Model object to draw
 * @param instances Number of instances to draw
 * @param tints Array of color tints for each instance
 * @param transforms Array of 4x4 matrices for transforming each instance
 */
void modelDraw(Model* m, size_t instances,
	color4 tints[instances], mat4x4 transforms[instances]);

#endif //MINOTE_RENDERER_H
