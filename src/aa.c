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

// AASimple, AAComplex, AAExtreme
static Framebuffer* msaaFb = null;
static TextureMS* msaaFbColor = null;
static RenderbufferMS* msaaFbDepthStencil = null;

// AAFast, AAComplex
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

// AAComplex
static Framebuffer* smaaSeparateFb = null;
static Texture* smaaSeparateFbColor = null;
static Texture* smaaSeparateFbColor2 = null;
static Framebuffer* smaaEdgeFb2 = null;
static Texture* smaaEdgeFbColor2 = null;
static Renderbuffer* smaaEdgeFbDepthStencil2 = null;
static Framebuffer* smaaBlendFb2 = null;
static Texture* smaaBlendFbColor2 = null;

static ProgramSmaaSeparate* smaaSeparate = null;

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

	size_t msaaSamples = (currentMode == AASimple) ? 4 :
	                     (currentMode == AAExtreme) ? 16 : 2;

	if (msaaFbColor)
		textureMSStorage(msaaFbColor, size, GL_RGBA16F, msaaSamples);
	if (msaaFbDepthStencil)
		renderbufferMSStorage(msaaFbDepthStencil, size, GL_DEPTH24_STENCIL8,
			msaaSamples);

	if (smaaSeparateFbColor)
		textureStorage(smaaSeparateFbColor, size, GL_RGBA16F);
	if (smaaEdgeFbColor)
		textureStorage(smaaEdgeFbColor, size, GL_RGBA8);
	if (smaaEdgeFbDepthStencil)
		renderbufferStorage(smaaEdgeFbDepthStencil, size, GL_DEPTH24_STENCIL8);
	if (smaaBlendFbColor)
		textureStorage(smaaBlendFbColor, size, GL_RGBA8);

	if (smaaSeparateFbColor2)
		textureStorage(smaaSeparateFbColor2, size, GL_RGBA16F);
	if (smaaEdgeFbColor2)
		textureStorage(smaaEdgeFbColor2, size, GL_RGBA8);
	if (smaaEdgeFbDepthStencil2)
		renderbufferStorage(smaaEdgeFbDepthStencil2, size, GL_DEPTH24_STENCIL8);
	if (smaaBlendFbColor2)
		textureStorage(smaaBlendFbColor2, size, GL_RGBA8);
}

