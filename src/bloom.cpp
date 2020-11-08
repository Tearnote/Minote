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

struct ThresholdShader : Shader {

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

struct BoxBlurShader : Shader {

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
static ThresholdShader thresholdShader;
static BoxBlurShader boxBlurShader;

static Draw<ThresholdShader> threshold {
	.shader = &thresholdShader,
	.framebuffer = &bloomFb[0],
	.triangles = 1,
	.params = {
		.culling = false,
		.depthTesting = false
	}
};

static Draw<BoxBlurShader> boxBlur {
	.shader = &boxBlurShader,
	.triangles = 1,
	.params = {
		.blendingMode = {BlendingOp::One, BlendingOp::One},
		.culling = false,
		.depthTesting = false
	}
};

static Draw<BlitShader> blit {
	.shader = &blitShader,
	.framebuffer = &renderFb,
	.triangles = 1,
	.params = {
		.blending = true,
		.blendingMode = {BlendingOp::One, BlendingOp::One},
		.culling = false,
		.depthTesting = false
	}
};

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

	thresholdShader.create("threshold", ThresholdVertSrc, ThresholdFragSrc);
	boxBlurShader.create("boxBlur", BoxBlurVertSrc, BoxBlurFragSrc);

	initialized = true;
}

void bloomCleanup(void)
{
	if (!initialized) return;

	boxBlurShader.destroy();
	thresholdShader.destroy();
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
	threshold.params.viewport = {.size = {currentSize.x >> 1, currentSize.y >> 1}};
	threshold.shader->image = renderFbColor;
	threshold.shader->threshold = 1.0f;
	threshold.shader->softKnee = 0.25f;
	threshold.shader->strength = 1.0f;
	threshold.draw(*window);

	// Blur the bloom image
	for (size_t i = 0; i < BloomPasses - 1; i += 1) {
		boxBlur.framebuffer = &bloomFb[i + 1];
		boxBlur.params.viewport = {.size = {
			currentSize.x >> (i + 2),
			currentSize.y >> (i + 2)
		}};
		boxBlur.params.blending = false;
		boxBlur.shader->image = bloomFbColor[i];
		boxBlur.shader->step = 1.0f;
		boxBlur.shader->imageTexel = {
			1.0 / (float)(currentSize.x >> (i + 1)),
			1.0 / (float)(currentSize.y >> (i + 1))
		};
		boxBlur.draw(*window);
	}
	for (size_t i = BloomPasses - 2; i < BloomPasses; i -= 1) {
		boxBlur.framebuffer = &bloomFb[i];
		boxBlur.params.viewport = {.size = {
			currentSize.x >> (i + 1),
			currentSize.y >> (i + 1)
		}};
		boxBlur.params.blending = true;
		boxBlur.shader->image = bloomFbColor[i + 1];
		boxBlur.shader->step = 0.5f;
		boxBlur.shader->imageTexel = {
			1.0 / (float)(currentSize.x >> (i + 2)),
			1.0 / (float)(currentSize.y >> (i + 2))
		};
		boxBlur.draw(*window);
	}

	// Draw the bloom on top of the render
	blit.shader->image = bloomFbColor[0];
	blit.shader->boost = 1.0f;
	blit.draw(*window);
}
