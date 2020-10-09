/**
 * Implementation of aa.h
 * @file
 */

#include "aa.hpp"

#include "smaa/AreaTex.h"
#include "smaa/SearchTex.h"
#include "renderer.hpp"
#include "sys/window.hpp"
#include "base/util.hpp"
#include "base/log.hpp"

using namespace minote;

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

static const char* ProgramSmaaSeparateVertName = "smaaSeparate.vert";
static const GLchar* ProgramSmaaSeparateVertSrc = (GLchar[]){
#include "smaaSeparate.vert"
	'\0'};
static const char* ProgramSmaaSeparateFragName = "smaaSeparate.frag";
static const GLchar* ProgramSmaaSeparateFragSrc = (GLchar[]){
#include "smaaSeparate.frag"
	'\0'};

static const char* ProgramSmaaEdgeVertName = "smaaEdge.vert";
static const GLchar* ProgramSmaaEdgeVertSrc = (GLchar[]){
#include "smaaEdge.vert"
	'\0'};
static const char* ProgramSmaaEdgeFragName = "smaaEdge.frag";
static const GLchar* ProgramSmaaEdgeFragSrc = (GLchar[]){
#include "smaaEdge.frag"
	'\0'};

static const char* ProgramSmaaBlendVertName = "smaaBlend.vert";
static const GLchar* ProgramSmaaBlendVertSrc = (GLchar[]){
#include "smaaBlend.vert"
	'\0'};
static const char* ProgramSmaaBlendFragName = "smaaBlend.frag";
static const GLchar* ProgramSmaaBlendFragSrc = (GLchar[]){
#include "smaaBlend.frag"
	'\0'};

static const char* ProgramSmaaNeighborVertName = "smaaNeighbor.vert";
static const GLchar* ProgramSmaaNeighborVertSrc = (GLchar[]){
#include "smaaNeighbor.vert"
	'\0'};
static const char* ProgramSmaaNeighborFragName = "smaaNeighbor.frag";
static const GLchar* ProgramSmaaNeighborFragSrc = (GLchar[]){
#include "smaaNeighbor.frag"
	'\0'};

static Window* window = nullptr;

// AASimple, AAComplex, AAExtreme
static Framebuffer* msaaFb = nullptr;
static TextureMS msaaFbColor;
static RenderbufferMS* msaaFbDepthStencil = nullptr;

// AAFast, AAComplex
static Framebuffer* smaaEdgeFb = nullptr;
static Texture smaaEdgeFbColor;
static Renderbuffer* smaaEdgeFbDepthStencil = nullptr;
static Framebuffer* smaaBlendFb = nullptr;
static Texture smaaBlendFbColor;

static Texture smaaArea;
static Texture smaaSearch;

static ProgramSmaaEdge* smaaEdge = nullptr;
static ProgramSmaaBlend* smaaBlend = nullptr;
static ProgramSmaaNeighbor* smaaNeighbor = nullptr;

// AAComplex
static Framebuffer* smaaSeparateFb = nullptr;
static Texture smaaSeparateFbColor;
static Texture smaaSeparateFbColor2;
static Framebuffer* smaaEdgeFb2 = nullptr;
static Texture smaaEdgeFbColor2;
static Renderbuffer* smaaEdgeFbDepthStencil2 = nullptr;
static Framebuffer* smaaBlendFb2 = nullptr;
static Texture smaaBlendFbColor2;

static ProgramSmaaSeparate* smaaSeparate = nullptr;

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
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	if (size.x == currentSize.x && size.y == currentSize.y) return;
	currentSize.x = size.x;
	currentSize.y = size.y;

	size_t msaaSamples = (currentMode == AASimple) ? 4 :
	                     (currentMode == AAExtreme) ? 8 : 2;

	if (msaaFbColor.id)
		msaaFbColor.resize(size);
	if (msaaFbDepthStencil)
		renderbufferMSStorage(msaaFbDepthStencil, size, GL_DEPTH24_STENCIL8,
			msaaSamples);

	if (smaaSeparateFbColor.id)
		smaaSeparateFbColor.resize(size);
	if (smaaEdgeFbColor.id)
		smaaEdgeFbColor.resize(size);
	if (smaaEdgeFbDepthStencil)
		renderbufferStorage(smaaEdgeFbDepthStencil, size, GL_DEPTH24_STENCIL8);
	if (smaaBlendFbColor.id)
		smaaBlendFbColor.resize(size);

	if (smaaSeparateFbColor2.id)
		smaaSeparateFbColor2.resize(size);
	if (smaaEdgeFbColor2.id)
		smaaEdgeFbColor2.resize(size);
	if (smaaEdgeFbDepthStencil2)
		renderbufferStorage(smaaEdgeFbDepthStencil2, size, GL_DEPTH24_STENCIL8);
	if (smaaBlendFbColor2.id)
		smaaBlendFbColor2.resize(size);
}

