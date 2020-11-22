// Minote - sys/opengl/framebuffer.hpp
// Type-safe wrapper of OpenGL framebuffer object (FBO)

#pragma once

#include "glad/glad.h"
#include "base/array.hpp"
#include "sys/opengl/base.hpp"
#include "sys/opengl/texture.hpp"

namespace minote {

// Index of the framebuffer attachment
enum struct Attachment : GLenum {
	None = GL_NONE,
	DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT,
	Color0 = GL_COLOR_ATTACHMENT0,
	Color1 = GL_COLOR_ATTACHMENT1,
	Color2 = GL_COLOR_ATTACHMENT2,
	Color3 = GL_COLOR_ATTACHMENT3,
	Color4 = GL_COLOR_ATTACHMENT4,
	Color5 = GL_COLOR_ATTACHMENT5,
	Color6 = GL_COLOR_ATTACHMENT6,
	Color7 = GL_COLOR_ATTACHMENT7,
	Color8 = GL_COLOR_ATTACHMENT8,
	Color9 = GL_COLOR_ATTACHMENT9,
	Color10 = GL_COLOR_ATTACHMENT10,
	Color11 = GL_COLOR_ATTACHMENT11,
	Color12 = GL_COLOR_ATTACHMENT12,
	Color13 = GL_COLOR_ATTACHMENT13,
	Color14 = GL_COLOR_ATTACHMENT14,
	Color15 = GL_COLOR_ATTACHMENT15,
};

// Framebuffer object wrapper. Proxy object that allows drawing into textures
// and renderbuffers using shaders.
struct Framebuffer : GLObject {

	// Sample count of the attachments; all attachments need to match
	Samples samples = Samples::None;

	// Whether the attachment setup has been modified since the last draw
	// If true, all color attachment will be enabled for drawing
	// and a completeness check will be executed
	bool dirty = true;

	// Register of attached textures. Empty attachments are nullptr. Ordered
	// with an internal method.
	array<TextureBase const*, 17> attachments = {};

	// Initialize the framebuffer object. The object has no textures attached
	// by default, and needs to have at least one color attachment attached
	// to satisfy completeness requirements.
	void create(char const* name);

	// Destroy the framebuffer object. The FBO itself is released, but attached
	// objects continue to exist.
	void destroy();

	// Attach a texture to a specified attachment point. All future attachments
	// must not be multisampled. The DepthStencil attachment can only hold
	// a texture with a DepthStencil pixel format. Attachments cannot
	// be overwritten.
	template<PixelFmt F>
	void attach(Texture<F>& t, Attachment attachment);

	// Attach a multisample texture to a specified attachment point. All future
	// attachments must have the same number of samples. The DepthStencil
	// attachment can only hold a texture with a DepthStencil pixel format.
	// Attachments cannot be overwritten.
	template<PixelFmt F>
	void attach(TextureMS<F>& t, Attachment attachment);

	// Attach a renderbuffer to a specified attachment point. All future
	// attachments must not be multisampled. The DepthStencil attachment can
	// only hold a renderbuffer with a DepthStencil pixel format. Attachments
	// cannot be overwritten.
	template<PixelFmt F>
	void attach(Renderbuffer<F>& r, Attachment attachment);

	// Attach a multisample renderbuffer to a specified attachment point.
	// All future attachments must have the same number of samples.
	// The DepthStencil attachment can only hold a renderbuffer
	// with a DepthStencil pixel format. Attachments cannot be overwritten.
	template<PixelFmt F>
	void attach(RenderbufferMS<F>& r, Attachment attachment);

	// Return the size of the biggest attached texture.
	auto size() -> uvec2;

	// Bind this framebuffer to the OpenGL context, causing all future draw
	// commands to render into the framebuffer's attachments. In a debug build,
	// the framebuffer is checked for completeness.
	void bind();

	// Bind this framebuffer to the read target. Only useful for reading pixels
	// and blitting.
	void bindRead() const;

	// Bind the zero framebuffer, which causes all future draw commands to draw
	// to the backbuffer.
	static void unbind();

	// Copy the contents of one framebuffer to another. MSAA resolve
	// is performed if required. If depthStencil is true, the DS contents
	// will also be copied.
	static void blit(Framebuffer& dst, Framebuffer const& src,
		Attachment srcBuffer = Attachment::Color0, bool depthStencil = false);

};

}

#include "sys/opengl/framebuffer.tpp"
