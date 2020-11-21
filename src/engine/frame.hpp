// Minote - engine/frame.hpp
// The main rendertarget of the game

#pragma once

#include "sys/opengl/framebuffer.hpp"
#include "sys/opengl/texture.hpp"
#include "sys/opengl/draw.hpp"
#include "store/shaders.hpp"

namespace minote {

struct Frame {

	// Current main framebuffer. Might be singlesampled or multisampled,
	// and might change during the course of a frame. Is nullptr outside
	// of a begin()/end() pair.
	Framebuffer* fb = nullptr;

	// Size of the current frame after a begin() call. Use this for
	// the viewport parameter of a Draw.
	uvec2 size = {};

	// Singlesampled framebuffer
	Framebuffer ssfb;

	// Multisampled framebuffer. Might not exist in certain AA modes
	Framebuffer msfb;

	// Color attachment of ssfb
	Texture<PixelFmt::RGBA_f16> color;

	// DS attachment of ssfb
	Renderbuffer<PixelFmt::DepthStencil> depthStencil;

	// Color attachment of msfb
	TextureMS<PixelFmt::RGBA_f16> colorMS;

	// DS attachment of msfb
	RenderbufferMS<PixelFmt::DepthStencil> depthStencilMS;

	// Current antialiasing mode. Change with changeAA()
	Samples aa = Samples::None;

	// Drawcall of the final blit to the backbuffer
	Draw<Shaders::Delinearize> delinearize;

	// Initialize the frame, with the specified initial size
	// and antialiasing mode.
	void create(uvec2 size, Shaders& shaders, Samples aa = Samples::_1);

	// Destroy the frame, freeing all used resources.
	void destroy();

	// Switch antialiasing modes. This operation leaves all framebuffer
	// attachments with undefined contents.
	void changeAA(Samples aa);

	// Switch from the multisampled framebuffer to the singlesampled one,
	// preserving color and DS contents. Safe no-op if antialiasing is disabled.
	void resolveAA();

	// Prepare the frame for drawing into. All attachments are resized
	// to the specified size. fb and size become available as framebuffer
	// and viewport parameters to Draw. The initial contents of the attachments
	// are undefined.
	void begin(uvec2 size);

	// Finalize the frame and blit it to the backbuffer. Alpha correction
	// is performed, and antialiasing is resolved if required.
	void end();

};

}
