/**
 * Implementation of bloom.h
 * @file
 */

#include "bloom.h"

#include <assert.h>
#include "glad/glad.h"
#include "renderer.h"
#include "opengl.h"
#include "window.h"
#include "util.h"
#include "log.h"

#define BloomPasses 6

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

static Framebuffer* bloomFb[BloomPasses] = {null};
static Texture* bloomFbColor[BloomPasses] = {null};
static ProgramThreshold* threshold = null;
static ProgramBoxBlur* boxBlur = null;

static size2i currentSize = {0};
static bool initialized = false;

/**
 * Ensure that AA framebuffers are the correct size in relation to the screen.
 * This can be run every frame with the current size of the screen.
 * @param size Current screen size
 */
static void bloomResize(size2i size)
{
	assert(size.x > 0);
	assert(size.y > 0);
	if (size.x == currentSize.x && size.y == currentSize.y) return;
	currentSize.x = size.x;
	currentSize.y = size.y;

	for (size_t i = 0; i < BloomPasses; i += 1) {
		size2i layerSize = {
			size.x >> (i + 1),
			size.y >> (i + 1)
		};
		textureStorage(bloomFbColor[i], layerSize, GL_RGBA16F);
	}
}

void bloomInit(void)
{
	if (initialized) return;

	for (size_t i = 0; i < BloomPasses; i += 1) {
		bloomFb[i] = framebufferCreate();
		bloomFbColor[i] = textureCreate();
	}

	bloomResize(windowGetSize());

	for (size_t i = 0; i < BloomPasses; i += 1) {
		framebufferTexture(bloomFb[i], bloomFbColor[i], GL_COLOR_ATTACHMENT0);
		if (!framebufferCheck(bloomFb[i])) {
			logCrit(applog, u8"Failed to create the bloom framebuffer #%zu", i);
			exit(EXIT_FAILURE);
		}
	}

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

	initialized = true;
}

void bloomCleanup(void)
{
	if (!initialized) return;

	programDestroy(boxBlur);
	boxBlur = null;
	programDestroy(threshold);
	threshold = null;
	for (size_t i = BloomPasses - 1; i < BloomPasses; i -= 1) {
		textureDestroy(bloomFbColor[i]);
		bloomFbColor[i] = null;
		framebufferDestroy(bloomFb[i]);
		bloomFb[i] = null;
	}

	initialized = false;
}

void bloomApply(void)
{
	assert(initialized);
	bloomResize(windowGetSize());

	// Prepare the image for bloom
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	framebufferUse(bloomFb[0]);
	glViewport(0, 0, currentSize.x >> 1, currentSize.y >> 1);
	programUse(threshold);
	textureUse(rendererTexture(), threshold->image);
	glUniform1f(threshold->threshold, 1.0f);
	glUniform1f(threshold->softKnee, 0.25f);
	glUniform1f(threshold->strength, 1.0f);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Blur the bloom image
	programUse(boxBlur);
	for (size_t i = 0; i < BloomPasses - 1; i += 1) {
		framebufferUse(bloomFb[i + 1]);
		glViewport(0, 0, currentSize.x >> (i + 2), currentSize.y >> (i + 2));
		textureUse(bloomFbColor[i], boxBlur->image);
		glUniform1f(boxBlur->step, 1.0f);
		glUniform2f(boxBlur->imageTexel,
			1.0 / (float)(currentSize.x >> (i + 1)),
			1.0 / (float)(currentSize.y >> (i + 1)));
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	for (size_t i = BloomPasses - 2; i < BloomPasses; i -= 1) {
		framebufferUse(bloomFb[i]);
		glViewport(0, 0, currentSize.x >> (i + 1), currentSize.y >> (i + 1));
		textureUse(bloomFbColor[i + 1], boxBlur->image);
		glUniform1f(boxBlur->step, 0.5f);
		glUniform2f(boxBlur->imageTexel,
			1.0 / (float)(currentSize.x >> (i + 2)),
			1.0 / (float)(currentSize.y >> (i + 2)));
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// Draw the bloom on top of the render
	framebufferUse(rendererFramebuffer());
	glViewport(0, 0, currentSize.x, currentSize.y);
	rendererBlit(bloomFbColor[0], 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
}
