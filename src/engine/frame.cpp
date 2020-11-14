/**
 * Implementation of engine/frame.hpp
 * @file
 */

#include "engine/frame.hpp"

namespace minote {

static void createFramebuffers(Frame& frame, uvec2 const size, Samples const _aa)
{
	ASSERT(_aa != Samples::None);

	frame.ssfb.create("Frame::ssfb");
	frame.color.create("Frame::color", size);
	frame.depthStencil.create("Frame::depthStencil", size);
	frame.ssfb.attach(frame.color, Attachment::Color0);
	frame.ssfb.attach(frame.depthStencil, Attachment::DepthStencil);

	if (_aa != Samples::_1) {
		frame.msfb.create("Frame::msfb");
		frame.colorMS.create("Frame::colorMS", size, _aa);
		frame.depthStencilMS.create("Frame::depthStencilMS", size, _aa);
		frame.msfb.attach(frame.colorMS, Attachment::Color0);
		frame.msfb.attach(frame.depthStencilMS, Attachment::DepthStencil);
	}
}

static void destroyFramebuffers(Frame& frame)
{
	if (frame.fb)
		frame.fb = nullptr;
	if (frame.ssfb.id)
		frame.ssfb.destroy();
	if (frame.msfb.id)
		frame.msfb.destroy();
	if (frame.color.id)
		frame.color.destroy();
	if (frame.depthStencil.id)
		frame.depthStencil.destroy();
	if (frame.colorMS.id)
		frame.colorMS.destroy();
	if (frame.depthStencilMS.id)
		frame.depthStencilMS.destroy();
}

void Frame::create(uvec2 const size, Shaders& shaders, Samples const _aa)
{
	ASSERT(_aa != Samples::None);
	ASSERT(aa == Samples::None);

	createFramebuffers(*this, size, _aa);

	delinearize = {
		.shader = &shaders.delinearize,
		.framebuffer = nullptr,
		.triangles = 1,
		.params = {
			.culling = false,
			.depthTesting = false
		}
	};

	aa = _aa;
	L.debug("Frame created with MSAA %dx", +aa);
}

void Frame::destroy()
{
	ASSERT(aa != Samples::None);

	destroyFramebuffers(*this);

	aa = Samples::None;
	L.debug("Frame destroyed");
}

void Frame::changeAA(Samples const _aa)
{
	ASSERT(aa != Samples::None);
	if (aa == _aa) return;

	uvec2 const size = color.size;
	destroyFramebuffers(*this);
	createFramebuffers(*this, size, _aa);
	aa = _aa;
}

void Frame::resolveAA()
{
	ASSERT(aa != Samples::None);
	if(aa == Samples::_1) return;
	ASSERT(fb == &msfb);

	Framebuffer::blit(ssfb, msfb, Attachment::Color0, true);

	fb = &ssfb;
}

void Frame::begin(uvec2 const size)
{
	ASSERT(aa != Samples::None);

	color.resize(size);
	depthStencil.resize(size);
	if (aa != Samples::_1) {
		colorMS.resize(size);
		depthStencilMS.resize(size);
	}

	if (aa == Samples::_1)
		fb = &ssfb;
	else
		fb = &msfb;

	delinearize.params.viewport = {.size = size};
}

void Frame::end()
{
	ASSERT(aa != Samples::None);

	if (fb == &msfb)
		resolveAA();

	delinearize.shader->image = color;
	delinearize.draw();
}

}