void aaInit(AAMode mode)
{
	if (initialized) return;
	currentMode = mode;

	// Create needed objects
	if (mode == AASimple || mode == AAComplex || mode == AAExtreme) {
		msaaFb = framebufferCreate();
		msaaFbColor = textureMSCreate();
		msaaFbDepthStencil = renderbufferMSCreate();
	}

	if (mode == AAFast || mode == AAComplex) {
		smaaBlendFb = framebufferCreate();
		smaaEdgeFb = framebufferCreate();
		smaaEdgeFbColor = textureCreate();
		smaaBlendFbColor = textureCreate();
		smaaEdgeFbDepthStencil = renderbufferCreate();
	}

	if (mode == AAComplex) {
		smaaSeparateFb = framebufferCreate();
		smaaSeparateFbColor = textureCreate();
		smaaSeparateFbColor2 = textureCreate();
		smaaEdgeFb2 = framebufferCreate();
		smaaEdgeFbColor2 = textureCreate();
		smaaEdgeFbDepthStencil2 = renderbufferCreate();
		smaaBlendFb2 = framebufferCreate();
		smaaBlendFbColor2 = textureCreate();
	}

	// Set up framebuffer textures
	aaResize(windowGetSize());

	// Put framebuffers together and create shaders
	if (mode == AASimple || mode == AAComplex || mode == AAExtreme) {
		framebufferTextureMS(msaaFb, msaaFbColor, GL_COLOR_ATTACHMENT0);
		framebufferRenderbufferMS(msaaFb, msaaFbDepthStencil,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(msaaFb)) {
			logCrit(applog, u8"Failed to create the render framebuffer");
			exit(EXIT_FAILURE);
		}

		if (mode == AAComplex) {
			// Verify that multisampling has the expected subsample layout
			framebufferUse(msaaFb);
			GLfloat sampleLocations[4] = {0};
			glGetMultisamplefv(GL_SAMPLE_POSITION, 0, sampleLocations);
			glGetMultisamplefv(GL_SAMPLE_POSITION, 1, sampleLocations + 2);
			if (sampleLocations[0] != 0.75f ||
				sampleLocations[1] != 0.75f ||
				sampleLocations[2] != 0.25f ||
				sampleLocations[3] != 0.25f) {
				logWarn(applog,
					"MSAA 2x subsample locations are not as expected:");
				logWarn(applog,
					"    Subsample #0: (%f, %f), expected (0.75, 0.75)",
					sampleLocations[0], sampleLocations[1]);
				logWarn(applog,
					"    Subsample #1: (%f, %f), expected (0.25, 0.25)",
					sampleLocations[2], sampleLocations[3]);
#ifdef NDEBUG
				logWarn(applog, "  Graphics will look ugly.");
#else //NDEBUG
				logCrit(applog,
					"Aborting, please tell the developer that runtime subsample detection is needed");
				exit(EXIT_FAILURE);
#endif //NDEBUG
			}
		}
	}

	if (mode == AAFast || mode == AAComplex) {
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

	if (mode == AAComplex) {
		framebufferTexture(smaaSeparateFb, smaaSeparateFbColor,
			GL_COLOR_ATTACHMENT0);
		framebufferTexture(smaaSeparateFb, smaaSeparateFbColor2,
			GL_COLOR_ATTACHMENT1);
		framebufferBuffers(smaaSeparateFb, 2);
		if (!framebufferCheck(smaaSeparateFb)) {
			logCrit(applog, u8"Failed to create the SMAA separate framebuffer");
			exit(EXIT_FAILURE);
		}

		framebufferTexture(smaaEdgeFb2, smaaEdgeFbColor2, GL_COLOR_ATTACHMENT0);
		framebufferRenderbuffer(smaaEdgeFb2, smaaEdgeFbDepthStencil2,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(smaaEdgeFb2)) {
			logCrit(applog, u8"Failed to create the SMAA edge framebuffer");
			exit(EXIT_FAILURE);
		}

		framebufferTexture(smaaBlendFb2, smaaBlendFbColor2, GL_COLOR_ATTACHMENT0);
		framebufferRenderbuffer(smaaBlendFb2, smaaEdgeFbDepthStencil2,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(smaaBlendFb2)) {
			logCrit(applog, u8"Failed to create the SMAA blend framebuffer");
			exit(EXIT_FAILURE);
		}

		smaaSeparate = programCreate(ProgramSmaaSeparate,
			ProgramSmaaSeparateVertName, ProgramSmaaSeparateVertSrc,
			ProgramSmaaSeparateFragName, ProgramSmaaSeparateFragSrc);
		smaaSeparate->image = programSampler(smaaSeparate, u8"image",
			GL_TEXTURE0);
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
	
	framebufferDestroy(smaaSeparateFb);
	smaaSeparateFb = null;
	textureDestroy(smaaSeparateFbColor);
	smaaSeparateFbColor = null;
	textureDestroy(smaaSeparateFbColor2);
	smaaSeparateFbColor2 = null;
	framebufferDestroy(smaaEdgeFb2);
	smaaEdgeFb2 = null;
	textureDestroy(smaaEdgeFbColor2);
	smaaEdgeFbColor2 = null;
	renderbufferDestroy(smaaEdgeFbDepthStencil2);
	smaaEdgeFbDepthStencil2 = null;
	framebufferDestroy(smaaBlendFb2);
	smaaBlendFb2 = null;
	textureDestroy(smaaBlendFbColor2);
	smaaBlendFbColor2 = null;

	programDestroy(smaaSeparate);
	smaaSeparate = null;

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
	if (currentMode == AANone) return;
	aaResize(windowGetSize());
	if (currentMode == AASimple
		|| currentMode == AAComplex
		|| currentMode == AAExtreme)
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

	if (currentMode == AAComplex) {
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
		framebufferUse(smaaEdgeFb);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		textureUse(smaaSeparateFbColor, smaaEdge->image);
		textureFilter(smaaSeparateFbColor, GL_NEAREST);
		glUniform4f(smaaEdge->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		framebufferUse(smaaEdgeFb2);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		textureUse(smaaSeparateFbColor2, smaaEdge->image);
		textureFilter(smaaSeparateFbColor2, GL_NEAREST);
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
			1.0f, 2.0f, 2.0f, 0.0f);
		glUniform4f(smaaBlend->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		framebufferUse(smaaBlendFb2);
		glClear(GL_COLOR_BUFFER_BIT);
		textureUse(smaaEdgeFbColor2, smaaBlend->edges);
		textureUse(smaaArea, smaaBlend->area);
		textureUse(smaaSearch, smaaBlend->search);
		glUniform4f(smaaBlend->subsampleIndices,
			2.0f, 1.0f, 1.0f, 0.0f);
		glUniform4f(smaaBlend->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisable(GL_STENCIL_TEST);

		// SMAA neighbor blending pass
		programUse(smaaNeighbor);
		framebufferUse(rendererFramebuffer());
		textureUse(smaaSeparateFbColor, smaaNeighbor->image);
		textureFilter(smaaSeparateFbColor, GL_LINEAR);
		textureUse(smaaBlendFbColor, smaaNeighbor->blend);
		glUniform1f(smaaNeighbor->alpha, 1.0f);
		glUniform4f(smaaNeighbor->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_BLEND);
		textureUse(smaaSeparateFbColor2, smaaNeighbor->image);
		textureFilter(smaaSeparateFbColor2, GL_LINEAR);
		textureUse(smaaBlendFbColor2, smaaNeighbor->blend);
		glUniform1f(smaaNeighbor->alpha, 0.5f);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}

	if (currentMode == AASimple || currentMode == AAExtreme) {
		framebufferBlit(msaaFb, rendererFramebuffer(), currentSize);
	}
}
