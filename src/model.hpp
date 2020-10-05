/**
 * Rendering 3D model data with a number of blending systems
 * @file
 */

#ifndef MINOTE_MODEL_H
#define MINOTE_MODEL_H

#include "linmath/linmath.h"
#include "base/types.hpp"
#include "opengl.hpp"

/// Convenient 4x4 matrix for when you want to perform no transform
inline mat4x4 IdentityMatrix{{1.0f, 0.0f, 0.0f, 0.0f},
                             {0.0f, 1.0f, 0.0f, 0.0f},
                             {0.0f, 0.0f, 1.0f, 0.0f},
                             {0.0f, 0.0f, 0.0f, 1.0f}};

/// Data of a single mesh vertex of ::ModelFlat
typedef struct VertexFlat {
	minote::point3f pos; ///< in model space
	minote::color4 color;
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

/// Type tag for the Model object
typedef enum ModelType {
	ModelTypeNone, ///< zero value
	ModelTypeFlat, ///< ::ModelFlat
	ModelTypePhong, ///< ::ModelPhong
	ModelTypeSize ///< terminator
} ModelType;

/// Base type for a Model. Contains all attributes common to every model type.
typedef struct Model {
	ModelType type; ///< Correct type to cast the Model to
	const char* name; ///< Human-readable name for reference
} Model;
typedef Model ModelBase;

/// Model type with flat shading. Each instance can be tinted.
typedef struct ModelFlat {
	ModelBase base;
	size_t numVertices;
	VertexBuffer vertices; ///< VBO with model vertex data
	VertexBuffer tints; ///< VBO for storing per-draw tint colors
	VertexBuffer highlights; ///< VBO for storing per-draw color highlight colors
	VertexBuffer transforms; ///< VBO for storing per-draw model matrices
	VertexArray vao;
} ModelFlat;

/// Model type with Phong shading. Makes use of light source and material data.
typedef struct ModelPhong {
	ModelBase base;
	size_t numVertices;
	VertexBuffer vertices; ///< VBO with model vertex data
	VertexBuffer normals; ///< VBO with model normals, generated from vertices
	VertexBuffer tints; ///< VBO for storing per-draw tint colors
	VertexBuffer highlights; ///< VBO for storing per-draw color highlight colors
	VertexBuffer transforms; ///< VBO for storing per-draw model matrices
	VertexArray vao;
	MaterialPhong material;
} ModelPhong;

/**
 * Initialize the model rendering system. Must be called after rendererInit().
 * This is only required for drawing models, not creation or destruction.
 */
void modelInit(void);

/**
 * Cleanup the model rendering system. No model drawing functions can be used
 * until modelInit() is called again.
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
	size_t numVertices, VertexPhong vertices[],
	MaterialPhong material);

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
	minote::color4 tints[], minote::color4 highlights[],
	mat4x4 transforms[]);

#endif //MINOTE_MODEL_H
