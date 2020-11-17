/**
 * Implementation of bloom.h
 * @file
 */

#include "bloom.hpp"

#include "glad/glad.h"
#include "sys/window.hpp"
#include "sys/opengl.hpp"
#include "base/util.hpp"
#include "base/log.hpp"
#include "store/shaders.hpp"

using namespace minote;

#define BloomPasses 6

static Framebuffer bloomFb[BloomPasses];
static Texture<PixelFmt::RGBA_f16> bloomFbColor[BloomPasses];

static Draw<Shaders::Threshold> threshold {
	.framebuffer = &bloomFb[0],
	.triangles = 1,
	.params = {
		.culling = false,
		.depthTesting = false
	}
};

static Draw<Shaders::BoxBlur> boxBlur {
	.triangles = 1,
	.params = {
		.blendingMode = {BlendingOp::One, BlendingOp::One},
		.culling = false,
		.depthTesting = false
	}
};

static Draw<Shaders::Blit> blit {
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

void bloomInit(Window& window)
{
	if (initialized) return;
	uvec2 windowSize = window.size;

	for (size_t i = 0; i < BloomPasses; i += 1) {
		uvec2 layerSize = {
			windowSize.x >> (i + 1),
			windowSize.y >> (i + 1)
		};
		bloomFb[i].create("bloomFb");
		bloomFbColor[i].create("bloomFbColor", layerSize);
	}

	bloomResize(window.size);

	for (size_t i = 0; i < BloomPasses; i += 1)
		bloomFb[i].attach(bloomFbColor[i], Attachment::Color0);

	initialized = true;
}

void bloomCleanup(void)
{
	if (!initialized) return;

	for (size_t i = BloomPasses - 1; i < BloomPasses; i -= 1) {
		bloomFbColor[i].destroy();
		bloomFb[i].destroy();
	}

	initialized = false;
}

void bloomApply(Engine& engine)
{
	ASSERT(initialized);
	bloomResize(engine.window.size);

	threshold.shader = &engine.shaders.threshold;
	boxBlur.shader = &engine.shaders.boxBlur;
	blit.shader = &engine.shaders.blit;
	blit.framebuffer = engine.frame.fb;

	// Prepare the image for bloom
	threshold.params.viewport = {.size = {currentSize.x >> 1, currentSize.y >> 1}};
	threshold.shader->image = engine.frame.color;
	threshold.shader->threshold = 1.0f;
	threshold.shader->softKnee = 0.25f;
	threshold.shader->strength = 1.0f;
	threshold.draw();

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
		boxBlur.draw();
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
		boxBlur.draw();
	}

	// Draw the bloom on top of the render
	blit.shader->image = bloomFbColor[0];
	blit.shader->boost = 1.0f;
	blit.params.viewport = {.size = engine.window.size};
	blit.draw();
}
