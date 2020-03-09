/**
 * Implementation of renderer.h
 * @file
 */

#include "renderer.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "linmath/linmath.h"
#include "smaa/AreaTex.h"
#include "smaa/SearchTex.h"
#include "window.h"
#include "shader.h"
#include "util.h"
#include "time.h"
#include "log.h"

#define BloomPasses 6

typedef GLuint Texture; ///< OpenGL texture object ID
typedef GLuint Framebuffer; ///< OpenGL framebuffer object ID
typedef GLuint Renderbuffer; ///< OpenGL renderbuffer object ID

/// Start of the clipping plane, in world distance units
#define ProjectionNear 0.1f

/// End of the clipping plane (draw distance), in world distance units
#define ProjectionFar 100.0f

/// Flat shading type
typedef struct ProgramFlat {
	ProgramBase base;
	Uniform camera;
	Uniform projection;
} ProgramFlat;

/// Phong-Blinn shading type
typedef struct ProgramPhong {
	ProgramBase base;
	Uniform camera;
	Uniform projection;
	Uniform lightPosition;
	Uniform lightColor;
	Uniform ambientColor;
	Uniform ambient;
	Uniform diffuse;
	Uniform specular;
	Uniform shine;
} ProgramPhong;

typedef struct ProgramSmaaSeparate {
	ProgramBase base;
	TextureUnit image;
} ProgramSmaaSeparate;

typedef struct ProgramSmaaEdge {
	ProgramBase base;
	TextureUnit image;
	Uniform screenSize;
} ProgramSmaaEdge;

typedef struct ProgramSmaaBlend {
	ProgramBase base;
	TextureUnit edges1;
	TextureUnit edges2;
	TextureUnit area;
	TextureUnit search;
	Uniform screenSize;
} ProgramSmaaBlend;

typedef struct ProgramSmaaNeighbor {
	ProgramBase base;
	TextureUnit image1;
	TextureUnit image2;
	TextureUnit blend1;
	TextureUnit blend2;
	Uniform screenSize;
} ProgramSmaaNeighbor;

/// Basic blit function type
typedef struct ProgramBlit {
	ProgramBase base;
	TextureUnit image;
	Uniform boost;
} ProgramBlit;

/// Bloom threshold filter type
typedef struct ProgramThreshold {
	ProgramBase base;
	TextureUnit image;
	Uniform threshold;
	Uniform softKnee;
	Uniform strength;
} ProgramThreshold;

typedef struct ProgramBoxBlur {
	ProgramBase base;
	TextureUnit image;
	Uniform step;
	Uniform imageTexel;
} ProgramBoxBlur;

static const char* ProgramFlatVertName = u8"flat.vert";
static const GLchar* ProgramFlatVertSrc = (GLchar[]){
#include "flat.vert"
	'\0'};
static const char* ProgramFlatFragName = u8"flat.frag";
static const GLchar* ProgramFlatFragSrc = (GLchar[]){
#include "flat.frag"
	'\0'};

static const char* ProgramPhongVertName = u8"phong.vert";
static const GLchar* ProgramPhongVertSrc = (GLchar[]){
#include "phong.vert"
	'\0'};
static const char* ProgramPhongFragName = u8"phong.frag";
static const GLchar* ProgramPhongFragSrc = (GLchar[]){
#include "phong.frag"
	'\0'};

static const char* ProgramSmaaSeparateVertName = u8"smaaSeparate.vert";
static const GLchar* ProgramSmaaSeparateVertSrc = (GLchar[]){
#include "smaaSeparate.vert"
	'\0'};
static const char* ProgramSmaaSeparateFragName = u8"smaaSeparate.frag";
static const GLchar* ProgramSmaaSeparateFragSrc = (GLchar[]){
#include "smaaSeparate.frag"
	'\0'};

static const char* ProgramSmaaEdgeVertName = u8"smaaEdge.vert";
static const GLchar* ProgramSmaaEdgeVertSrc = (GLchar[]){
#include "smaaEdge.vert"
	'\0'};
static const char* ProgramSmaaEdgeFragName = u8"smaaEdge.frag";
static const GLchar* ProgramSmaaEdgeFragSrc = (GLchar[]){
#include "smaaEdge.frag"
	'\0'};

static const char* ProgramSmaaBlendVertName = u8"smaaBlend.vert";
static const GLchar* ProgramSmaaBlendVertSrc = (GLchar[]){
#include "smaaBlend.vert"
	'\0'};
static const char* ProgramSmaaBlendFragName = u8"smaaBlend.frag";
static const GLchar* ProgramSmaaBlendFragSrc = (GLchar[]){
#include "smaaBlend.frag"
	'\0'};

static const char* ProgramSmaaNeighborVertName = u8"smaaNeighbor.vert";
static const GLchar* ProgramSmaaNeighborVertSrc = (GLchar[]){
#include "smaaNeighbor.vert"
	'\0'};
static const char* ProgramSmaaNeighborFragName = u8"smaaNeighbor.frag";
static const GLchar* ProgramSmaaNeighborFragSrc = (GLchar[]){
#include "smaaNeighbor.frag"
	'\0'};

static const char* ProgramBlitVertName = u8"blit.vert";
static const GLchar* ProgramBlitVertSrc = (GLchar[]){
#include "blit.vert"
	'\0'};
static const char* ProgramBlitFragName = u8"blit.frag";
static const GLchar* ProgramBlitFragSrc = (GLchar[]){
#include "blit.frag"
	'\0'};

