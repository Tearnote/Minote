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

struct SmaaSeparate : Shader {

	Sampler<TextureMS> image;

	void setLocations() override
	{
		image.setLocation(*this, "image");
	}

};

struct SmaaEdge : Shader {

	Sampler<Texture> image;
	Uniform<vec4> screenSize;

	void setLocations() override
	{
		image.setLocation(*this, "image");
		screenSize.setLocation(*this, "screenSize");
	}

};

struct SmaaBlend : Shader {

	Sampler<Texture> edges;
	Sampler<Texture> area;
	Sampler<Texture> search;
	Uniform<vec4> subsampleIndices;
	Uniform<vec4> screenSize;

	void setLocations() override
	{
		edges.setLocation(*this, "edges", TextureUnit::_0);
		area.setLocation(*this, "area", TextureUnit::_1);
		search.setLocation(*this, "search", TextureUnit::_2);
		subsampleIndices.setLocation(*this, "subsampleIndices");
		screenSize.setLocation(*this, "screenSize");
	}

};

struct SmaaNeighbor : Shader {

	Sampler<Texture> image;
	Sampler<Texture> blend;
	Uniform<float> alpha;
	Uniform<vec4> screenSize;

	void setLocations() override
	{
		image.setLocation(*this, "image", TextureUnit::_0);
		blend.setLocation(*this, "blend", TextureUnit::_2);
		alpha.setLocation(*this, "alpha");
		screenSize.setLocation(*this, "screenSize");
	}

};

static const GLchar* SmaaSeparateVertSrc = (GLchar[]){
#include "smaaSeparate.vert"
	'\0'};
static const GLchar* SmaaSeparateFragSrc = (GLchar[]){
#include "smaaSeparate.frag"
	'\0'};

static const GLchar* SmaaEdgeVertSrc = (GLchar[]){
#include "smaaEdge.vert"
	'\0'};
static const GLchar* SmaaEdgeFragSrc = (GLchar[]){
#include "smaaEdge.frag"
	'\0'};

static const GLchar* SmaaBlendVertSrc = (GLchar[]){
#include "smaaBlend.vert"
	'\0'};
static const GLchar* SmaaBlendFragSrc = (GLchar[]){
#include "smaaBlend.frag"
	'\0'};

static const GLchar* SmaaNeighborVertSrc = (GLchar[]){
#include "smaaNeighbor.vert"
	'\0'};
static const GLchar* SmaaNeighborFragSrc = (GLchar[]){
#include "smaaNeighbor.frag"
	'\0'};

static Window* window = nullptr;

// AASimple, AAComplex, AAExtreme
static Framebuffer msaaFb;
static TextureMS<PixelFmt::RGBA_f16> msaaFbColor;
static RenderbufferMS<PixelFmt::DepthStencil> msaaFbDepthStencil;

// AAFast, AAComplex
static Framebuffer smaaEdgeFb;
static Texture<PixelFmt::RGBA_u8> smaaEdgeFbColor;
static Renderbuffer<PixelFmt::DepthStencil> smaaEdgeFbDepthStencil;
static Framebuffer smaaBlendFb;
static Texture<PixelFmt::RGBA_u8> smaaBlendFbColor;

static Texture<PixelFmt::RG_u8> smaaArea;
static Texture<PixelFmt::R_u8> smaaSearch;

static SmaaEdge smaaEdge;
static SmaaBlend smaaBlend;
static SmaaNeighbor smaaNeighbor;

// AAComplex
static Framebuffer smaaSeparateFb;
static Texture<PixelFmt::RGBA_f16> smaaSeparateFbColor;
static Texture<PixelFmt::RGBA_f16> smaaSeparateFbColor2;
static Framebuffer smaaEdgeFb2;
static Texture<PixelFmt::RGBA_u8> smaaEdgeFbColor2;
static Renderbuffer<PixelFmt::DepthStencil> smaaEdgeFbDepthStencil2;
static Framebuffer smaaBlendFb2;
static Texture<PixelFmt::RGBA_u8> smaaBlendFbColor2;

static SmaaSeparate smaaSeparate;

