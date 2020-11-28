#pragma once

#include "base/log.hpp"

namespace minote {

namespace detail {

// Convert an Attachment value to attachments array index.
inline auto attachmentIndex(Attachment const attachment) -> size_t
{
	switch(attachment) {
	case Attachment::DepthStencil:
		return 16;
#ifndef NDEBUG
	case Attachment::None:
		L.warn("Invalid attachment index {}", +attachment);
		return -1;
#endif //NDEBUG
	default:
		return (+attachment) - (+Attachment::Color0);
	}
}

// Retrieve a texture pointer at a specified attachment point.
inline auto getAttachment(Framebuffer& f, Attachment const attachment) -> TextureBase const*&
{
	return f.attachments[attachmentIndex(attachment)];
}

inline auto getAttachment(Framebuffer const& f, Attachment const attachment) -> TextureBase const*
{
	return f.attachments[attachmentIndex(attachment)];
}

}

template<PixelFmt F>
void Framebuffer::attach(Texture<F>& t, Attachment const attachment)
{
	ASSERT(id);
	ASSERT(t.id);
	ASSERT(attachment != Attachment::None);
	if (t.Format == PixelFmt::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != Samples::None)
		ASSERT(samples == Samples::_1);
	ASSERT(!detail::getAttachment(*this, attachment));

	dirty = false; // Prevent checking validity
	bind();
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_TEXTURE_2D, t.id, 0);
	detail::getAttachment(*this, attachment) = &t;
	samples = Samples::_1;
	dirty = true;
}

template<PixelFmt F>
void Framebuffer::attach(TextureMS<F>& t, Attachment const attachment)
{
	ASSERT(id);
	ASSERT(t.id);
	ASSERT(attachment != Attachment::None);
	if (t.Format == PixelFmt::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != Samples::None)
		ASSERT(samples == t.samples);

	dirty = false; // Prevent checking validity
	bind();
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_TEXTURE_2D_MULTISAMPLE, t.id, 0);
	detail::getAttachment(*this, attachment) = &t;
	samples = t.samples;
	dirty = true;
}

template<PixelFmt F>
void Framebuffer::attach(Renderbuffer<F>& r, Attachment const attachment)
{
	ASSERT(id);
	ASSERT(r.id);
	ASSERT(attachment != Attachment::None);
	if (r.Format == PixelFmt::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != Samples::None)
		ASSERT(samples == Samples::_1);

	dirty = false; // Prevent checking validity
	bind();
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_RENDERBUFFER, r.id);
	detail::getAttachment(*this, attachment) = &r;
	samples = Samples::_1;
	dirty = true;
}

template<PixelFmt F>
void Framebuffer::attach(RenderbufferMS<F>& r, Attachment const attachment)
{
	ASSERT(id);
	ASSERT(r.id);
	ASSERT(attachment != Attachment::None);
	if (r.Format == PixelFmt::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != Samples::None)
		ASSERT(samples == r.samples);

	dirty = false; // Prevent checking validity
	bind();
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_RENDERBUFFER, r.id);
	detail::getAttachment(*this, attachment) = &r;
	samples = r.samples;
	dirty = true;
}

}
