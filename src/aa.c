/**
 * Implementation of aa.h
 * @file
 */

#include "aa.h"

#include <assert.h>
#include "smaa/AreaTex.h"
#include "smaa/SearchTex.h"
#include "basetypes.h"
#include "window.h"
#include "renderer.h"
#include "util.h"
#include "log.h"

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

static Framebuffer* msaaFb = null;
static TextureMS* msaaFbColor = null;
static RenderbufferMS* msaaFbDepthStencil = null;

static Framebuffer* smaaSeparateFb = null;
static Framebuffer* smaaEdgeFb[2] = {null};
static Framebuffer* smaaBlendFb = null;
static Texture* smaaSeparateFbColor[2] = {null};
static Texture* smaaEdgeFbColor[2] = {null};
static Texture* smaaBlendFbColor[2] = {null};
static Renderbuffer* smaaEdgeFbDepthStencil = null;

static Texture* smaaArea = null;
static Texture* smaaSearch = null;

static ProgramSmaaSeparate* smaaSeparate = null;
static ProgramSmaaEdge* smaaEdge = null;
static ProgramSmaaBlend* smaaBlend = null;
static ProgramSmaaNeighbor* smaaNeighbor = null;

static AAMode current = AANone;
static size2i currentSize = {0};

static bool initialized = false;

/**
 * Ensure that AA framebuffers are of the same size as the screen. This can be
 * run every frame with the current size of the screen.
 * @param size Current screen size
 */
static void aaResize(size2i size)
{
	assert(size.x > 0);
	assert(size.y > 0);
	if (size.x == currentSize.x && size.y == currentSize.y) return;
	currentSize.x = size.x;
	currentSize.y = size.y;

	textureMSStorage(msaaFbColor, size, GL_RGBA16F, 2);
	renderbufferMSStorage(msaaFbDepthStencil, size, GL_DEPTH24_STENCIL8, 2);

	for (size_t i = 0; i < 2; i += 1) {
		textureStorage(smaaSeparateFbColor[i], size, GL_RGBA16F);
		textureStorage(smaaEdgeFbColor[i], size, GL_RGBA8);
		textureStorage(smaaBlendFbColor[i], size, GL_RGBA8);
	}
	renderbufferStorage(smaaEdgeFbDepthStencil, size, GL_DEPTH24_STENCIL8);
}

void aaInit(AAMode type)
{
	if (initialized) return;

	msaaFb = framebufferCreate();
	msaaFbColor = textureMSCreate();
	msaaFbDepthStencil = renderbufferMSCreate();

	smaaSeparateFb = framebufferCreate();
	smaaBlendFb = framebufferCreate();
	for (size_t i = 0; i < 2; i += 1) {
		smaaEdgeFb[i] = framebufferCreate();
		smaaSeparateFbColor[i] = textureCreate();
		smaaEdgeFbColor[i] = textureCreate();
		smaaBlendFbColor[i] = textureCreate();
	}
	smaaEdgeFbDepthStencil = renderbufferCreate();

	// Set up framebuffer textures
	aaResize(windowGetSize());

	// Put framebuffers together
	framebufferTextureMS(msaaFb, msaaFbColor, GL_COLOR_ATTACHMENT0);
	framebufferRenderbufferMS(msaaFb, msaaFbDepthStencil,
		GL_DEPTH_STENCIL_ATTACHMENT);
	if (!framebufferCheck(msaaFb)) {
		logCrit(applog, u8"Failed to create the render framebuffer");
		exit(EXIT_FAILURE);
	}
	// Verify that multisampling has the expected subsample layout
	framebufferUse(msaaFb);
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
		logCrit(applog,
			"Aborting, please tell the developer that runtime subsample detection is needed");
		exit(EXIT_FAILURE);
#endif //NDEBUG
	}

	framebufferTexture(smaaSeparateFb, smaaSeparateFbColor[0],
		GL_COLOR_ATTACHMENT0);
	framebufferTexture(smaaSeparateFb, smaaSeparateFbColor[1],
		GL_COLOR_ATTACHMENT1);
	framebufferBuffers(smaaSeparateFb, 2);
	if (!framebufferCheck(smaaSeparateFb)) {
		logCrit(applog, u8"Failed to create the SMAA separate framebuffer");
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < 2; i += 1) {
		framebufferTexture(smaaEdgeFb[i], smaaEdgeFbColor[i],
			GL_COLOR_ATTACHMENT0);
		framebufferRenderbuffer(smaaEdgeFb[i], smaaEdgeFbDepthStencil,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(smaaEdgeFb[i])) {
			logCrit(applog, u8"Failed to create the SMAA edge framebuffer #%zu",
				i);
			exit(EXIT_FAILURE);
		}
	}

	framebufferTexture(smaaBlendFb, smaaBlendFbColor[0], GL_COLOR_ATTACHMENT0);
	framebufferTexture(smaaBlendFb, smaaBlendFbColor[1], GL_COLOR_ATTACHMENT1);
	framebufferRenderbuffer(smaaBlendFb, smaaEdgeFbDepthStencil,
		GL_DEPTH_STENCIL_ATTACHMENT);
	framebufferBuffers(smaaBlendFb, 2);
	if (!framebufferCheck(smaaBlendFb)) {
		logCrit(applog, u8"Failed to create the SMAA blend framebuffer");
		exit(EXIT_FAILURE);
	}

	framebufferUse(rendererFramebuffer());

	// Create shaders
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
	smaaNeighbor->image1 = programSampler(smaaNeighbor, u8"image1",
		GL_TEXTURE0);
	smaaNeighbor->image2 = programSampler(smaaNeighbor, u8"image2",
		GL_TEXTURE1);
	smaaNeighbor->blend1 = programSampler(smaaNeighbor, u8"blend1",
		GL_TEXTURE2);
	smaaNeighbor->blend2 = programSampler(smaaNeighbor, u8"blend2",
		GL_TEXTURE3);
	smaaNeighbor->screenSize = programUniform(smaaNeighbor, u8"screenSize");

	// Load lookup textures
	unsigned char areaTexBytesFlipped[AREATEX_SIZE] = {0};
	for (size_t i = 0; i < AREATEX_HEIGHT; i += 1)
		memcpy(areaTexBytesFlipped + i * AREATEX_PITCH,
			areaTexBytes + (AREATEX_HEIGHT - 1 - i) * AREATEX_PITCH,
			AREATEX_PITCH);

	smaaArea = textureCreate();
	textureStorage(smaaArea, (size2i){AREATEX_WIDTH, AREATEX_HEIGHT}, GL_RG8);
	textureData(smaaArea, areaTexBytesFlipped, GL_RG, GL_UNSIGNED_BYTE);

	unsigned char searchTexBytesFlipped[SEARCHTEX_SIZE] = {0};
	for (size_t i = 0; i < SEARCHTEX_HEIGHT; i += 1)
		memcpy(searchTexBytesFlipped + i * SEARCHTEX_PITCH,
			searchTexBytes + (SEARCHTEX_HEIGHT - 1 - i) * SEARCHTEX_PITCH,
			SEARCHTEX_PITCH);

	smaaSearch = textureCreate();
	textureFilter(smaaSearch, GL_NEAREST);
	textureStorage(smaaSearch, (size2i){SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT},
		GL_RG8);
	textureData(smaaSearch, searchTexBytesFlipped, GL_RED, GL_UNSIGNED_BYTE);

	initialized = true;
}