void aaInit(AAMode mode, Window& w)
{
	if (initialized) return;
	currentMode = mode;
	window = &w;

	// Create needed objects
	size_t msaaSamples = (currentMode == AASimple) ? 4 :
	                     (currentMode == AAExtreme) ? 8 : 2;

	if (mode == AASimple || mode == AAComplex || mode == AAExtreme) {
		msaaFb = framebufferCreate();
		msaaFbColor.create(window->size, PixelFormat::RGBA_f16, msaaSamples);
		msaaFbDepthStencil = renderbufferMSCreate();
	}

	if (mode == AAFast || mode == AAComplex) {
		smaaBlendFb = framebufferCreate();
		smaaEdgeFb = framebufferCreate();
		smaaEdgeFbColor.create(window->size, PixelFormat::RGBA_u8);
		smaaBlendFbColor.create(window->size, PixelFormat::RGBA_u8);
		smaaEdgeFbDepthStencil = renderbufferCreate();
	}

	if (mode == AAComplex) {
		smaaSeparateFb = framebufferCreate();
		smaaSeparateFbColor.create(window->size, PixelFormat::RGBA_f16);
		smaaSeparateFbColor2.create(window->size, PixelFormat::RGBA_f16);
		smaaEdgeFb2 = framebufferCreate();
		smaaEdgeFbColor2.create(window->size, PixelFormat::RGBA_u8);
		smaaEdgeFbDepthStencil2 = renderbufferCreate();
		smaaBlendFb2 = framebufferCreate();
		smaaBlendFbColor2.create(window->size, PixelFormat::RGBA_u8);
	}

	// Set up framebuffer textures
	aaResize(window->size);

	// Put framebuffers together and create shaders
	if (mode == AASimple || mode == AAComplex || mode == AAExtreme) {
		framebufferTextureMS(msaaFb, msaaFbColor, GL_COLOR_ATTACHMENT0);
		framebufferRenderbufferMS(msaaFb, msaaFbDepthStencil,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(msaaFb)) {
			L.crit("Failed to create the render framebuffer");
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
				L.warn("MSAA 2x subsample locations are not as expected:");
				L.warn("    Subsample #0: (%f, %f), expected (0.75, 0.75)",
					sampleLocations[0], sampleLocations[1]);
				L.warn("    Subsample #1: (%f, %f), expected (0.25, 0.25)",
					sampleLocations[2], sampleLocations[3]);
#ifdef NDEBUG
				L.warn("  Graphics will look ugly.");
#else //NDEBUG
				L.crit("Aborting, please tell the developer that runtime subsample detection is needed");
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
			L.crit("Failed to create the SMAA edge framebuffer");
			exit(EXIT_FAILURE);
		}

		framebufferTexture(smaaBlendFb, smaaBlendFbColor, GL_COLOR_ATTACHMENT0);
		framebufferRenderbuffer(smaaBlendFb, smaaEdgeFbDepthStencil,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(smaaBlendFb)) {
			L.crit("Failed to create the SMAA blend framebuffer");
			exit(EXIT_FAILURE);
		}

		smaaEdge = programCreate(ProgramSmaaEdge,
			ProgramSmaaEdgeVertName, ProgramSmaaEdgeVertSrc,
			ProgramSmaaEdgeFragName, ProgramSmaaEdgeFragSrc);
		smaaEdge->image = programSampler(smaaEdge, "image", GL_TEXTURE0);
		smaaEdge->screenSize = programUniform(smaaEdge, "screenSize");

		smaaBlend = programCreate(ProgramSmaaBlend,
			ProgramSmaaBlendVertName, ProgramSmaaBlendVertSrc,
			ProgramSmaaBlendFragName, ProgramSmaaBlendFragSrc);
		smaaBlend->edges = programSampler(smaaBlend, "edges", GL_TEXTURE0);
		smaaBlend->area = programSampler(smaaBlend, "area", GL_TEXTURE1);
		smaaBlend->search = programSampler(smaaBlend, "search", GL_TEXTURE2);
		smaaBlend->subsampleIndices = programUniform(smaaBlend, "subsampleIndices");
		smaaBlend->screenSize = programUniform(smaaBlend, "screenSize");

		smaaNeighbor = programCreate(ProgramSmaaNeighbor,
			ProgramSmaaNeighborVertName, ProgramSmaaNeighborVertSrc,
			ProgramSmaaNeighborFragName, ProgramSmaaNeighborFragSrc);
		smaaNeighbor->image = programSampler(smaaNeighbor, "image",
			GL_TEXTURE0);
		smaaNeighbor->blend = programSampler(smaaNeighbor, "blend",
			GL_TEXTURE2);
		smaaNeighbor->alpha = programUniform(smaaNeighbor, "alpha");
		smaaNeighbor->screenSize = programUniform(smaaNeighbor, "screenSize");

		// Load lookup textures
		unsigned char areaTexBytesFlipped[AREATEX_SIZE] = {0};
		for (size_t i = 0; i < AREATEX_HEIGHT; i += 1)
			memcpy(areaTexBytesFlipped + i * AREATEX_PITCH,
				areaTexBytes + (AREATEX_HEIGHT - 1 - i) * AREATEX_PITCH,
				AREATEX_PITCH);

		smaaArea.create({AREATEX_WIDTH, AREATEX_HEIGHT}, PixelFormat::RG_u8);
		smaaArea.upload(areaTexBytesFlipped);

		unsigned char searchTexBytesFlipped[SEARCHTEX_SIZE] = {0};
		for (size_t i = 0; i < SEARCHTEX_HEIGHT; i += 1)
			memcpy(searchTexBytesFlipped + i * SEARCHTEX_PITCH,
				searchTexBytes + (SEARCHTEX_HEIGHT - 1 - i) * SEARCHTEX_PITCH,
				SEARCHTEX_PITCH);

		smaaSearch.create({SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT}, PixelFormat::R_u8);
		smaaSearch.setFilter(Filter::Nearest);
		smaaSearch.upload(searchTexBytesFlipped);
	}

	if (mode == AAComplex) {
		framebufferTexture(smaaSeparateFb, smaaSeparateFbColor,
			GL_COLOR_ATTACHMENT0);
		framebufferTexture(smaaSeparateFb, smaaSeparateFbColor2,
			GL_COLOR_ATTACHMENT1);
		framebufferBuffers(smaaSeparateFb, 2);
		if (!framebufferCheck(smaaSeparateFb)) {
			L.crit("Failed to create the SMAA separate framebuffer");
			exit(EXIT_FAILURE);
		}

		framebufferTexture(smaaEdgeFb2, smaaEdgeFbColor2, GL_COLOR_ATTACHMENT0);
		framebufferRenderbuffer(smaaEdgeFb2, smaaEdgeFbDepthStencil2,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(smaaEdgeFb2)) {
			L.crit("Failed to create the SMAA edge framebuffer");
			exit(EXIT_FAILURE);
		}

		framebufferTexture(smaaBlendFb2, smaaBlendFbColor2, GL_COLOR_ATTACHMENT0);
		framebufferRenderbuffer(smaaBlendFb2, smaaEdgeFbDepthStencil2,
			GL_DEPTH_STENCIL_ATTACHMENT);
		if (!framebufferCheck(smaaBlendFb2)) {
			L.crit("Failed to create the SMAA blend framebuffer");
			exit(EXIT_FAILURE);
		}

		smaaSeparate = programCreate(ProgramSmaaSeparate,
			ProgramSmaaSeparateVertName, ProgramSmaaSeparateVertSrc,
			ProgramSmaaSeparateFragName, ProgramSmaaSeparateFragSrc);
		smaaSeparate->image = programSampler(smaaSeparate, "image",
			GL_TEXTURE0);
	}

	framebufferUse(rendererFramebuffer());

	initialized = true;
	L.debug("Initialized AA mode %d", mode);
}

