/**
 * System for drawing inside a ::Window using a ::Renderer and ::Model instances
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

/// Struct containing the data of a single mesh vertex
typedef struct Vertex {
	Point3f pos; ///< Vertex position in model space
	Color4 color; ///< Vertex color
} Vertex;

/// Struct that binds 3 vertices into a triangle
typedef struct Triangle {
	Vertex v1; ///< First vertex
	Vertex v2; ///< Second vertex
	Vertex v3; ///< Third vertex
} Triangle;

/// List of shader programs supported by the renderer
typedef enum ProgramType {
	ProgramNone, ///< zero value
	ProgramFlat, ///< Flat shading and no lighting. Most basic shader
//	ProgramPhong,
//	ProgramMsdf,
	ProgramSize ///< terminator
} ProgramType;

/**
 * Opaque model type. You can obtain an instance with modelCreate().
 */
typedef struct Model Model;

/**
 * Create a ::Model instance. The provided mesh is uploaded to the GPU and the
 * model is ready to draw with modelDraw().
 * @param r The ::Renderer object
 * @param type Shader program to use for drawing the model
 * @param name Human-readable name for reference
 * @param numTriangles Number of triangles in @p triangles array
 * @param triangles Model mesh as an array of ::Triangle structs
 * @return Newly created ::Model. Must be destroyed with modelDestroy()
 */
Model* modelCreate(Renderer* r, ProgramType type, const char* name,
		size_t numTriangles, Triangle triangles[numTriangles]);

/**
 * Destroy a ::Model instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param r The ::Renderer object
 * @param m The ::Model object to destroy
 */
void modelDestroy(Renderer* r, Model* m);

/**
 * Draw a ::Model on the screen. Instanced rendering is used, and each instance
 * can be tinted with a provided color.
 * @param r The ::Renderer object
 * @param m The ::Model object to draw
 * @param instances Number of instances to draw
 * @param tints Array of color tints for each instance
 * @param transforms Array of 4x4 matrices for transforming each instance
 */
void modelDraw(Renderer* r, Model* m, size_t instances,
		Color4 tints[instances], mat4x4 transforms[instances]);

#endif //MINOTE_RENDERER_H