static const char* ProgramThresholdVertName = u8"threshold.vert";
static const GLchar* ProgramThresholdVertSrc = (GLchar[]){
#include "threshold.vert"
	'\0'};
static const char* ProgramThresholdFragName = u8"threshold.frag";
static const GLchar* ProgramThresholdFragSrc = (GLchar[]){
#include "threshold.frag"
	'\0'};

static const char* ProgramBoxBlurVertName = u8"boxBlur.vert";
static const GLchar* ProgramBoxBlurVertSrc = (GLchar[]){
#include "boxBlur.vert"
	'\0'};
static const char* ProgramBoxBlurFragName = u8"boxBlur.frag";
static const GLchar* ProgramBoxBlurFragSrc = (GLchar[]){
#include "boxBlur.frag"
	'\0'};

static bool initialized = false;

static Framebuffer renderFbMS = 0; ///< Main render destination, multisampled
static Texture renderFbMSColor = 0;
static Renderbuffer renderFbMSDepth = 0;
static Framebuffer renderFbSS = 0; ///< Resolved render, for post-processing
static Texture renderFbSSColor = 0;

static Framebuffer smaaSeparateFb = 0;
static Texture smaaSeparateFbColor[2] = {0};
static Framebuffer smaaEdgeFb[2] = {0};
static Renderbuffer smaaEdgeFbStencil = 0;
static Texture smaaEdgeFbColor[2] = {0};
static Framebuffer smaaBlendFb = 0;
static Texture smaaBlendFbColor[2] = {0};
static Texture smaaArea = 0;
static Texture smaaSearch = 0;

static Framebuffer bloomFb[BloomPasses] = {0}; ///< Intermediate bloom results
static Texture bloomFbColor[BloomPasses] = {0};

static size2i viewportSize = {0}; ///< in pixels
static mat4x4 projection = {0}; ///< perspective transform
static mat4x4 camera = {0}; ///< view transform
static point3f lightPosition = {0}; ///< in world space
static color3 lightColor = {0};
static color3 ambientColor = {0};

static Model* sync = null; ///< Invisible model used to prevent frame buffering

static ProgramFlat* flat = null;
static ProgramPhong* phong = null;
static ProgramSmaaSeparate* smaaSeparate = null;
static ProgramSmaaEdge* smaaEdge = null;
static ProgramSmaaBlend* smaaBlend = null;
static ProgramSmaaNeighbor* smaaNeighbor = null;
static ProgramBlit* blit = null;
static ProgramThreshold* threshold = null;
static ProgramBoxBlur* boxBlur = null;

/**
 * Prevent the driver from buffering commands. Call this after windowFlip()
 * to minimize video latency.
 * @see https://danluu.com/latency-mitigation/
 */
static void rendererSync(void)
{
	assert(initialized);
	modelDraw(sync, 1, null, null, &IdentityMatrix);
	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, secToNsec(0.1));
}

/**
 * Resize the rendering viewport, preferably to window size. Recreates the
 * matrices and framebuffers as needed.
 * @param size New viewport size in pixels
 */
