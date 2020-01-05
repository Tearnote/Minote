/**
 * System for drawing inside a ::Window using a ::Renderer and Model* instances
 * @file
 */

#ifndef MINOTE_RENDERER_H
#define MINOTE_RENDERER_H

#include <stddef.h>
#include "linmath/linmath.h"
#include "window.h"
#include "log.h"
#include "visualdef.h"

/**
 * Opaque renderer type. You can obtain an instance with rendererCreate().
 */
typedef struct Renderer Renderer;

/**
 * Create a new ::Renderer instance and attach it to a ::Window.
 * @param window The ::Window to attach to. Must have no other ::Renderer attached
 * @param log A ::Log to use for rendering messages
 * @return A newly created ::Renderer. Needs to be destroyed with
 * rendererDestroy()
 */
Renderer* rendererCreate(Window* window, Log* log);

/**
 * Destroy a ::Renderer instance. The context is freed from the ::Window, and
 * another ::Renderer can be attached instead. he destroyed object cannot be
 * used anymore and the pointer becomes invalid.
 * @param r The ::Renderer object
 */
void rendererDestroy(Renderer* r);

/**
 * Clear all buffers to a specified color.
 * @param r The ::Renderer object
 * @param color RGB color to clear with
 */
void rendererClear(Renderer* r, Color3 color);

/**
 * Flip buffers, presenting the rendered image to the screen.
 * @param r The ::Renderer object
 */
void rendererFlip(Renderer* r);

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
 * @param r The ::Renderer object
 * @param name Human-readable name for reference
 * @param numVertices Number of elements in @a vertices array
 * @param vertices Model mesh as an array of ::VertexFlat structs
 * @return Newly created ::ModelFlat. Must be destroyed with modelDestroyFlat()
 */
ModelFlat* modelCreateFlat(Renderer* r, const char* name,
		size_t numVertices, VertexFlat vertices[]);

/**
 * Create a ::ModelPhong instance. The provided mesh is uploaded to the GPU
 * and the model is ready to draw with modelDrawPhong().
 * @param r The ::Renderer object
 * @param name Human-readable name for reference
 * @param numVertices Number of elements in @a vertices array
 * @param vertices Model mesh as an array of ::VertexPhong structs
 * @param material Material data for the Phong shading model
 * @return Newly created ::ModelPhong. Must be destroyed with 
 * modelDestroyPhong()
 */
ModelPhong* modelCreatePhong(Renderer* r, const char* name,
		size_t numVertices, VertexPhong vertices[], MaterialPhong material);

/**
 * Destroy a ::ModelFlat instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param r The ::Renderer object
 * @param m The ::ModelFlat object to destroy
 */
void modelDestroyFlat(Renderer* r, ModelFlat* m);

/**
 * Destroy a ::ModelPhong instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param r The ::Renderer object
 * @param m The ::ModelPhong object to destroy
 */
void modelDestroyPhong(Renderer* r, ModelPhong* m);

/**
 * Draw a ::ModelFlat on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param r The ::Renderer object
 * @param m The ::Model object to draw
 * @param instances Number of instances to draw
 * @param tints Array of color tints for each instance
 * @param transforms Array of 4x4 matrices for transforming each instance
 */
void modelDrawFlat(Renderer* r, ModelFlat* m, size_t instances,
		Color4 tints[instances], mat4x4 transforms[instances]);

/**
 * Draw a ::ModelPhong on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param r The ::Renderer object
 * @param m The ::Model object to draw
 * @param instances Number of instances to draw
 * @param tints Array of color tints for each instance
 * @param transforms Array of 4x4 matrices for transforming each instance
 */
void modelDrawPhong(Renderer* r, ModelPhong* m, size_t instances,
		Color4 tints[instances], mat4x4 transforms[instances]);

#endif //MINOTE_RENDERER_H