void aaCleanup(void)
{
	if (!initialized) return;
	framebufferDestroy(msaaFb);
	msaaFb = null;
	textureMSDestroy(msaaFbColor);
	msaaFbColor = null;
	renderbufferMSDestroy(msaaFbDepthStencil);
	msaaFbDepthStencil = null;

	framebufferDestroy(smaaSeparateFb);
	smaaSeparateFb = null;
	framebufferDestroy(smaaBlendFb);
	smaaBlendFb = null;
	for (size_t i = 0; i < 2; i += 1) {
		framebufferDestroy(smaaEdgeFb[i]);
		smaaEdgeFb[i] = null;
		textureDestroy(smaaBlendFbColor[i]);
		smaaBlendFbColor[i] = null;
		textureDestroy(smaaEdgeFbColor[i]);
		smaaEdgeFbColor[i] = null;
		textureDestroy(smaaSeparateFbColor[i]);
		smaaSeparateFbColor[i] = null;
	}
	renderbufferDestroy(smaaEdgeFbDepthStencil);
	smaaEdgeFbDepthStencil = null;

	textureDestroy(smaaArea);
	smaaArea = null;
	textureDestroy(smaaSearch);
	smaaSearch = null;

	initialized = false;
}

void aaSwitch(AAMode type)
{
	assert(initialized);
}

void aaBegin(void)
{
	assert(initialized);
	aaResize(windowGetSize());
	framebufferUse(msaaFb);
}

void aaEnd(void)
{
	assert(initialized);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// SMAA sample separation pass
	programUse(smaaSeparate);
	framebufferUse(smaaSeparateFb);
	textureMSUse(msaaFbColor, smaaSeparate->image);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// SMAA edge detection pass
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	programUse(smaaEdge);
	for (size_t i = 0; i < 2; i += 1) {
		framebufferUse(smaaEdgeFb[i]);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | (i ? 0 : GL_STENCIL_BUFFER_BIT));
		textureUse(smaaSeparateFbColor[i], smaaEdge->image);
		textureFilter(smaaSeparateFbColor[i], GL_NEAREST);
		glUniform4f(smaaEdge->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// SMAA blending weight calculation pass
	programUse(smaaBlend);
	framebufferUse(smaaBlendFb);
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glClear(GL_COLOR_BUFFER_BIT);
	textureUse(smaaEdgeFbColor[0], smaaBlend->edges1);
	textureUse(smaaEdgeFbColor[1], smaaBlend->edges2);
	textureUse(smaaArea, smaaBlend->area);
	textureUse(smaaSearch, smaaBlend->search);
	glUniform4f(smaaBlend->screenSize,
		1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
		currentSize.x, currentSize.y);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisable(GL_STENCIL_TEST);

	// SMAA neighbor blending pass
	programUse(smaaNeighbor);
	framebufferUse(rendererFramebuffer());
	textureUse(smaaSeparateFbColor[0], smaaNeighbor->image1);
	textureFilter(smaaSeparateFbColor[0], GL_LINEAR);
	textureUse(smaaSeparateFbColor[1], smaaNeighbor->image2);
	textureFilter(smaaSeparateFbColor[1], GL_LINEAR);
	textureUse(smaaBlendFbColor[0], smaaNeighbor->blend1);
	textureUse(smaaBlendFbColor[1], smaaNeighbor->blend2);
	glUniform4f(smaaNeighbor->screenSize,
		1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
		currentSize.x, currentSize.y);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
}