static void rendererResize(size2i size)
{
	assert(initialized);
	assert(size.x > 0);
	assert(size.y > 0);
	viewportSize.x = size.x;
	viewportSize.y = size.y;

	// Matrices
	glViewport(0, 0, size.x, size.y);
	mat4x4_perspective(projection, radf(45.0f),
		(float)size.x / (float)size.y, ProjectionNear, ProjectionFar);

	// Framebuffers
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderFbMSColor);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, GL_RGB16F,
		size.x, size.y, GL_TRUE);
	glBindRenderbuffer(GL_RENDERBUFFER, renderFbMSDepth);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 2, GL_DEPTH_COMPONENT,
		size.x, size.y);
	glBindTexture(GL_TEXTURE_2D, renderFbSSColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F,
		size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, null);

	glBindRenderbuffer(GL_RENDERBUFFER, smaaEdgeFbStencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
		size.x, size.y);

	for (size_t i = 0; i < 2; i += 1) {
	glBindTexture(GL_TEXTURE_2D, smaaSeparateFbColor[i]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F,
		size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, null);
		glBindTexture(GL_TEXTURE_2D, smaaEdgeFbColor[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);
		glBindTexture(GL_TEXTURE_2D, smaaBlendFbColor[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);
	}

	for (size_t i = 0; i < BloomPasses; i += 1) {
		glBindTexture(GL_TEXTURE_2D, bloomFbColor[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F,
			size.x >> (i + 1), size.y >> (i + 1), 0, GL_RGB, GL_UNSIGNED_BYTE,
			null);
	}
}

void rendererInit(void)
{
	if (initialized) return;

	// Pick up the OpenGL context
	windowContextActivate();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logCrit(applog, u8"Failed to initialize OpenGL");
		exit(EXIT_FAILURE);
	}
	initialized = true;

	// Set up global OpenGL state
	glfwSwapInterval(1); // Enable vsync
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB);

	// Create framebuffers
	glGenFramebuffers(1, &renderFbMS);
	glGenTextures(1, &renderFbMSColor);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderFbMSColor);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER,
		GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S,
		GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T,
		GL_CLAMP_TO_EDGE);
	glGenRenderbuffers(1, &renderFbMSDepth);

	glGenFramebuffers(1, &renderFbSS);
	glGenTextures(1, &renderFbSSColor);
	glBindTexture(GL_TEXTURE_2D, renderFbSSColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &smaaSeparateFb);
	glGenTextures(2, smaaSeparateFbColor);
	for (size_t i = 0; i < 2; i += 1) {
		glBindTexture(GL_TEXTURE_2D, smaaSeparateFbColor[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glGenFramebuffers(2, smaaEdgeFb);
	glGenTextures(2, smaaEdgeFbColor);
	glGenRenderbuffers(1, &smaaEdgeFbStencil);
	for (size_t i = 0; i < 2; i += 1) {
		glBindTexture(GL_TEXTURE_2D, smaaEdgeFbColor[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glGenFramebuffers(1, &smaaBlendFb);
	glGenTextures(2, smaaBlendFbColor);
	for (size_t i = 0; i < 2; i += 1) {
		glBindTexture(GL_TEXTURE_2D, smaaBlendFbColor[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glGenFramebuffers(BloomPasses, bloomFb);
	glGenTextures(BloomPasses, bloomFbColor);
	for (size_t i = 0; i < BloomPasses; i += 1) {
		glBindTexture(GL_TEXTURE_2D, bloomFbColor[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	// Set up matrices and framebuffer textures
	rendererResize(windowGetSize());

	// Put framebuffers together
	glBindFramebuffer(GL_FRAMEBUFFER, renderFbMS);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D_MULTISAMPLE, renderFbMSColor, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, renderFbMSDepth);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		logCrit(applog, u8"Failed to create the render framebuffer");
		exit(EXIT_FAILURE);
	}
	// Verify that multisampling has the expected subsample layout
	GLfloat sampleLocations[4] = {0};
	glGetMultisamplefv(GL_SAMPLE_POSITION, 0, sampleLocations);
	glGetMultisamplefv(GL_SAMPLE_POSITION, 1, sampleLocations + 2);
	if (sampleLocations[0] != 0.75f ||
		sampleLocations[1] != 0.75f ||
		sampleLocations[2] != 0.25f ||
		sampleLocations[3] != 0.25f) {
		logWarn(applog, "MSAA 2x subsample locations are not as expected:");
		logWarn(applog, "    Subsample #0: (%f, %f), expected (0.75, 0.75)",
			sampleLocations[0], sampleLocations[1]);
		logWarn(applog, "    Subsample #1: (%f, %f), expected (0.25, 0.25)",
			sampleLocations[2], sampleLocations[3]);
#ifdef NDEBUG
		logWarn(applog, "  Graphics will look ugly.");
#else //NDEBUG
		logCrit(applog, "Aborting, please tell the developer that runtime subsample detection is needed");
		exit(EXIT_FAILURE);
#endif //NDEBUG
	}

	glBindFramebuffer(GL_FRAMEBUFFER, renderFbSS);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, renderFbSSColor, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		logCrit(applog, u8"Failed to create the post-processing framebuffer");
		exit(EXIT_FAILURE);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, smaaSeparateFb);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, smaaSeparateFbColor[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, smaaSeparateFbColor[1], 0);
	glDrawBuffers(2, (GLuint[]){GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		logCrit(applog, u8"Failed to create the SMAA separate framebuffer");
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < 2; i += 1) {
		glBindFramebuffer(GL_FRAMEBUFFER, smaaEdgeFb[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, smaaEdgeFbColor[i], 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
			GL_RENDERBUFFER, smaaEdgeFbStencil);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
			!= GL_FRAMEBUFFER_COMPLETE) {
			logCrit(applog, u8"Failed to create the SMAA edge framebuffer #%zu", i);
			exit(EXIT_FAILURE);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, smaaBlendFb);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, smaaBlendFbColor[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, smaaBlendFbColor[1], 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
		GL_RENDERBUFFER, smaaEdgeFbStencil);
	glDrawBuffers(2, (GLuint[]){GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
		!= GL_FRAMEBUFFER_COMPLETE) {
		logCrit(applog, u8"Failed to create the SMAA blend framebuffer");
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < BloomPasses; i += 1) {
		glBindFramebuffer(GL_FRAMEBUFFER, bloomFb[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, bloomFbColor[i], 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
			!= GL_FRAMEBUFFER_COMPLETE) {
			logCrit(applog, u8"Failed to create the bloom framebuffer #%zu", i);
			exit(EXIT_FAILURE);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Create built-in shaders
	flat = programCreate(ProgramFlat,
		ProgramFlatVertName, ProgramFlatVertSrc,
		ProgramFlatFragName, ProgramFlatFragSrc);
	flat->projection = programUniform(flat, u8"projection");
	flat->camera = programUniform(flat, u8"camera");

	phong = programCreate(ProgramPhong,
		ProgramPhongVertName, ProgramPhongVertSrc,
		ProgramPhongFragName, ProgramPhongFragSrc);
	phong->projection = programUniform(phong, u8"projection");
	phong->camera = programUniform(phong, u8"camera");
	phong->lightPosition = programUniform(phong, u8"lightPosition");
	phong->lightColor = programUniform(phong, u8"lightColor");
	phong->ambientColor = programUniform(phong, u8"ambientColor");
	phong->ambient = programUniform(phong, u8"ambient");
	phong->diffuse = programUniform(phong, u8"diffuse");
	phong->specular = programUniform(phong, u8"specular");
	phong->shine = programUniform(phong, u8"shine");

	smaaSeparate = programCreate(ProgramSmaaSeparate,
		ProgramSmaaSeparateVertName, ProgramSmaaSeparateVertSrc,
		ProgramSmaaSeparateFragName, ProgramSmaaSeparateFragSrc);
	smaaSeparate->image = programSampler(smaaSeparate, u8"image", GL_TEXTURE0);

	smaaEdge = programCreate(ProgramSmaaEdge,
		ProgramSmaaEdgeVertName, ProgramSmaaEdgeVertSrc,
		ProgramSmaaEdgeFragName, ProgramSmaaEdgeFragSrc);
	smaaEdge->image = programSampler(smaaEdge, u8"image", GL_TEXTURE0);
	smaaEdge->screenSize = programUniform(smaaEdge, u8"screenSize");

	smaaBlend = programCreate(ProgramSmaaBlend,
		ProgramSmaaBlendVertName, ProgramSmaaBlendVertSrc,
		ProgramSmaaBlendFragName, ProgramSmaaBlendFragSrc);
	smaaBlend->edges1 = programSampler(smaaBlend, u8"edges1", GL_TEXTURE0);
	smaaBlend->edges2 = programSampler(smaaBlend, u8"edges2", GL_TEXTURE1);
	smaaBlend->area = programSampler(smaaBlend, u8"area", GL_TEXTURE2);
	smaaBlend->search = programSampler(smaaBlend, u8"search", GL_TEXTURE3);
	smaaBlend->screenSize = programUniform(smaaBlend, u8"screenSize");

	smaaNeighbor = programCreate(ProgramSmaaNeighbor,
		ProgramSmaaNeighborVertName, ProgramSmaaNeighborVertSrc,
		ProgramSmaaNeighborFragName, ProgramSmaaNeighborFragSrc);
	smaaNeighbor->image1 = programSampler(smaaNeighbor, u8"image1", GL_TEXTURE0);
	smaaNeighbor->image2 = programSampler(smaaNeighbor, u8"image2", GL_TEXTURE1);
	smaaNeighbor->blend1 = programSampler(smaaNeighbor, u8"blend1", GL_TEXTURE2);
	smaaNeighbor->blend2 = programSampler(smaaNeighbor, u8"blend2", GL_TEXTURE3);
	smaaNeighbor->screenSize = programUniform(smaaNeighbor, u8"screenSize");

	blit = programCreate(ProgramBlit,
		ProgramBlitVertName, ProgramBlitVertSrc,
		ProgramBlitFragName, ProgramBlitFragSrc);
	blit->image = programSampler(blit, u8"image", GL_TEXTURE0);
	blit->boost = programUniform(blit, u8"boost");

	threshold = programCreate(ProgramThreshold,
		ProgramThresholdVertName, ProgramThresholdVertSrc,
		ProgramThresholdFragName, ProgramThresholdFragSrc);
	threshold->image = programSampler(threshold, u8"image", GL_TEXTURE0);
	threshold->threshold = programUniform(threshold, u8"threshold");
	threshold->softKnee = programUniform(threshold, u8"softKnee");
	threshold->strength = programUniform(threshold, u8"strength");
	
	boxBlur = programCreate(ProgramBoxBlur,
		ProgramBoxBlurVertName, ProgramBoxBlurVertSrc,
		ProgramBoxBlurFragName, ProgramBoxBlurFragSrc);
	boxBlur->image = programSampler(boxBlur, u8"image", GL_TEXTURE0);
	boxBlur->step = programUniform(boxBlur, u8"step");
	boxBlur->imageTexel = programUniform(boxBlur, u8"imageTexel");

	// Load lookup textures
	unsigned char areaTexBytesFlipped[AREATEX_SIZE] = {0};
	for (size_t i = 0; i < AREATEX_HEIGHT; i += 1)
		memcpy(areaTexBytesFlipped + i * AREATEX_PITCH,
			areaTexBytes + (AREATEX_HEIGHT - 1 - i) * AREATEX_PITCH,
			AREATEX_PITCH);

	glGenTextures(1, &smaaArea);
	glBindTexture(GL_TEXTURE_2D, smaaArea);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RG_RGTC2,
		AREATEX_WIDTH, AREATEX_HEIGHT, 0,
		GL_RG, GL_UNSIGNED_BYTE, areaTexBytesFlipped);

	unsigned char searchTexBytesFlipped[SEARCHTEX_SIZE] = {0};
	for (size_t i = 0; i < SEARCHTEX_HEIGHT; i += 1)
		memcpy(searchTexBytesFlipped + i * SEARCHTEX_PITCH,
			searchTexBytes + (SEARCHTEX_HEIGHT - 1 - i) * SEARCHTEX_PITCH,
			SEARCHTEX_PITCH);

	glGenTextures(1, &smaaSearch);
	glBindTexture(GL_TEXTURE_2D, smaaSearch);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RED_RGTC1,
		SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 0,
		GL_RED, GL_UNSIGNED_BYTE, searchTexBytesFlipped);

	// Set up the camera and light globals
	vec3 eye = {0.0f, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(camera, eye, center, up);
	lightPosition.x = -8.0f;
	lightPosition.y = 32.0f;
	lightPosition.z = 16.0f;
	lightColor.r = 1.0f;
	lightColor.g = 1.0f;
	lightColor.b = 1.0f;
	ambientColor.r = 1.0f;
	ambientColor.g = 1.0f;
	ambientColor.b = 1.0f;

	// Create sync model
	sync = modelCreateFlat(u8"sync", 3, (VertexFlat[]){
		{
			.pos = {0.0f, 0.0f, 0.0f},
			.color = Color4Clear
		},
		{
			.pos = {1.0f, 0.0f, 0.0f},
			.color = Color4Clear
		},
		{
			.pos = {0.0f, 1.0f, 0.0f},
			.color = Color4Clear
		}
	});

	logDebug(applog, u8"Created renderer for window \"%s\"", windowGetTitle());
}

void rendererCleanup(void)
{
	if (!initialized) return;
	if (sync) {
		modelDestroy(sync);
		sync = null;
	}
	if (smaaSearch) {
		glDeleteTextures(1, &smaaSearch);
		smaaSearch = null;
	}
	if (smaaArea) {
		glDeleteTextures(1, &smaaArea);
		smaaArea = null;
	}
	if (boxBlur) {
		programDestroy(boxBlur);
		boxBlur = null;
	}
	if (threshold) {
		programDestroy(threshold);
		threshold = null;
	}
	if (blit) {
		programDestroy(blit);
		blit = null;
	}
	if (smaaNeighbor) {
		programDestroy(smaaNeighbor);
		smaaNeighbor = null;
	}
	if (smaaBlend) {
		programDestroy(smaaBlend);
		smaaBlend = null;
	}
	if (smaaEdge) {
		programDestroy(smaaEdge);
		smaaEdge = null;
	}
	if (phong) {
		programDestroy(phong);
		phong = null;
	}
	if (flat) {
		programDestroy(flat);
		flat = null;
	}
	if (bloomFbColor[0]) {
		glDeleteTextures(BloomPasses, bloomFbColor);
		arrayClear(bloomFbColor);
	}
	if (bloomFb[0]) {
		glDeleteFramebuffers(BloomPasses, bloomFb);
		arrayClear(bloomFb);
	}
	if (smaaBlendFbColor[0]) {
		glDeleteTextures(2, smaaBlendFbColor);
		arrayClear(smaaBlendFbColor);
	}
	if (smaaBlendFb) {
		glDeleteFramebuffers(1, &smaaBlendFb);
		smaaBlendFb = 0;
	}
	if (smaaEdgeFbStencil) {
		glDeleteRenderbuffers(1, &smaaEdgeFbStencil);
		smaaEdgeFbStencil = 0;
	}
	if (smaaEdgeFbColor[0]) {
		glDeleteTextures(2, smaaEdgeFbColor);
		arrayClear(smaaEdgeFbColor);
	}
	if (smaaEdgeFb[0]) {
		glDeleteFramebuffers(2, smaaEdgeFb);
		arrayClear(smaaEdgeFb);
	}
	if (smaaSeparateFbColor[0]) {
		glDeleteTextures(2, smaaSeparateFbColor);
		arrayClear(smaaSeparateFbColor);
	}
	if (smaaSeparateFb) {
		glDeleteFramebuffers(1, &smaaSeparateFb);
		smaaSeparateFb = 0;
	}
	if (renderFbSSColor) {
		glDeleteTextures(1, &renderFbSSColor);
		renderFbSSColor = 0;
	}
	if (renderFbSS) {
		glDeleteFramebuffers(1, &renderFbSS);
		renderFbSS = 0;
	}
	if (renderFbMSDepth) {
		glDeleteRenderbuffers(1, &renderFbMSDepth);
		renderFbMSDepth = 0;
	}
	if (renderFbMSColor) {
		glDeleteTextures(1, &renderFbMSColor);
		renderFbMSColor = 0;
	}
	if (renderFbMS) {
		glDeleteFramebuffers(1, &renderFbMS);
		renderFbMS = 0;
	}
	windowContextDeactivate();
	logDebug(applog, u8"Destroyed renderer for window \"%s\"",
		windowGetTitle());
	initialized = false;
}

void rendererClear(color3 color)
{
	assert(initialized);
	color3Copy(ambientColor, color);
	glClearColor(color.r, color.g, color.b, 1.0f);
	glClear((GLuint)GL_COLOR_BUFFER_BIT | (GLuint)GL_DEPTH_BUFFER_BIT);
}

void rendererFrameBegin(void)
{
	assert(initialized);
	size2i windowSize = windowGetSize();
	if (viewportSize.x != windowSize.x || viewportSize.y != windowSize.y)
		rendererResize(windowSize);

	vec3 eye = {0.0f, 12.0f, 32.0f};
	vec3 center = {0.0f, 12.0f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	mat4x4_look_at(camera, eye, center, up);
	lightPosition.x = -8.0f;
	lightPosition.y = 32.0f;
	lightPosition.z = 16.0f;
	lightColor.r = 1.0f;
	lightColor.g = 1.0f;
	lightColor.b = 1.0f;
	glBindFramebuffer(GL_FRAMEBUFFER, renderFbMS);
}

void rendererResolveAA(void)
{
	assert(initialized);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// SMAA sample separation pass
	programUse(smaaSeparate);
	glBindFramebuffer(GL_FRAMEBUFFER, smaaSeparateFb);
	glActiveTexture(smaaSeparate->image);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderFbMSColor);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// SMAA edge detection pass
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	programUse(smaaEdge);
	for (size_t i = 0; i < 2; i += 1) {
		glBindFramebuffer(GL_FRAMEBUFFER, smaaEdgeFb[i]);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | (i? 0 : GL_STENCIL_BUFFER_BIT));
		glActiveTexture(smaaEdge->image);
		glBindTexture(GL_TEXTURE_2D, smaaSeparateFbColor[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glUniform4f(smaaEdge->screenSize,
			1.0 / (float)viewportSize.x, 1.0 / (float)viewportSize.y,
			viewportSize.x, viewportSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// SMAA blending weight calculation pass
	programUse(smaaBlend);
	glBindFramebuffer(GL_FRAMEBUFFER, smaaBlendFb);
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glClear(GL_COLOR_BUFFER_BIT);
	glActiveTexture(smaaBlend->edges1);
	glBindTexture(GL_TEXTURE_2D, smaaEdgeFbColor[0]);
	glActiveTexture(smaaBlend->edges2);
	glBindTexture(GL_TEXTURE_2D, smaaEdgeFbColor[1]);
	glActiveTexture(smaaBlend->area);
	glBindTexture(GL_TEXTURE_2D, smaaArea);
	glActiveTexture(smaaBlend->search);
	glBindTexture(GL_TEXTURE_2D, smaaSearch);
	glUniform4f(smaaBlend->screenSize,
		1.0 / (float)viewportSize.x, 1.0 / (float)viewportSize.y,
		viewportSize.x, viewportSize.y);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisable(GL_STENCIL_TEST);

	// SMAA neighbor blending pass
	programUse(smaaNeighbor);
	glBindFramebuffer(GL_FRAMEBUFFER, renderFbSS);
	glActiveTexture(smaaNeighbor->image1);
	glBindTexture(GL_TEXTURE_2D, smaaSeparateFbColor[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glActiveTexture(smaaNeighbor->image2);
	glBindTexture(GL_TEXTURE_2D, smaaSeparateFbColor[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glActiveTexture(smaaNeighbor->blend1);
	glBindTexture(GL_TEXTURE_2D, smaaBlendFbColor[0]);
	glActiveTexture(smaaNeighbor->blend2);
	glBindTexture(GL_TEXTURE_2D, smaaBlendFbColor[1]);
	glUniform4f(smaaNeighbor->screenSize,
		1.0 / (float)viewportSize.x, 1.0 / (float)viewportSize.y,
		viewportSize.x, viewportSize.y);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
}

void rendererFrameEnd(void)
{
	assert(initialized);

	// Prepare the image for bloom
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFb[0]);
	glViewport(0, 0, viewportSize.x >> 1, viewportSize.y >> 1);
	programUse(threshold);
	glActiveTexture(threshold->image);
	glBindTexture(GL_TEXTURE_2D, renderFbSSColor);
	glUniform1f(threshold->threshold, 1.0f);
	glUniform1f(threshold->softKnee, 0.25f);
	glUniform1f(threshold->strength, 1.0f);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Blur the bloom image
	programUse(boxBlur);
	for (size_t i = 0; i < BloomPasses - 1; i += 1) {
		glBindFramebuffer(GL_FRAMEBUFFER, bloomFb[i + 1]);
		glViewport(0, 0, viewportSize.x >> (i + 2), viewportSize.y >> (i + 2));
		glActiveTexture(boxBlur->image);
		glBindTexture(GL_TEXTURE_2D, bloomFbColor[i]);
		glUniform1f(boxBlur->step, 1.0f);
		glUniform2f(boxBlur->imageTexel,
			1.0 / (float)(viewportSize.x >> (i + 1)),
			1.0 / (float)(viewportSize.y >> (i + 1)));
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	for (size_t i = BloomPasses - 2; i < BloomPasses; i -= 1) {
		glBindFramebuffer(GL_FRAMEBUFFER, bloomFb[i]);
		glViewport(0, 0, viewportSize.x >> (i + 1), viewportSize.y >> (i + 1));
		glActiveTexture(boxBlur->image);
		glBindTexture(GL_TEXTURE_2D, bloomFbColor[i + 1]);
		glUniform1f(boxBlur->step, 0.5f);
		glUniform2f(boxBlur->imageTexel,
			1.0 / (float)(viewportSize.x >> (i + 2)),
			1.0 / (float)(viewportSize.y >> (i + 2)));
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// Draw the bloom on top of the render
	programUse(blit);
	glBindFramebuffer(GL_FRAMEBUFFER, renderFbSS);
	glViewport(0, 0, viewportSize.x, viewportSize.y);
	glActiveTexture(blit->image);
	glBindTexture(GL_TEXTURE_2D, bloomFbColor[0]);
	glUniform1f(blit->boost, 2.0f);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, viewportSize.x, viewportSize.y,
		0, 0, viewportSize.x, viewportSize.y,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// Present the frame
	windowFlip();
	rendererSync();
}

void rendererDepthOnlyBegin(void)
{
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void rendererDepthOnlyEnd(void)
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

////////////////////////////////////////////////////////////////////////////////

/// Semantic rename of OpenGL vertex buffer object ID
typedef GLuint VertexBuffer;

/// Semantic rename of OpenGL vertex array object ID
typedef GLuint VertexArray;

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
} ModelBase;

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
 * Destroy a ::ModelFlat instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param m The ::ModelFlat object to destroy
 */
static void modelDestroyFlat(ModelFlat* m)
{
	assert(initialized);
	assert(m);
	glDeleteVertexArrays(1, &m->vao);
	m->vao = 0;
	glDeleteBuffers(1, &m->transforms);
	m->transforms = 0;
	glDeleteBuffers(1, &m->highlights);
	m->highlights = 0;
	glDeleteBuffers(1, &m->tints);
	m->tints = 0;
	glDeleteBuffers(1, &m->vertices);
	m->vertices = 0;
	logDebug(applog, u8"Model %s destroyed", m->base.name);
	free(m);
	m = null;
}

/**
 * Destroy a ::ModelPhong instance. All referenced GPU resources are freed. The
 * destroyed object cannot be used anymore and the pointer becomes invalid.
 * @param m The ::ModelPhong object to destroy
 */
static void modelDestroyPhong(ModelPhong* m)
{
	assert(initialized);
	assert(m);
	glDeleteVertexArrays(1, &m->vao);
	m->vao = 0;
	glDeleteBuffers(1, &m->transforms);
	m->transforms = 0;
	glDeleteBuffers(1, &m->highlights);
	m->highlights = 0;
	glDeleteBuffers(1, &m->tints);
	m->tints = 0;
	glDeleteBuffers(1, &m->normals);
	m->normals = 0;
	glDeleteBuffers(1, &m->vertices);
	m->vertices = 0;
	logDebug(applog, u8"Model %s destroyed", m->base.name);
	free(m);
	m = null;
}

/**
 * Draw a ::ModelFlat on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param m The ::ModelFlat object to draw
 * @param instances Number of instances to draw
 * @param tints Color tints for each instance. Can be null.
 * @param highlights Highlight colors to blend into each instance. Can be null
 * @param transforms 4x4 matrices for transforming each instance
 */
static void modelDrawFlat(ModelFlat* m, size_t instances,
	color4 tints[instances], color4 highlights[instances],
	mat4x4 transforms[instances])
{
	assert(initialized);
	assert(m);
	assert(m->base.type == ModelTypeFlat);
	assert(m->vao);
	assert(m->base.name);
	assert(m->vertices);
	assert(m->tints);
	assert(m->transforms);
	assert(transforms);
	if (!instances) return;

	glBindVertexArray(m->vao);
	programUse(flat);
	if (tints) {
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, m->tints);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, null,
			GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances, tints);
	} else {
		glDisableVertexAttribArray(2);
		glVertexAttrib4f(2, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (highlights) {
		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, m->highlights);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, null,
			GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances,
			highlights);
	} else {
		glDisableVertexAttribArray(3);
		glVertexAttrib4f(3, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * instances, null,
		GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mat4x4) * instances, transforms);
	glUniformMatrix4fv(flat->projection, 1, GL_FALSE, projection[0]);
	glUniformMatrix4fv(flat->camera, 1, GL_FALSE, camera[0]);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}

/**
 * Draw a ::ModelPhong on the screen. Instanced rendering is used, and each
 * instance can be tinted with a provided color.
 * @param m The ::ModelPhong object to draw
 * @param instances Number of instances to draw
 * @param tints Color tints for each instance. Can be null
 * @param highlights Highlight colors to blend into each instance. Can be null
 * @param transforms 4x4 matrices for transforming each instance
 */
static void modelDrawPhong(ModelPhong* m, size_t instances,
	color4 tints[instances], color4 highlights[instances],
	mat4x4 transforms[instances])
{
	assert(initialized);
	assert(m);
	assert(m->base.type == ModelTypePhong);
	assert(m->vao);
	assert(m->base.name);
	assert(m->vertices);
	assert(m->normals);
	assert(m->tints);
	assert(m->transforms);
	assert(transforms);
	if (!instances) return;

	glBindVertexArray(m->vao);
	programUse(phong);
	if (tints) {
		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, m->tints);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, null,
			GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances, tints);
	} else {
		glDisableVertexAttribArray(3);
		glVertexAttrib4f(3, 1.0f, 1.0f, 1.0f, 1.0f);
	}
	if (highlights) {
		glEnableVertexAttribArray(4);
		glBindBuffer(GL_ARRAY_BUFFER, m->highlights);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color4) * instances, null,
			GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color4) * instances,
			highlights);
	} else {
		glDisableVertexAttribArray(4);
		glVertexAttrib4f(4, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * instances, null,
		GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mat4x4) * instances, transforms);
	glUniformMatrix4fv(phong->projection, 1, GL_FALSE, projection[0]);
	glUniformMatrix4fv(phong->camera, 1, GL_FALSE, camera[0]);
	glUniform3fv(phong->lightPosition, 1, lightPosition.arr);
	glUniform3fv(phong->lightColor, 1, lightColor.arr);
	glUniform3fv(phong->ambientColor, 1, ambientColor.arr);
	glUniform1f(phong->ambient, m->material.ambient);
	glUniform1f(phong->diffuse, m->material.diffuse);
	glUniform1f(phong->specular, m->material.specular);
	glUniform1f(phong->shine, m->material.shine);
	glDrawArraysInstanced(GL_TRIANGLES, 0, m->numVertices, instances);
}

/**
 * Generate normal vectors for a given triangle mesh.
 * @param numVertices Number of vertices in @a vertices and @a normalData
 * @param vertices Model mesh made out of triangles
 * @param[out] normalData Normal vectors corresponding to the vertex mesh
 */
static void modelGenerateNormals(size_t numVertices,
	VertexPhong vertices[numVertices], point3f normalData[numVertices])
{
	assert(initialized);
	assert(numVertices);
	assert(vertices);
	assert(normalData);
	assert(numVertices % 3 == 0);
	for (size_t i = 0; i < numVertices; i += 3) {
		vec3 v0 = {vertices[i + 0].pos.x,
		           vertices[i + 0].pos.y,
		           vertices[i + 0].pos.z};
		vec3 v1 = {vertices[i + 1].pos.x,
		           vertices[i + 1].pos.y,
		           vertices[i + 1].pos.z};
		vec3 v2 = {vertices[i + 2].pos.x,
		           vertices[i + 2].pos.y,
		           vertices[i + 2].pos.z};
		vec3 u = {0};
		vec3 v = {0};
		vec3_sub(u, v1, v0);
		vec3_sub(v, v2, v0);
		vec3 normal = {0};
		vec3_mul_cross(normal, u, v);
		vec3_norm(normal, normal);
		normalData[i + 0].x = normal[0];
		normalData[i + 0].y = normal[1];
		normalData[i + 0].z = normal[2];
		normalData[i + 1].x = normal[0];
		normalData[i + 1].y = normal[1];
		normalData[i + 1].z = normal[2];
		normalData[i + 2].x = normal[0];
		normalData[i + 2].y = normal[1];
		normalData[i + 2].z = normal[2];
	}
}

Model* modelCreateFlat(const char* name,
	size_t numVertices, VertexFlat vertices[])
{
	assert(initialized);
	assert(name);
	assert(numVertices);
	assert(vertices);
	ModelFlat* m = alloc(sizeof(*m));
	m->base.type = ModelTypeFlat;
	m->base.name = name;
	m->numVertices = numVertices;
	glGenBuffers(1, &m->vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexFlat) * m->numVertices, vertices,
		GL_STATIC_DRAW);
	glGenBuffers(1, &m->tints);
	glGenBuffers(1, &m->highlights);
	glGenBuffers(1, &m->transforms);
	glGenVertexArrays(1, &m->vao);
	glBindVertexArray(m->vao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFlat),
		(void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexFlat),
		(void*)offsetof(VertexFlat, color));
	glBindBuffer(GL_ARRAY_BUFFER, m->tints);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, m->highlights);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(3, 1);
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)0);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)sizeof(vec4));
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)(sizeof(vec4) * 2));
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)(sizeof(vec4) * 3));
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);
	logDebug(applog, u8"Model %s created", m->base.name);
	return (Model*)m;
}

Model* modelCreatePhong(const char* name,
	size_t numVertices, VertexPhong vertices[], MaterialPhong material)
{
	assert(initialized);
	assert(name);
	assert(numVertices);
	assert(vertices);
	ModelPhong* m = alloc(sizeof(*m));
	m->base.type = ModelTypePhong;
	m->base.name = name;
	structCopy(m->material, material);

	m->numVertices = numVertices;
	glGenBuffers(1, &m->vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPhong) * m->numVertices,
		vertices, GL_STATIC_DRAW);
	point3f normalData[m->numVertices];
	assert(sizeof(normalData) == sizeof(point3f) * m->numVertices);
	arrayClear(normalData);
	modelGenerateNormals(m->numVertices, vertices, normalData);
	glGenBuffers(1, &m->normals);
	glBindBuffer(GL_ARRAY_BUFFER, m->normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normalData), normalData,
		GL_STATIC_DRAW);
	glGenBuffers(1, &m->tints);
	glGenBuffers(1, &m->highlights);
	glGenBuffers(1, &m->transforms);

	glGenVertexArrays(1, &m->vao);
	glBindVertexArray(m->vao);
	glBindBuffer(GL_ARRAY_BUFFER, m->vertices);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPhong),
		(void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexPhong),
		(void*)offsetof(VertexPhong, color));
	glBindBuffer(GL_ARRAY_BUFFER, m->normals);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(point3f),
		(void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, m->tints);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(3, 1);
	glBindBuffer(GL_ARRAY_BUFFER, m->highlights);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(color4),
		(void*)0);
	glVertexAttribDivisor(4, 1);
	glBindBuffer(GL_ARRAY_BUFFER, m->transforms);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	glEnableVertexAttribArray(7);
	glEnableVertexAttribArray(8);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)0);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)sizeof(vec4));
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)(sizeof(vec4) * 2));
	glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(mat4x4),
		(void*)(sizeof(vec4) * 3));
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);
	glVertexAttribDivisor(8, 1);

	logDebug(applog, u8"Model %s created", m->base.name);
	return (Model*)m;
}

void modelDestroy(Model* m)
{
	assert(m);
	assert(m->type > ModelTypeNone && m->type < ModelTypeSize);
	switch (m->type) {
	case ModelTypeFlat:
		modelDestroyFlat((ModelFlat*)m);
		break;
	case ModelTypePhong:
		modelDestroyPhong((ModelPhong*)m);
		break;
	default:
		assert(false);
	}
}

void modelDraw(Model* m, size_t instances,
	color4 tints[instances], color4 highlights[instances],
	mat4x4 transforms[instances])
{
	assert(m);
	assert(m->type > ModelTypeNone && m->type < ModelTypeSize);
	assert(transforms);

	switch (m->type) {
	case ModelTypeFlat:
		modelDrawFlat((ModelFlat*)m, instances, tints, highlights, transforms);
		break;
	case ModelTypePhong:
		modelDrawPhong((ModelPhong*)m, instances, tints, highlights,
			transforms);
		break;
	default:
		assert(false);
	}
}
