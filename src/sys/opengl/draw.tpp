#pragma once

namespace minote {

template<ShaderType T>
void Draw<T>::draw()
{
	if (framebuffer)
		framebuffer->bind();
	else
		Framebuffer::unbind();

	if (clearColor || clearDepthStencil) {
		GLbitfield mask = 0u;
		if (clearColor) {
			detail::state.setClearColor(clearParams.color);
			mask |= GL_COLOR_BUFFER_BIT;
		}
		if (clearDepthStencil) {
			DASSERT(!framebuffer || detail::getAttachment(*framebuffer, Attachment::DepthStencil));
			detail::state.setClearDepth(clearParams.depth);
			detail::state.setClearStencil(clearParams.stencil);
			mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		}
		glClear(mask);
	}

	if (shader && instances > 0) {
		DASSERT(framebuffer || !params.viewport.zero());
		bool const instanced = (instances > 1);
		bool const indexed = (vertexarray && vertexarray->elementBits);
		auto const vertices = [this]() -> GLsizei {
			switch (mode) {
			case DrawMode::Triangles:
				return triangles * 3;
			case DrawMode::TriangleStrip:
				return triangles + 2;
			default:
				L.fail("Unknown draw mode");
			}
		}();

		if (params.viewport.zero()) {
			params.viewport.size = framebuffer->size();
			params.set();
			params.viewport.size = {0, 0};
		} else {
			params.set();
		}
		shader->bind();
		if (vertexarray)
			vertexarray->bind();

		if (!indexed) {

			if (!instanced)
				glDrawArrays(+mode, offset, vertices);
			else
				glDrawArraysInstanced(+mode, offset, vertices, instances);

		} else {

			GLenum const indexType = [=, this] {
				switch (vertexarray->elementBits) {
				case 8: return GL_UNSIGNED_BYTE;
				case 16: return GL_UNSIGNED_SHORT;
				case 32: return GL_UNSIGNED_INT;
				default: return 0;
				}
			}();
			if (!instanced)
				glDrawElements(+mode, vertices, indexType,
					reinterpret_cast<GLvoid*>(offset * sizeof(u32)));
			else
				glDrawElementsInstanced(+mode, vertices, indexType,
					reinterpret_cast<GLvoid*>(offset * sizeof(u32)), instances);

		}
	}
}

}
