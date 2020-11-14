/**
 * The main rendertarget of the game
 * @file
 */

#pragma once

#include "sys/opengl.hpp"
#include "store/shaders.hpp"

namespace minote {

struct Frame {

	Framebuffer* fb = nullptr;
	Framebuffer ssfb;
	Framebuffer msfb;
	Texture<PixelFmt::RGBA_f16> color;
	Renderbuffer<PixelFmt::DepthStencil> depthStencil;
	TextureMS<PixelFmt::RGBA_f16> colorMS;
	RenderbufferMS<PixelFmt::DepthStencil> depthStencilMS;
	Samples aa = Samples::None;
	Draw<Shaders::Delinearize> delinearize;

	void create(uvec2 size, Shaders& shaders, Samples aa = Samples::_1);

	void destroy();

	void changeAA(Samples aa);

	void resolveAA();

	void begin(uvec2 size);

	void end();

};

}
