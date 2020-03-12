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
#include "shader.h"
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

static Framebuffer msaaFb = 0;
static Texture msaaFbColor = 0;
static Renderbuffer msaaFbDepthStencil = 0;

static Framebuffer smaaSeparateFb = 0;
static Texture smaaSeparateFbColor[2] = {0};
static Framebuffer smaaEdgeFb[2] = {0};
static Renderbuffer smaaEdgeFbDepthStencil = 0;
static Texture smaaEdgeFbColor[2] = {0};
static Framebuffer smaaBlendFb = 0;
static Texture smaaBlendFbColor[2] = {0};
static Texture smaaArea = 0;
static Texture smaaSearch = 0;

static ProgramSmaaSeparate* smaaSeparate = null;
static ProgramSmaaEdge* smaaEdge = null;
static ProgramSmaaBlend* smaaBlend = null;
static ProgramSmaaNeighbor* smaaNeighbor = null;

static AAMode current = AANone;
static size2i currentSize = {0};

static void aaResize(size2i size)
{
	assert(size.x > 0);
	assert(size.y > 0);
	if (size.x == currentSize.x && size.y == currentSize.y) return;
	currentSize.x = size.x;
	currentSize.y = size.y;

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaFbColor);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, GL_RGBA16F,
		size.x, size.y, GL_TRUE);
	glBindRenderbuffer(GL_RENDERBUFFER, msaaFbDepthStencil);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 2, GL_DEPTH24_STENCIL8,
		size.x, size.y);

	glBindRenderbuffer(GL_RENDERBUFFER, smaaEdgeFbDepthStencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
		size.x, size.y);

	for (size_t i = 0; i < 2; i += 1) {
		glBindTexture(GL_TEXTURE_2D, smaaSeparateFbColor[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,
			size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);
		glBindTexture(GL_TEXTURE_2D, smaaEdgeFbColor[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);
		glBindTexture(GL_TEXTURE_2D, smaaBlendFbColor[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);
	}
}

void aaInit(AAMode type)
{
	glGenFramebuffers(1, &msaaFb);
	glGenTextures(1, &msaaFbColor);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaFbColor);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER,
		GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S,
		GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T,
		GL_CLAMP_TO_EDGE);
	glGenRenderbuffers(1, &msaaFbDepthStencil);

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
	glGenRenderbuffers(1, &smaaEdgeFbDepthStencil);
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

	// Set up framebuffer textures
	aaResize(windowGetSize());

	// Put framebuffers together
	glBindFramebuffer(GL_FRAMEBUFFER, msaaFb);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D_MULTISAMPLE, msaaFbColor, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
		GL_RENDERBUFFER, msaaFbDepthStencil);
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
			GL_RENDERBUFFER, smaaEdgeFbDepthStencil);
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
		GL_RENDERBUFFER, smaaEdgeFbDepthStencil);
	glDrawBuffers(2, (GLuint[]){GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
		!= GL_FRAMEBUFFER_COMPLETE) {
		logCrit(applog, u8"Failed to create the SMAA blend framebuffer");
		exit(EXIT_FAILURE);
	}

	rendererBindMainFb();

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
	smaaNeighbor->image1 = programSampler(smaaNeighbor, u8"image1", GL_TEXTURE0);
	smaaNeighbor->image2 = programSampler(smaaNeighbor, u8"image2", GL_TEXTURE1);
	smaaNeighbor->blend1 = programSampler(smaaNeighbor, u8"blend1", GL_TEXTURE2);
	smaaNeighbor->blend2 = programSampler(smaaNeighbor, u8"blend2", GL_TEXTURE3);
	smaaNeighbor->screenSize = programUniform(smaaNeighbor, u8"screenSize");

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
}

void aaCleanup(void)
{
	if (smaaSearch) {
		glDeleteTextures(1, &smaaSearch);
		smaaSearch = null;
	}
	if (smaaArea) {
		glDeleteTextures(1, &smaaArea);
		smaaArea = null;
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
	if (smaaBlendFbColor[0]) {
		glDeleteTextures(2, smaaBlendFbColor);
		arrayClear(smaaBlendFbColor);
	}
	if (smaaBlendFb) {
		glDeleteFramebuffers(1, &smaaBlendFb);
		smaaBlendFb = 0;
	}
	if (smaaEdgeFbDepthStencil) {
		glDeleteRenderbuffers(1, &smaaEdgeFbDepthStencil);
		smaaEdgeFbDepthStencil = 0;
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
	if (msaaFbDepthStencil) {
		glDeleteRenderbuffers(1, &msaaFbDepthStencil);
		msaaFbDepthStencil = 0;
	}
	if (msaaFbColor) {
		glDeleteTextures(1, &msaaFbColor);
		msaaFbColor = 0;
	}
	if (msaaFb) {
		glDeleteFramebuffers(1, &msaaFb);
		msaaFb = 0;
	}
}

void aaSwitch(AAMode type)
{
	;
}

void aaBegin(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, msaaFb);
}

void aaEnd(void)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// SMAA sample separation pass
	programUse(smaaSeparate);
	glBindFramebuffer(GL_FRAMEBUFFER, smaaSeparateFb);
	glActiveTexture(smaaSeparate->image);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaFbColor);
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
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y);
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
		1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
		currentSize.x, currentSize.y);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisable(GL_STENCIL_TEST);

	// SMAA neighbor blending pass
	programUse(smaaNeighbor);
	rendererBindMainFb();
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
		1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
		currentSize.x, currentSize.y);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
}
