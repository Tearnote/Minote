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

typedef struct ProgramSmaaEdge {
	ProgramBase base;
	TextureUnit image;
	Uniform screenSize;
} ProgramSmaaEdge;

typedef struct ProgramSmaaBlend {
	ProgramBase base;
	TextureUnit edges;
	TextureUnit area;
	TextureUnit search;
	Uniform subsampleIndices;
	Uniform screenSize;
} ProgramSmaaBlend;

typedef struct ProgramSmaaNeighbor {
	ProgramBase base;
	TextureUnit image;
	TextureUnit blend;
	Uniform alpha;
	Uniform screenSize;
} ProgramSmaaNeighbor;

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

// AAMulti
static Framebuffer* msaaFb = null;
static TextureMS* msaaFbColor = null;
static RenderbufferMS* msaaFbDepthStencil = null;

// AAFast
static Framebuffer* smaaEdgeFb = null;
static Texture* smaaEdgeFbColor = null;
static Renderbuffer* smaaEdgeFbDepthStencil = null;
static Framebuffer* smaaBlendFb = null;
static Texture* smaaBlendFbColor = null;

static Texture* smaaArea = null;
static Texture* smaaSearch = null;

static ProgramSmaaEdge* smaaEdge = null;
static ProgramSmaaBlend* smaaBlend = null;
static ProgramSmaaNeighbor* smaaNeighbor = null;

static AAMode currentMode = AANone;
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

	size_t msaaSamples = (currentMode == AAMulti) ? 4 : 2;

	if (msaaFbColor)
		textureMSStorage(msaaFbColor, size, GL_RGBA16F, msaaSamples);
	if (msaaFbDepthStencil)
		renderbufferMSStorage(msaaFbDepthStencil, size, GL_DEPTH24_STENCIL8,
			msaaSamples);

	if (smaaEdgeFbColor)
		textureStorage(smaaEdgeFbColor, size, GL_RGBA8);
	if (smaaEdgeFbDepthStencil)
		renderbufferStorage(smaaEdgeFbDepthStencil, size, GL_DEPTH24_STENCIL8);
	if (smaaBlendFbColor)
		textureStorage(smaaBlendFbColor, size, GL_RGBA8);
}