static AAMode currentMode = AANone;
static ivec2 currentSize {0};

static bool initialized = false;

/**
 * Ensure that AA framebuffers are of the same size as the screen. This can be
 * run every frame with the current size of the screen.
 * @param size Current screen size
 */
static void aaResize(ivec2 size)
{
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	if (size.x == currentSize.x && size.y == currentSize.y) return;
	currentSize.x = size.x;
	currentSize.y = size.y;

	if (msaaFbColor.id)
		msaaFbColor.resize(size);
	if (msaaFbDepthStencil.id)
		msaaFbDepthStencil.resize(size);

	if (smaaSeparateFbColor.id)
		smaaSeparateFbColor.resize(size);
	if (smaaEdgeFbColor.id)
		smaaEdgeFbColor.resize(size);
	if (smaaEdgeFbDepthStencil.id)
		smaaEdgeFbDepthStencil.resize(size);
	if (smaaBlendFbColor.id)
		smaaBlendFbColor.resize(size);

	if (smaaSeparateFbColor2.id)
		smaaSeparateFbColor2.resize(size);
	if (smaaEdgeFbColor2.id)
		smaaEdgeFbColor2.resize(size);
	if (smaaEdgeFbDepthStencil2.id)
		smaaEdgeFbDepthStencil2.resize(size);
	if (smaaBlendFbColor2.id)
		smaaBlendFbColor2.resize(size);
}