void aaCleanup(void)
{
	if (!initialized) return;
	framebufferDestroy(msaaFb);
	msaaFb = nullptr;
	msaaFbColor.destroy();
	renderbufferMSDestroy(msaaFbDepthStencil);
	msaaFbDepthStencil = nullptr;

	framebufferDestroy(smaaEdgeFb);
	smaaEdgeFb = nullptr;
	if (smaaEdgeFbColor.id)
		smaaEdgeFbColor.destroy();
	renderbufferDestroy(smaaEdgeFbDepthStencil);
	smaaEdgeFbDepthStencil = nullptr;
	framebufferDestroy(smaaBlendFb);
	smaaBlendFb = nullptr;
	if (smaaBlendFbColor.id)
		smaaBlendFbColor.destroy();

	if (smaaArea.id)
		smaaArea.destroy();
	if (smaaSearch.id)
		smaaSearch.destroy();
	
	programDestroy(smaaEdge);
	smaaEdge = nullptr;
	programDestroy(smaaBlend);
	smaaBlend = nullptr;
	programDestroy(smaaNeighbor);
	smaaNeighbor = nullptr;
	
	framebufferDestroy(smaaSeparateFb);
	smaaSeparateFb = nullptr;
	if (smaaSeparateFbColor.id)
		smaaSeparateFbColor.destroy();
	if (smaaSeparateFbColor2.id)
		smaaSeparateFbColor2.destroy();
	framebufferDestroy(smaaEdgeFb2);
	smaaEdgeFb2 = nullptr;
	if (smaaEdgeFbColor2.id)
		smaaEdgeFbColor2.destroy();
	renderbufferDestroy(smaaEdgeFbDepthStencil2);
	smaaEdgeFbDepthStencil2 = nullptr;
	framebufferDestroy(smaaBlendFb2);
	smaaBlendFb2 = nullptr;
	if (smaaBlendFbColor2.id)
		smaaBlendFbColor2.destroy();

	programDestroy(smaaSeparate);
	smaaSeparate = nullptr;

	currentSize.x = 0;
	currentSize.y = 0;

	initialized = false;
	L.debug("Cleaned up AA mode %d", currentMode);
}

