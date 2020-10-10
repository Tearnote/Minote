/**
 * Implementation of bloom.h
 * @file
 */

#include "bloom.hpp"

#include "glad/glad.h"
#include "renderer.hpp"
#include "sys/window.hpp"
#include "base/util.hpp"
#include "base/log.hpp"

using namespace minote;

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

static const char* ProgramThresholdVertName = "threshold.vert";
static const GLchar* ProgramThresholdVertSrc = (GLchar[]){
#include "threshold.vert"
	'\0'};
static const char* ProgramThresholdFragName = "threshold.frag";
static const GLchar* ProgramThresholdFragSrc = (GLchar[]){
#include "threshold.frag"
	'\0'};

static const char* ProgramBoxBlurVertName = "boxBlur.vert";
static const GLchar* ProgramBoxBlurVertSrc = (GLchar[]){
#include "boxBlur.vert"
	'\0'};
static const char* ProgramBoxBlurFragName = "boxBlur.frag";
static const GLchar* ProgramBoxBlurFragSrc = (GLchar[]){
#include "boxBlur.frag"
	'\0'};

static Window* window = nullptr;

static Framebuffer* bloomFb[BloomPasses] = {nullptr};
static Texture bloomFbColor[BloomPasses];
static ProgramThreshold* threshold = nullptr;
static ProgramBoxBlur* boxBlur = nullptr;

static size2i currentSize = {0};
static bool initialized = false;

/**
 * Ensure that AA framebuffers are the correct size in relation to the screen.
 * This can be run every frame with the current size of the screen.
 * @param size Current screen size
 */
static void bloomResize(size2i size)
{
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	if (size.x == currentSize.x && size.y == currentSize.y) return;
	currentSize.x = size.x;
	currentSize.y = size.y;

	for (size_t i = 0; i < BloomPasses; i += 1) {
		size2i layerSize = {
			size.x >> (i + 1),
			size.y >> (i + 1)
		};
		bloomFbColor[i].resize(layerSize);
	}
}

void bloomInit(Window& w)
{
	if (initialized) return;
	window = &w;
	size2i windowSize = window->size;

	for (size_t i = 0; i < BloomPasses; i += 1) {
		size2i layerSize = {
			windowSize.x >> (i + 1),
			windowSize.y >> (i + 1)
		};
		bloomFb[i] = framebufferCreate();
		bloomFbColor[i].create("bloomFbColor", layerSize, PixelFormat::RGBA_f16);
	}

	bloomResize(window->size);

	for (size_t i = 0; i < BloomPasses; i += 1) {
		framebufferTexture(bloomFb[i], bloomFbColor[i], GL_COLOR_ATTACHMENT0);
		if (!framebufferCheck(bloomFb[i])) {
			L.crit("Failed to create the bloom framebuffer #%zu", i);
			exit(EXIT_FAILURE);
		}
	}

	threshold = programCreate(ProgramThreshold,
		ProgramThresholdVertName, ProgramThresholdVertSrc,
		ProgramThresholdFragName, ProgramThresholdFragSrc);
	threshold->image = programSampler(threshold, "image", GL_TEXTURE0);
	threshold->threshold = programUniform(threshold, "threshold");
	threshold->softKnee = programUniform(threshold, "softKnee");
	threshold->strength = programUniform(threshold, "strength");

	boxBlur = programCreate(ProgramBoxBlur,
		ProgramBoxBlurVertName, ProgramBoxBlurVertSrc,
		ProgramBoxBlurFragName, ProgramBoxBlurFragSrc);
	boxBlur->image = programSampler(boxBlur, "image", GL_TEXTURE0);
	boxBlur->step = programUniform(boxBlur, "step");
	boxBlur->imageTexel = programUniform(boxBlur, "imageTexel");

	initialized = true;
}

void bloomCleanup(void)
{
	if (!initialized) return;

	programDestroy(boxBlur);
	boxBlur = nullptr;
	programDestroy(threshold);
	threshold = nullptr;
	for (size_t i = BloomPasses - 1; i < BloomPasses; i -= 1) {
		bloomFbColor[i].destroy();
		framebufferDestroy(bloomFb[i]);
		bloomFb[i] = nullptr;
	}

	initialized = false;
}

void bloomApply(void)
{
	ASSERT(initialized);
	bloomResize(window->size);

	// Prepare the image for bloom
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	framebufferUse(bloomFb[0]);
	glViewport(0, 0, currentSize.x >> 1, currentSize.y >> 1);
	programUse(threshold);
	rendererTexture().bind(threshold->image);
	glUniform1f(threshold->threshold, 1.0f);
	glUniform1f(threshold->softKnee, 0.25f);
	glUniform1f(threshold->strength, 1.0f);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Blur the bloom image
	programUse(boxBlur);
	for (size_t i = 0; i < BloomPasses - 1; i += 1) {
		framebufferUse(bloomFb[i + 1]);
		glViewport(0, 0, currentSize.x >> (i + 2), currentSize.y >> (i + 2));
		bloomFbColor[i].bind(boxBlur->image);
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
		bloomFbColor[i + 1].bind(boxBlur->image);
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
