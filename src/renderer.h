/**
 * System for drawing inside a ::Window using a ::Renderer and Model* instances
 * @file
 */

#ifndef MINOTE_RENDERER_H
#define MINOTE_RENDERER_H

#include <stddef.h>
#include "linmath/linmath.h"
#include "visualtypes.h"

/**
 * Initialize the renderer system. windowInit() must have been called first.
 * This can be called on a different thread than initWindow(), and the
 * calling thread becomes bound to the OpenGL context.
 */
void rendererInit(void);

/**
 * Cleanup the renderer system. No other renderer functions cannot be used
 * until rendererInit() is called again.
 */
void rendererCleanup(void);

/**
 * Clear all buffers to a specified color.
 * @param color Color to clear with
 */
void rendererClear(Color3 color);

/**
 * Prepare for rendering a new frame. You should call rendererClear() to
 * initialize the framebuffer itself to a known state.
 */
void rendererFrameBegin(void);

/**
 * Present the rendered image to the screen. This call blocks until the
 * display's next vertical refresh.
 */
void rendererFrameEnd(void);

////////////////////////////////////////////////////////////////////////////////

/// Struct containing the data of a single mesh vertex of ::ModelFlat
typedef struct VertexFlat {
	Point3f pos; ///< Vertex position in model space
	Color4 color; ///< Vertex color
} VertexFlat;

/// Struct containing the data of a single mesh vertex of ::ModelPhong
typedef VertexFlat VertexPhong;

/// Struct that describes material properties of a ::ModelPhong
typedef struct MaterialPhong {
	float ambient; ///< Strength of ambient light reflection
	float diffuse; ///< Strength of diffuse light reflection
	float specular; ///< Strength of specular light reflection
	float shine; ///< Smoothness of surface (inverse of specular highlight size)
} MaterialPhong;

/**
 * Model type with flat shading. You can obtain an instance with
 * modelCreateFlat().
 */
typedef struct ModelFlat ModelFlat;

/**
 * Model type with phong shading. You can obtain an instance with
 * modelCreatePhong().
 */
typedef struct ModelPhong ModelPhong;

/**
 * Create a ::ModelFlat instance. The provided mesh is uploaded to the GPU
 * and the model is ready to draw with modelDrawFlat().
 * @param name Human-readable name for reference
 * @param numVertices Number of elements in @a vertices array
 * @param vertices Model mesh as an array of ::VertexFlat structs
 * @return Newly created ::ModelFlat. Must be destroyed with modelDestroyFlat()
 */
ModelFlat* modelCreateFlat(const char* name,
	size_t numVertices, VertexFlat vertices[]);

/**
 * Create a ::ModelPhong instance. The provided mesh is uploaded to the GPU
 * and the model is ready to draw with modelDrawPhong().
 * @param name Human-readable name for reference
 * @param numVertices Number of elements in @a vertices array
 * @param vertices Model mesh as an array of ::VertexPhong structs
 * @param material Material data for the Phong shading model
 * @return Newly created ::ModelPhong. Must be destroyed with 
 * modelDestroyPhong()
 */
ModelPhong* modelCreatePhong(const char* name,
	size_t numVertices, VertexPhong vertices[], MaterialPhong material);

/**
 * Destroy a ::ModelFlat instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param m The ::ModelFlat object to destroy
 */
void modelDestroyFlat(ModelFlat* m);

/**
 * Destroy a ::ModelPhong instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param m The ::ModelPhong object to destroy
 */
void modelDestroyPhong(ModelPhong* m);

/**
 * Draw a ::ModelFlat on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param m The ::ModelFlat object to draw
 * @param instances Number of instances to draw
 * @param tints Array of color tints for each instance
 * @param transforms Array of 4x4 matrices for transforming each instance
 */
void modelDrawFlat(ModelFlat* m, size_t instances,
	Color4 tints[instances], mat4x4 transforms[instances]);

/**
 * Draw a ::ModelPhong on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param m The ::ModelPhong object to draw
 * @param instances Number of instances to draw
 * @param tints Array of color tints for each instance
 * @param transforms Array of 4x4 matrices for transforming each instance
 */
void modelDrawPhong(ModelPhong* m, size_t instances,
	Color4 tints[instances], mat4x4 transforms[instances]);

#endif //MINOTE_RENDERER_H