void aaSwitch(AAMode mode)
{
	ASSERT(initialized);
	if (mode == currentMode) return;
	aaCleanup();
	aaInit(mode, *window);
}

void aaBegin(void)
{
	ASSERT(initialized);
	if (currentMode == AANone) return;
	aaResize(window->size);
	if (currentMode == AASimple
		|| currentMode == AAComplex
		|| currentMode == AAExtreme)
		framebufferUse(msaaFb);
}

void aaEnd(void)
{
	ASSERT(initialized);
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
		rendererTexture().bind(smaaEdge->image);
		rendererTexture().setFilter(Filter::Nearest); // It's ok, we turn it back
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
		smaaEdgeFbColor.bind(smaaBlend->edges);
		smaaArea.bind(smaaBlend->area);
		smaaSearch.bind(smaaBlend->search);
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
		rendererTexture().bind(smaaNeighbor->image);
		rendererTexture().setFilter(Filter::Linear); // It's ok, we turn it back
		smaaBlendFbColor.bind(smaaNeighbor->blend);
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
		msaaFbColor.bind(smaaSeparate->image);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// SMAA edge detection pass
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		programUse(smaaEdge);
		framebufferUse(smaaEdgeFb);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		smaaSeparateFbColor.bind(smaaEdge->image);
		smaaSeparateFbColor.setFilter(Filter::Nearest);
		glUniform4f(smaaEdge->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		framebufferUse(smaaEdgeFb2);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		smaaSeparateFbColor2.bind(smaaEdge->image);
		smaaSeparateFbColor2.setFilter(Filter::Nearest);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// SMAA blending weight calculation pass
		programUse(smaaBlend);
		framebufferUse(smaaBlendFb);
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilMask(0x00);
		glClear(GL_COLOR_BUFFER_BIT);
		smaaEdgeFbColor.bind(smaaBlend->edges);
		smaaArea.bind(smaaBlend->area);
		smaaSearch.bind(smaaBlend->search);
		glUniform4f(smaaBlend->subsampleIndices,
			1.0f, 2.0f, 2.0f, 0.0f);
		glUniform4f(smaaBlend->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		framebufferUse(smaaBlendFb2);
		glClear(GL_COLOR_BUFFER_BIT);
		smaaEdgeFbColor2.bind(smaaBlend->edges);
		smaaArea.bind(smaaBlend->area);
		smaaSearch.bind(smaaBlend->search);
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
		smaaSeparateFbColor.bind(smaaNeighbor->image);
		smaaSeparateFbColor.setFilter(Filter::Linear);
		smaaBlendFbColor.bind(smaaNeighbor->blend);
		glUniform1f(smaaNeighbor->alpha, 1.0f);
		glUniform4f(smaaNeighbor->screenSize,
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_BLEND);
		smaaSeparateFbColor2.bind(smaaNeighbor->image);
		smaaSeparateFbColor2.setFilter(Filter::Linear);
		smaaBlendFbColor2.bind(smaaNeighbor->blend);
		glUniform1f(smaaNeighbor->alpha, 0.5f);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}

	if (currentMode == AASimple || currentMode == AAExtreme) {
		framebufferBlit(msaaFb, rendererFramebuffer(), currentSize);
	}
}
