/**
 * Rendering 3D model data with a number of blending systems
 * @file
 */

#ifndef MINOTE_MODEL_H
#define MINOTE_MODEL_H

#include "linmath/linmath.h"
#include "basetypes.h"

/// Convenient 4x4 matrix for when you want to perform no transform
#define IdentityMatrix ((mat4x4){{1.0f, 0.0f, 0.0f, 0.0f}, \
                                 {0.0f, 1.0f, 0.0f, 0.0f}, \
                                 {0.0f, 0.0f, 1.0f, 0.0f}, \
                                 {0.0f, 0.0f, 0.0f, 1.0f}})

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
 * Initialize the model rendering system. Must be called after rendererInit().
 */
void modelInit(void);

/**
 * Cleanup the model rendering system. No model functions can be used until
 * modelInit() is called again.
 */
void modelCleanup(void);

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
 * @param tints Color tints for each instance. Can be null
 * @param highlights Highlight colors to blend into each instance. Can be null
 * @param transforms 4x4 matrices for transforming each instance
 */
void modelDraw(Model* m, size_t instances,
	color4 tints[instances], color4 highlights[instances],
	mat4x4 transforms[instances]);

#endif //MINOTE_MODEL_H