void aaInit(AAMode mode)
{
	if (initialized) return;
	currentMode = mode;

	// Create needed objects
	if (mode == AAMulti) {
		msaaFb = framebufferCreate();
		msaaFbColor = textureMSCreate();
		msaaFbDepthStencil = renderbufferMSCreate();
	}

	if (mode == AAFast) {
		smaaBlendFb = framebufferCreate();
		smaaEdgeFb = framebufferCreate();
		smaaEdgeFbColor = textureCreate();
		smaaBlendFbColor = textureCreate();
		smaaEdgeFbDepthStencil = renderbufferCreate();
	}

	// Set up framebuffer textures
	aaResize(windowGetSize());

	// Put framebuffers together and create shaders
	if (mode == AAMulti) {
		framebufferTextureMS(msaaFb, msaaFbColor, GL_COLOR_ATTACHMENT0);
		framebufferRenderbufferMS(msaaFb, msaaFbDepthStencil,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(msaaFb)) {
			logCrit(applog, u8"Failed to create the render framebuffer");
			exit(EXIT_FAILURE);
		}
	}

	if (mode == AAFast) {
		framebufferTexture(smaaEdgeFb, smaaEdgeFbColor, GL_COLOR_ATTACHMENT0);
		framebufferRenderbuffer(smaaEdgeFb, smaaEdgeFbDepthStencil,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(smaaEdgeFb)) {
			logCrit(applog, u8"Failed to create the SMAA edge framebuffer");
			exit(EXIT_FAILURE);
		}

		framebufferTexture(smaaBlendFb, smaaBlendFbColor, GL_COLOR_ATTACHMENT0);
		framebufferRenderbuffer(smaaBlendFb, smaaEdgeFbDepthStencil,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(smaaBlendFb)) {
			logCrit(applog, u8"Failed to create the SMAA blend framebuffer");
			exit(EXIT_FAILURE);
		}

		smaaEdge = programCreate(ProgramSmaaEdge,
			ProgramSmaaEdgeVertName, ProgramSmaaEdgeVertSrc,
			ProgramSmaaEdgeFragName, ProgramSmaaEdgeFragSrc);
		smaaEdge->image = programSampler(smaaEdge, u8"image", GL_TEXTURE0);
		smaaEdge->screenSize = programUniform(smaaEdge, u8"screenSize");

		smaaBlend = programCreate(ProgramSmaaBlend,
			ProgramSmaaBlendVertName, ProgramSmaaBlendVertSrc,
			ProgramSmaaBlendFragName, ProgramSmaaBlendFragSrc);
		smaaBlend->edges = programSampler(smaaBlend, u8"edges", GL_TEXTURE0);
		smaaBlend->area = programSampler(smaaBlend, u8"area", GL_TEXTURE1);
		smaaBlend->search = programSampler(smaaBlend, u8"search", GL_TEXTURE2);
		smaaBlend->subsampleIndices = programUniform(smaaBlend, u8"subsampleIndices");
		smaaBlend->screenSize = programUniform(smaaBlend, u8"screenSize");

		smaaNeighbor = programCreate(ProgramSmaaNeighbor,
			ProgramSmaaNeighborVertName, ProgramSmaaNeighborVertSrc,
			ProgramSmaaNeighborFragName, ProgramSmaaNeighborFragSrc);
		smaaNeighbor->image = programSampler(smaaNeighbor, u8"image",
			GL_TEXTURE0);
		smaaNeighbor->blend = programSampler(smaaNeighbor, u8"blend",
			GL_TEXTURE2);
		smaaNeighbor->alpha = programUniform(smaaNeighbor, u8"alpha");
		smaaNeighbor->screenSize = programUniform(smaaNeighbor, u8"screenSize");

		// Load lookup textures
		unsigned char areaTexBytesFlipped[AREATEX_SIZE] = {0};
		for (size_t i = 0; i < AREATEX_HEIGHT; i += 1)
			memcpy(areaTexBytesFlipped + i * AREATEX_PITCH,
				areaTexBytes + (AREATEX_HEIGHT - 1 - i) * AREATEX_PITCH,
				AREATEX_PITCH);

		smaaArea = textureCreate();
		textureStorage(smaaArea, (size2i){AREATEX_WIDTH, AREATEX_HEIGHT},
			GL_RG8);
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
		textureData(smaaSearch, searchTexBytesFlipped, GL_RED,
			GL_UNSIGNED_BYTE);
	}

	framebufferUse(rendererFramebuffer());

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

	framebufferDestroy(smaaEdgeFb);
	smaaEdgeFb = null;
	textureDestroy(smaaEdgeFbColor);
	smaaEdgeFbColor = null;
	renderbufferDestroy(smaaEdgeFbDepthStencil);
	smaaEdgeFbDepthStencil = null;
	framebufferDestroy(smaaBlendFb);
	smaaBlendFb = null;
	textureDestroy(smaaBlendFbColor);
	smaaBlendFbColor = null;
	
	textureDestroy(smaaArea);
	smaaArea = null;
	textureDestroy(smaaSearch);
	smaaSearch = null;
	
	programDestroy(smaaEdge);
	smaaEdge = null;
	programDestroy(smaaBlend);
	smaaBlend = null;
	programDestroy(smaaNeighbor);
	smaaNeighbor = null;

	currentSize.x = 0;
	currentSize.y = 0;

	initialized = false;
}

void aaSwitch(AAMode mode)
{
	assert(initialized);
	if (mode == currentMode) return;
	aaCleanup();
	aaInit(mode);
}

void aaBegin(void)
{
	assert(initialized);
	if (currentMode == AANone || currentMode == AADist) return;
	aaResize(windowGetSize());
	if (currentMode == AAMulti)
		framebufferUse(msaaFb);
}

void aaEnd(void)
{
	assert(initialized);
	if (currentMode == AANone) return;

	if (currentMode == AAFast) {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		// SMAA edge detection pass
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		programUse(smaaEdge);
		framebufferUse(smaaEdgeFb);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		textureUse(rendererTexture(), smaaEdge->image);
		textureFilter(rendererTexture(), GL_NEAREST); // It's ok, we turn it back
		glUniform4f(smaaEdge->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// SMAA blending weight calculation pass
		programUse(smaaBlend);
		framebufferUse(smaaBlendFb);
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilMask(0x00);
		glClear(GL_COLOR_BUFFER_BIT);
		textureUse(smaaEdgeFbColor, smaaBlend->edges);
		textureUse(smaaArea, smaaBlend->area);
		textureUse(smaaSearch, smaaBlend->search);
		glUniform4f(smaaBlend->subsampleIndices,
			0.0f, 0.0f, 0.0f, 0.0f);
		glUniform4f(smaaBlend->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisable(GL_STENCIL_TEST);

		// SMAA neighbor blending pass
		programUse(smaaNeighbor);
		framebufferUse(rendererFramebuffer());
		textureUse(rendererTexture(), smaaNeighbor->image);
		textureFilter(rendererTexture(), GL_LINEAR);
		textureUse(smaaBlendFbColor, smaaNeighbor->blend);
		glUniform1f(smaaNeighbor->alpha, 1.0f);
		glUniform4f(smaaNeighbor->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}

	if (currentMode == AAMulti) {
		framebufferBlit(msaaFb, rendererFramebuffer(), currentSize);
	}
}
