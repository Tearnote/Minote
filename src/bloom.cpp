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
struct Threshold : Shader {

	Sampler<Texture> image;
	Uniform<float> threshold;
	Uniform<float> softKnee;
	Uniform<float> strength;

	void setLocations() override
	{
		image.setLocation(*this, "image");
		threshold.setLocation(*this, "threshold");
		softKnee.setLocation(*this, "softKnee");
		strength.setLocation(*this, "strength");
	}

};

struct BoxBlur : Shader {

	Sampler<Texture> image;
	Uniform<float> step;
	Uniform<vec2> imageTexel;

	void setLocations() override
	{
		image.setLocation(*this, "image");
		step.setLocation(*this, "step");
		imageTexel.setLocation(*this, "imageTexel");
	}

};

static const GLchar* ThresholdVertSrc = (GLchar[]){
#include "threshold.vert"
	'\0'};
static const GLchar* ThresholdFragSrc = (GLchar[]){
#include "threshold.frag"
	'\0'};

static const GLchar* BoxBlurVertSrc = (GLchar[]){
#include "boxBlur.vert"
	'\0'};
static const GLchar* BoxBlurFragSrc = (GLchar[]){
#include "boxBlur.frag"
	'\0'};

static Window* window = nullptr;

static Framebuffer bloomFb[BloomPasses];
static Texture<PixelFmt::RGBA_f16> bloomFbColor[BloomPasses];
static Threshold threshold;
static BoxBlur boxBlur;

static uvec2 currentSize {0};
static bool initialized = false;

/**
 * Ensure that AA framebuffers are the correct size in relation to the screen.
 * This can be run every frame with the current size of the screen.
 * @param size Current screen size
 */
static void bloomResize(uvec2 size)
{
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	if (size.x == currentSize.x && size.y == currentSize.y) return;
	currentSize.x = size.x;
	currentSize.y = size.y;

	for (size_t i = 0; i < BloomPasses; i += 1) {
		uvec2 layerSize = {
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
	uvec2 windowSize = window->size;

	for (size_t i = 0; i < BloomPasses; i += 1) {
		uvec2 layerSize = {
			windowSize.x >> (i + 1),
			windowSize.y >> (i + 1)
		};
		bloomFb[i].create("bloomFb");
		bloomFbColor[i].create("bloomFbColor", layerSize);
	}

	bloomResize(window->size);

	for (size_t i = 0; i < BloomPasses; i += 1)
		bloomFb[i].attach(bloomFbColor[i], Attachment::Color0);

	threshold.create("threshold", ThresholdVertSrc, ThresholdFragSrc);
	boxBlur.create("boxBlur", BoxBlurVertSrc, BoxBlurFragSrc);

	initialized = true;
}

void bloomCleanup(void)
{
	if (!initialized) return;

	boxBlur.destroy();
	threshold.destroy();
	for (size_t i = BloomPasses - 1; i < BloomPasses; i -= 1) {
		bloomFbColor[i].destroy();
		bloomFb[i].destroy();
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
	bloomFb[0].bind();
	glViewport(0, 0, currentSize.x >> 1, currentSize.y >> 1);
	threshold.bind();
	threshold.image = renderFbColor;
	threshold.threshold = 1.0f;
	threshold.softKnee = 0.25f;
	threshold.strength = 1.0f;
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Blur the bloom image
	boxBlur.bind();
	for (size_t i = 0; i < BloomPasses - 1; i += 1) {
		bloomFb[i + 1].bind();
		glViewport(0, 0, currentSize.x >> (i + 2), currentSize.y >> (i + 2));
		boxBlur.image = bloomFbColor[i];
		boxBlur.step = 1.0f;
		boxBlur.imageTexel = {
			1.0 / (float)(currentSize.x >> (i + 1)),
			1.0 / (float)(currentSize.y >> (i + 1))
		};
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	for (size_t i = BloomPasses - 2; i < BloomPasses; i -= 1) {
		bloomFb[i].bind();
		glViewport(0, 0, currentSize.x >> (i + 1), currentSize.y >> (i + 1));
		boxBlur.image = bloomFbColor[i + 1];
		boxBlur.step = 0.5f;
		boxBlur.imageTexel = {
			1.0 / (float)(currentSize.x >> (i + 2)),
			1.0 / (float)(currentSize.y >> (i + 2))
		};
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// Draw the bloom on top of the render
	renderFb.bind();
	glViewport(0, 0, currentSize.x, currentSize.y);
	rendererBlit(bloomFbColor[0], 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
}
