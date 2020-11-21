// Minote - sys/opengl/framebuffer.hpp
// Type-safe wrapper of OpenGL framebuffer object (FBO)

#pragma once

#include "glad/glad.h"
#include "base/array.hpp"
#include "sys/opengl/base.hpp"
#include "sys/opengl/texture.hpp"

namespace minote {

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

/// OpenGL framebuffer. Proxy object that allows drawing into textures
/// and renderbuffers from within shaders.
struct Framebuffer : GLObject {

	Samples samples = Samples::None; ///< Sample count of all attachments needs to match
	bool dirty = true; ///< Is a glDrawBuffers call and completeness check needed?
	array<TextureBase const*, 17> attachments = {};

	/**
	 * Create an OpenGL ID for the framebuffer. This needs to be called before
	 * the framebuffer can be used. The object has no textures attached
	 * by default, and needs to have at least one color attachment attached
	 * to satisfy completeness requirements.
	 * @param name Human-readable name, for logging and debug output
	 */
	void create(char const* name);

	/**
	 * Destroy the OpenGL framebuffer object. The ID is released, but attached
	 * objects continue to exist.
	 */
	void destroy();

	/**
	 * Attach a texture to a specified attachment point. All future attachments
	 * must not be multisample. The DepthStencil attachment can only hold
	 * a texture with a DepthStencil pixel format. Attachments cannot
	 * be overwritten.
	 * @param t Texture to attach
	 * @param attachment Attachment point to attach to
	 */
	template<PixelFmt F>
	void attach(Texture<F>& t, Attachment attachment);

	/**
	 * Attach a multisample texture to a specified attachment point. All future
	 * attachments must have the same number of samples. The DepthStencil
	 * attachment can only hold a texture with a DepthStencil pixel format.
	 * Attachments cannot be overwritten.
	 * @param t Multisample texture to attach
	 * @param attachment Attachment point to attach to
	 */
	template<PixelFmt F>
	void attach(TextureMS<F>& t, Attachment attachment);

	/**
	 * Attach a renderbuffer to a specified attachment point. All future
	 * attachments must not be multisample. The DepthStencil attachment can only
	 * hold a renderbuffer with a DepthStencil pixel format. Attachments
	 * cannot be overwritten.
	 * @param t Renderbuffer to attach
	 * @param attachment Attachment point to attach to
	 */
	template<PixelFmt F>
	void attach(Renderbuffer<F>& r, Attachment attachment);

	/**
	 * Attach a multisample renderbuffer to a specified attachment point.
	 * All future attachments must have the same number of samples.
	 * The DepthStencil attachment can only hold a renderbuffer
	 * with a DepthStencil pixel format. Attachments cannot be overwritten.
	 * @param t Multisample renderbuffer to attach
	 * @param attachment Attachment point to attach to
	 */
	template<PixelFmt F>
	void attach(RenderbufferMS<F>& r, Attachment attachment);

	auto size() -> uvec2;

	/**
	 * Bind this framebuffer to the OpenGL context, causing all future draw
	 * commands to render into the framebuffer's attachments. In a debug build,
	 * the framebuffer is checked for completeness.
	 */
	void bind();

	void bindRead() const;

	/**
	 * Bind the zero framebuffer, which causes all future draw commands to draw
	 * to the window surface.
	 */
	static void unbind();

	/**
	 * Copy the contents of one framebuffer to another. MSAA resolve
	 * is performed if required.
	 * @param dst Destination framebuffer
	 * @param src Source framebuffer
	 * @param srcBuffer The attachment in src to read from
	 * @param depthStencil Whether to blit the DepthStencil attachment
	 */
	static void blit(Framebuffer& dst, Framebuffer const& src,
		Attachment srcBuffer = Attachment::Color0, bool depthStencil = false);

};

}

#include "sys/opengl/framebuffer.tpp"
