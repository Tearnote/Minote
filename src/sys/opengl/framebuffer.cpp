#include "sys/opengl/framebuffer.hpp"

#include <cstring>
#include "base/log.hpp"
#include "sys/opengl/state.hpp"

namespace minote {

void Framebuffer::create(char const* const _name)
{
	ASSERT(!id);
	ASSERT(_name);

	glGenFramebuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_FRAMEBUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;

	L.debug(R"(Framebuffer "%s" created)", name);
}

void Framebuffer::destroy()
{
	ASSERT(id);

	detail::state.deleteFramebuffer(id);
	id = 0;
	samples = Samples::None;
	dirty = true;
	attachments.fill(nullptr);

	L.debug(R"(Framebuffer "%s" destroyed)", name);
	name = nullptr;
}

auto Framebuffer::size() -> uvec2
{
	uvec2 result = {0, 0};
	for (auto const* const attachment : attachments) {
		if (!attachment) continue;
		result = max(result, attachment->size);
	}
	return result;
}

void Framebuffer::bind()
{
	ASSERT(id);

	detail::state.bindFramebuffer(GL_DRAW_FRAMEBUFFER, id);

	if (dirty) {
		// Call glDrawBuffers with all enabled color framebuffers
		auto const[enabledBuffers, enabledSize] = [this] {
			array<GLenum, 16> buffers = {};
			size_t size = 0;
			for (size_t i = 0; i < 16; i += 1) {
				if (!attachments[i])
					continue;
				buffers[size] = i + GL_COLOR_ATTACHMENT0;
				size += 1;
			}
			return std::make_pair(buffers, size);
		}();

		glDrawBuffers(enabledSize, enabledBuffers.data());

#ifndef NDEBUG
		// Check framebuffer correctness
		if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			L.error(R"(Framebuffer "%s" validity check failed)", name);
#endif //NDEBUG

		dirty = false;
	}
}

void Framebuffer::bindRead() const
{
	ASSERT(id);
	ASSERT(!dirty);

	detail::state.bindFramebuffer(GL_READ_FRAMEBUFFER, id);
}

void Framebuffer::unbind()
{
	detail::state.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Framebuffer::blit(Framebuffer& dst, Framebuffer const& src,
	Attachment const srcBuffer, bool const depthStencil)
{
	ASSERT(detail::getAttachment(src, srcBuffer));
	if (depthStencil) {
		ASSERT(detail::getAttachment(src, Attachment::DepthStencil));
		ASSERT(detail::getAttachment(dst, Attachment::DepthStencil));
	}

	src.bindRead();
	dst.bind();
	glReadBuffer(+srcBuffer);

	uvec2 const blitSize = src.attachments[detail::attachmentIndex(srcBuffer)]->size;
	GLbitfield const mask = GL_COLOR_BUFFER_BIT |
		(depthStencil? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : 0);

	glBlitFramebuffer(0, 0, blitSize.x, blitSize.y,
		0, 0, blitSize.x, blitSize.y,
		mask, GL_NEAREST);
}

}