void aaInit(AAMode mode, Window& w)
{
	if (initialized) return;
	currentMode = mode;
	window = &w;

	// Create needed objects
	const Samples msaaSamples = (currentMode == AASimple) ? Samples::_4 :
	                     (currentMode == AAExtreme) ? Samples::_8 : Samples::_2;

	if (mode == AASimple || mode == AAComplex || mode == AAExtreme) {
		msaaFb.create("msaaFb");
		msaaFbColor.create("msaaFbColor", window->size, msaaSamples);
		msaaFbDepthStencil.create("msaaFbDepthStencil", window->size, msaaSamples);
	}

	if (mode == AAFast || mode == AAComplex) {
		smaaBlendFb.create("smaaBlendFb");
		smaaEdgeFb.create("smaaEdgeFb");
		smaaEdgeFbColor.create("smaaEdgeFbColor", window->size);
		smaaBlendFbColor.create("smaaBlendFbColor", window->size);
		smaaEdgeFbDepthStencil.create("smaaEdgeFbDepthStencil", window->size);
	}

	if (mode == AAComplex) {
		smaaSeparateFb.create("smaaSeparateFb");
		smaaSeparateFbColor.create("smaaSeparateFbColor", window->size);
		smaaSeparateFbColor2.create("smaaSeparateFbColor2", window->size);
		smaaEdgeFb2.create("smaaEdgeFb2");
		smaaEdgeFbColor2.create("smaaEdgeFbColor2", window->size);
		smaaEdgeFbDepthStencil2.create("smaaEdgeFbDepthStencil2", window->size);
		smaaBlendFb2.create("smaaBlendFb2");
		smaaBlendFbColor2.create("smaaBlendFbColor2", window->size);
	}

	// Set up framebuffer textures
	aaResize(window->size);

	// Put framebuffers together and create shaders
	if (mode == AASimple || mode == AAComplex || mode == AAExtreme) {
		msaaFb.attach(msaaFbColor, Attachment::Color0);
		msaaFb.attach(msaaFbDepthStencil, Attachment::DepthStencil);

		if (mode == AAComplex) {
			// Verify that multisampling has the expected subsample layout
			msaaFb.bind();
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
		smaaEdgeFb.attach(smaaEdgeFbColor, Attachment::Color0);
		smaaEdgeFb.attach(smaaEdgeFbDepthStencil, Attachment::DepthStencil);

		smaaBlendFb.attach(smaaBlendFbColor, Attachment::Color0);
		smaaBlendFb.attach(smaaEdgeFbDepthStencil, Attachment::DepthStencil);

		smaaEdge.create("smaaEdge", SmaaEdgeVertSrc, SmaaEdgeFragSrc);
		smaaBlend.create("smaaBlend", SmaaBlendVertSrc, SmaaBlendFragSrc);
		smaaNeighbor.create("smaaNeighbor", SmaaNeighborVertSrc, SmaaNeighborFragSrc);

		// Load lookup textures
		u8vec2 areaTexBytesFlipped[AREATEX_SIZE / 2] = {};
		for (size_t i = 0; i < AREATEX_HEIGHT; i += 1)
			memcpy(areaTexBytesFlipped + i * AREATEX_PITCH,
				areaTexBytes + (AREATEX_HEIGHT - 1 - i) * AREATEX_PITCH,
				AREATEX_PITCH);

		smaaArea.create("smaaArea", {AREATEX_WIDTH, AREATEX_HEIGHT});
		smaaArea.upload(std::to_array(areaTexBytesFlipped));

		unsigned char searchTexBytesFlipped[SEARCHTEX_SIZE] = {0};
		for (size_t i = 0; i < SEARCHTEX_HEIGHT; i += 1)
			memcpy(searchTexBytesFlipped + i * SEARCHTEX_PITCH,
				searchTexBytes + (SEARCHTEX_HEIGHT - 1 - i) * SEARCHTEX_PITCH,
				SEARCHTEX_PITCH);

		smaaSearch.create("smaaSearch", {SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT});
		smaaSearch.setFilter(Filter::Nearest);
		smaaSearch.upload(std::to_array(searchTexBytesFlipped));
	}

	if (mode == AAComplex) {
		smaaSeparateFb.attach(smaaSeparateFbColor, Attachment::Color0);
		smaaSeparateFb.attach(smaaSeparateFbColor2,	Attachment::Color1);

		smaaEdgeFb2.attach(smaaEdgeFbColor2, Attachment::Color0);
		smaaEdgeFb2.attach(smaaEdgeFbDepthStencil2, Attachment::DepthStencil);

		smaaBlendFb2.attach(smaaBlendFbColor2, Attachment::Color0);
		smaaBlendFb2.attach(smaaEdgeFbDepthStencil2, Attachment::DepthStencil);

		smaaSeparate.create("smaaSeparate", SmaaSeparateVertSrc, SmaaSeparateFragSrc);
	}

	renderFb.bind();

	initialized = true;
	L.debug("Initialized AA mode %d", mode);
}

void aaCleanup(void)
{
	if (!initialized) return;
	if (msaaFb.id)
		msaaFb.destroy();
	if (msaaFbColor.id)
		msaaFbColor.destroy();
	if (msaaFbDepthStencil.id)
		msaaFbDepthStencil.destroy();

	if (smaaEdgeFb.id)
		smaaEdgeFb.destroy();
	if (smaaEdgeFbColor.id)
		smaaEdgeFbColor.destroy();
	if (smaaEdgeFbDepthStencil.id)
		smaaEdgeFbDepthStencil.destroy();
	if (smaaBlendFb.id)
		smaaBlendFb.destroy();
	if (smaaBlendFbColor.id)
		smaaBlendFbColor.destroy();

	if (smaaArea.id)
		smaaArea.destroy();
	if (smaaSearch.id)
		smaaSearch.destroy();

	if (smaaEdge.id)
		smaaEdge.destroy();
	if (smaaBlend.id)
		smaaBlend.destroy();
	if (smaaNeighbor.id)
		smaaNeighbor.destroy();

	if (smaaSeparateFb.id)
		smaaSeparateFb.destroy();
	if (smaaSeparateFbColor.id)
		smaaSeparateFbColor.destroy();
	if (smaaSeparateFbColor2.id)
		smaaSeparateFbColor2.destroy();
	if (smaaEdgeFb2.id)
		smaaEdgeFb2.destroy();
	if (smaaEdgeFbColor2.id)
		smaaEdgeFbColor2.destroy();
	if (smaaEdgeFbDepthStencil2.id)
		smaaEdgeFbDepthStencil2.destroy();
	if (smaaBlendFb2.id)
		smaaBlendFb2.destroy();
	if (smaaBlendFbColor2.id)
		smaaBlendFbColor2.destroy();

	if (smaaSeparate.id)
		smaaSeparate.destroy();

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
		msaaFb.bind();
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
		smaaEdge.bind();
		smaaEdgeFb.bind();
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		smaaEdge.image = renderFbColor;
		renderFbColor.setFilter(Filter::Nearest); // It's ok, we turn it back
		smaaEdge.screenSize = {
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y
		};
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// SMAA blending weight calculation pass
		smaaBlend.bind();
		smaaBlendFb.bind();
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilMask(0x00);
		glClear(GL_COLOR_BUFFER_BIT);
		smaaBlend.edges = smaaEdgeFbColor;
		smaaBlend.area = smaaArea;
		smaaBlend.search = smaaSearch;
		smaaBlend.subsampleIndices = {0.0f, 0.0f, 0.0f, 0.0f};
		smaaBlend.screenSize = {
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y
		};
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisable(GL_STENCIL_TEST);

		// SMAA neighbor blending pass
		smaaNeighbor.bind();
		renderFb.bind();
		smaaNeighbor.image = renderFbColor;
		renderFbColor.setFilter(Filter::Linear); // It's ok, we turn it back
		smaaNeighbor.blend = smaaBlendFbColor;
		smaaNeighbor.alpha = 1.0f;
		smaaNeighbor.screenSize = {
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y
		};
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}

	if (currentMode == AAComplex) {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		// SMAA sample separation pass
		smaaSeparate.bind();
		smaaSeparateFb.bind();
		smaaSeparate.image = msaaFbColor;
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// SMAA edge detection pass
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		smaaEdge.bind();
		smaaEdgeFb.bind();
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		smaaEdge.image = smaaSeparateFbColor;
		smaaSeparateFbColor.setFilter(Filter::Nearest);
		smaaEdge.screenSize = {
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y
		};
		glDrawArrays(GL_TRIANGLES, 0, 3);

		smaaEdgeFb2.bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		smaaEdge.image = smaaSeparateFbColor2;
		smaaSeparateFbColor2.setFilter(Filter::Nearest);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// SMAA blending weight calculation pass
		smaaBlend.bind();
		smaaBlendFb.bind();
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilMask(0x00);
		glClear(GL_COLOR_BUFFER_BIT);
		smaaBlend.edges = smaaEdgeFbColor;
		smaaBlend.area = smaaArea;
		smaaBlend.search = smaaSearch;
		smaaBlend.subsampleIndices = {1.0f, 2.0f, 2.0f, 0.0f};
		smaaBlend.screenSize = {
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y
		};
		glDrawArrays(GL_TRIANGLES, 0, 3);

		smaaBlendFb2.bind();
		glClear(GL_COLOR_BUFFER_BIT);
		smaaBlend.edges = smaaEdgeFbColor2;
		smaaBlend.area = smaaArea;
		smaaBlend.search = smaaSearch;
		smaaBlend.subsampleIndices = {2.0f, 1.0f, 1.0f, 0.0f};
		smaaBlend.screenSize = {
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y
		};
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisable(GL_STENCIL_TEST);

		// SMAA neighbor blending pass
		smaaNeighbor.bind();
		renderFb.bind();
		smaaNeighbor.image = smaaSeparateFbColor;
		smaaSeparateFbColor.setFilter(Filter::Linear);
		smaaNeighbor.blend = smaaBlendFbColor;
		smaaNeighbor.alpha = 1.0f;
		smaaNeighbor.screenSize = {
			1.0 / (float)currentSize.x, 1.0 / (float)currentSize.y,
			currentSize.x, currentSize.y
		};
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_BLEND);
		smaaNeighbor.image = smaaSeparateFbColor2;
		smaaSeparateFbColor2.setFilter(Filter::Linear);
		smaaNeighbor.blend = smaaBlendFbColor2;
		smaaNeighbor.alpha = 0.5f;
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glEnable(GL_DEPTH_TEST);
	}

	if (currentMode == AASimple || currentMode == AAExtreme) {
		Framebuffer::blit(renderFb, msaaFb);
	}
}
