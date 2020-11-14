/**
 * Template implementation of opengl.hpp
 * @file
 */

#pragma once

#include "base/log.hpp"

namespace minote {

namespace detail {

struct GLState {

	struct GLBlendingMode {

		GLenum src = GL_ONE;
		GLenum dst = GL_ZERO;

	};

	struct GLTextureUnitState {

		GLuint texture2D = 0;
		GLuint texture2DMS = 0;
		GLuint bufferTexture = 0;

	};

	struct GLStencilMode {

		GLenum func = GL_ALWAYS;
		GLint ref = 0;

		GLenum sfail = GL_KEEP;
		GLenum dpfail = GL_KEEP;
		GLenum dppass = GL_KEEP;

	};

	// Rasterizer features
	bool blending = false;
	GLBlendingMode blendingMode;
	bool culling = false;
	bool depthTesting = false;
	GLenum depthFunc = GL_LESS;
	bool scissorTesting = false;
	AABB<2, i32> scissorBox;
	bool stencilTesting = false;
	GLStencilMode stencilMode;
	AABB<2, i32> viewport;
	bool colorWrite = true;

	void setFeature(GLenum feature, bool state);

	void setBlendingMode(GLBlendingMode mode);

	void setDepthFunc(GLenum func);

	void setScissorBox(AABB<2, i32> box);

	void setStencilMode(GLStencilMode mode);

	void setViewport(AABB<2, i32> box);

	void setColorWrite(bool state);

	// Object bindings
	GLuint vertexbuffer = 0;
	GLuint elementbuffer = 0;
	GLuint texturebuffer = 0;
	GLuint vertexarray = 0;
	TextureUnit currentUnit = TextureUnit::_0;
	array<GLTextureUnitState, 16> textures;
	GLuint renderbuffer = 0;
	GLuint framebufferRead = 0;
	GLuint framebufferWrite = 0;
	GLuint shader = 0;

	void bindBuffer(GLenum target, GLuint id);

	void bindVertexArray(GLuint id);

	void setTextureUnit(TextureUnit unit);

	void bindTexture(GLenum target, GLuint id);

	void bindRenderbuffer(GLuint id);

	void bindFramebuffer(GLenum target, GLuint id);

	void bindShader(GLuint id);

	// Object deletion
	void deleteBuffer(GLenum target, GLuint id);

	void deleteVertexArray(GLuint id);

	void deleteTexture(GLenum target, GLuint id);

	void deleteRenderbuffer(GLuint id);

	void deleteFramebuffer(GLuint id);

};

inline void GLState::setFeature(GLenum const feature, bool const state)
{
	auto& featureState = [=, this]() -> bool& {
		switch (feature) {
		case GL_BLEND:
			return blending;
		case GL_CULL_FACE:
			return culling;
		case GL_DEPTH_TEST:
			return depthTesting;
		case GL_SCISSOR_TEST:
			return scissorTesting;
		case GL_STENCIL_TEST:
			return stencilTesting;
		default:
			L.fail("Unknown rasterizer feature");
		}
	}();
	auto const stateFunc = [=] {
		if (state)
			return glEnable;
		return glDisable;
	}();

	if (state == featureState) return;

	stateFunc(feature);
	featureState = state;
}

inline void GLState::setBlendingMode(GLBlendingMode const mode)
{
	if (mode.src == blendingMode.src && mode.dst == blendingMode.dst) return;

	glBlendFunc(mode.src, mode.dst);
	blendingMode = mode;
}

inline void GLState::setDepthFunc(GLenum func)
{
	if (func == depthFunc) return;

	glDepthFunc(func);
	depthFunc = func;
}

inline void GLState::setScissorBox(AABB<2, i32> const box)
{
	if (box == scissorBox) return;

	glScissor(box.pos.x, box.pos.y, box.size.x, box.size.y);
	scissorBox = box;
}

inline void GLState::setStencilMode(GLStencilMode const mode)
{
	if (mode.func != stencilMode.func ||
		mode.ref != stencilMode.ref) {
		glStencilFunc(mode.func, mode.ref, 0xFFFF'FFFF);
		stencilMode.func = mode.func;
		stencilMode.ref = mode.ref;
	}

	if (mode.sfail != stencilMode.sfail ||
		mode.dpfail != stencilMode.dpfail ||
		mode.dppass != stencilMode.dppass) {
		glStencilOp(mode.sfail, mode.dpfail, mode.dppass);
		stencilMode.sfail = mode.sfail;
		stencilMode.dpfail = mode.dpfail;
		stencilMode.dppass = mode.dppass;
	}
}

inline void GLState::setViewport(AABB<2, i32> const box)
{
	if (box == viewport) return;

	glViewport(box.pos.x, box.pos.y, box.size.x, box.size.y);
	viewport = box;
}

inline void GLState::setColorWrite(bool const state)
{
	if (state == colorWrite) return;

	GLboolean const glState = state? GL_TRUE : GL_FALSE;
	glColorMask(glState, glState, glState, glState);
	colorWrite = state;
}

inline void GLState::bindBuffer(GLenum const target, GLuint const id)
{
	auto& binding = [=, this]() -> GLuint& {
		switch (target) {
		case GL_ARRAY_BUFFER:
			return vertexbuffer;
		case GL_ELEMENT_ARRAY_BUFFER:
			return elementbuffer;
		case GL_TEXTURE_BUFFER:
			return texturebuffer;
		default:
			L.fail("Unknown buffer type");
		}
	}();

	if (id == binding) return;

	glBindBuffer(target, id);
	binding = id;
}

inline void GLState::bindVertexArray(GLuint const id)
{
	if (id == vertexarray) return;

	glBindVertexArray(id);
	vertexarray = id;
}

inline void GLState::setTextureUnit(TextureUnit const unit)
{
	if (unit == TextureUnit::None || unit == currentUnit) return;

	glActiveTexture(+unit);
	currentUnit = unit;
}

inline void GLState::bindTexture(GLenum const target, GLuint const id)
{
	size_t const unitIndex = +currentUnit - GL_TEXTURE0;
	auto& binding = [=, this]() -> GLuint& {
		switch (target) {
		case GL_TEXTURE_2D:
			return textures[unitIndex].texture2D;
		case GL_TEXTURE_2D_MULTISAMPLE:
			return textures[unitIndex].texture2DMS;
		case GL_TEXTURE_BUFFER:
			return textures[unitIndex].bufferTexture;
		default:
			L.fail("Unknown texture type");
		}
	}();

	if (id == binding) return;

	glBindTexture(target, id);
	binding = id;
}

inline void GLState::bindRenderbuffer(GLuint const id)
{
	if (id == renderbuffer) return;

	glBindRenderbuffer(GL_RENDERBUFFER, id);
	renderbuffer = id;
}

inline void GLState::bindFramebuffer(GLenum const target, GLuint const id)
{
	auto& binding = [=, this]() -> GLuint& {
		switch (target) {
		case GL_READ_FRAMEBUFFER:
			return framebufferRead;
		case GL_DRAW_FRAMEBUFFER:
			return framebufferWrite;
		default:
			L.fail("Unknown framebuffer binding");
		}
	}();
	
	if (id == binding) return;
	
	glBindFramebuffer(target, id);
	binding = id;
}

inline void GLState::bindShader(GLuint const id)
{
	if (id == shader) return;

	glUseProgram(id);
	shader = id;
}

inline void GLState::deleteBuffer(GLenum const target, GLuint const id)
{
	auto& binding = [=, this]() -> GLuint& {
		switch (target) {
		case GL_ARRAY_BUFFER:
			return vertexbuffer;
		case GL_ELEMENT_ARRAY_BUFFER:
			return elementbuffer;
		case GL_TEXTURE_BUFFER:
			return texturebuffer;
		default:
			L.fail("Unknown buffer type");
		}
	}();

	glDeleteBuffers(1, &id);
	if (id == binding)
		binding = 0;
}

inline void GLState::deleteVertexArray(GLuint const id)
{
	glDeleteVertexArrays(1, &id);
	if (id == vertexarray)
		vertexarray = 0;
}

inline void GLState::deleteTexture(GLenum const target, GLuint const id)
{
	glDeleteTextures(1, &id);
	for (auto& unit : textures) {
		auto& binding = [=, &unit]() -> GLuint& {
			switch (target) {
			case GL_TEXTURE_2D:
				return unit.texture2D;
			case GL_TEXTURE_2D_MULTISAMPLE:
				return unit.texture2DMS;
			case GL_TEXTURE_BUFFER:
				return unit.bufferTexture;
			default:
				L.fail("Unknown texture type");
			}
		}();
		if (id == binding)
			binding = 0;
	}
}

inline void GLState::deleteRenderbuffer(GLuint const id)
{
	glDeleteRenderbuffers(1, &id);
	if (id == renderbuffer)
		renderbuffer = 0;
}

inline void GLState::deleteFramebuffer(GLuint const id)
{
	glDeleteFramebuffers(1, &id);
	if (id == framebufferRead)
		framebufferRead = 0;
	if (id == framebufferWrite)
		framebufferWrite = 0;
}

inline auto attachmentIndex(Attachment const attachment) -> size_t
{
	switch(attachment) {
	case Attachment::DepthStencil:
		return 16;
#ifndef NDEBUG
	case Attachment::None:
		L.warn("Invalid attachment index %d", +attachment);
		return -1;
#endif //NDEBUG
	default:
		return (+attachment) - (+Attachment::Color0);
	}
}

/**
 * Helper function to retrieve a texture pointer at a specified attachment point
 * @param f Framebuffer object
 * @param attachment Attachment point
 * @return The pointer to texture at given attachment point (can be nullptr)
 */
inline auto getAttachment(Framebuffer& f, Attachment const attachment) -> TextureBase const*&
{
	return f.attachments[attachmentIndex(attachment)];
}

inline auto getAttachment(Framebuffer const& f, Attachment const attachment) -> TextureBase const*
{
	return f.attachments[attachmentIndex(attachment)];
}

template<GLSLType Component, typename T>
auto setVaoAttribute(VertexArray& vao, GLuint const index,
	VertexBuffer<T>& buffer, std::ptrdiff_t const offset, bool const instanced)
{
	constexpr auto components = []() -> GLint {
		if constexpr (std::is_same_v<Component, vec2> ||
			std::is_same_v<Component, ivec2> ||
			std::is_same_v<Component, uvec2>)
			return 2;
		if constexpr (std::is_same_v<Component, vec3> ||
			std::is_same_v<Component, ivec3> ||
			std::is_same_v<Component, uvec3>)
			return 3;
		if constexpr (std::is_same_v<Component, vec4> ||
			std::is_same_v<Component, ivec4> ||
			std::is_same_v<Component, uvec4> ||
			std::is_same_v<Component, mat4>)
			return 4;
		return 1;
	}();
	constexpr auto type = []() -> GLenum {
		if constexpr (std::is_same_v<Component, f32> ||
			std::is_same_v<Component, vec2> ||
			std::is_same_v<Component, vec3> ||
			std::is_same_v<Component, vec4> ||
			std::is_same_v<Component, mat4>)
			return GL_FLOAT;
		if constexpr (std::is_same_v<Component, u32> ||
			std::is_same_v<Component, uvec2> ||
			std::is_same_v<Component, uvec3> ||
			std::is_same_v<Component, uvec4>)
			return GL_UNSIGNED_INT;
		if constexpr (std::is_same_v<Component, i32> ||
			std::is_same_v<Component, ivec2> ||
			std::is_same_v<Component, ivec3> ||
			std::is_same_v<Component, ivec4>)
			return GL_INT;
		L.fail("Unknown vertex array component type");
	}();

	vao.bind();
	buffer.bind();
	if constexpr (type == GL_FLOAT) {
		if constexpr (std::is_same_v<Component, mat4>) {

			// Matrix version
			for (GLuint i = 0; i < 4; i += 1) {
				glEnableVertexAttribArray(index + i);
				glVertexAttribPointer(index + i, components, type, GL_FALSE,
					sizeof(T), reinterpret_cast<void*>(offset + sizeof(vec4) * i));
				if (instanced)
					glVertexAttribDivisor(index + i, 1);
				vao.attributes[index + i] = true;
			}

		} else {

			// Float scalar/array version
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index, components, type, GL_FALSE, sizeof(T),
				reinterpret_cast<void*>(offset));
			if (instanced)
				glVertexAttribDivisor(index, 1);
			vao.attributes[index] = true;

		}
	} else if constexpr (type == GL_UNSIGNED_INT || type == GL_INT) {

		// Integer scalar/array version
		glEnableVertexAttribArray(index);
		glVertexAttribIPointer(index, components, type, sizeof(T),
			reinterpret_cast<void*>(offset));
		if (instanced)
			glVertexAttribDivisor(index, 1);
		vao.attributes[index] = true;

	}

	L.debug(R"(Buffer "%s" bound to attribute %d of VAO "%s")",
		buffer.name, index, vao.name);
}

inline thread_local GLState state;

}

template<TriviallyCopyable T, GLenum _target>
void BufferBase<T, _target>::create(char const* const _name, bool const _dynamic)
{
		ASSERT(!id);
		ASSERT(_name);

	glGenBuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_BUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG

	name = _name;
	dynamic = _dynamic;

	L.debug(R"(%s vertex buffer "%s" created)",
		dynamic? "Dynamic" : "Static", name);
}

template<TriviallyCopyable T, GLenum _target>
void BufferBase<T, _target>::destroy()
{
	ASSERT(id);

	detail::state.deleteBuffer(Target, id);
	id = 0;
	dynamic = false;
	uploaded = false;

	L.debug(R"(Vertex buffer "%s" destroyed)", name);
	name = nullptr;
}

template<TriviallyCopyable T, GLenum _target>
template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, T, N>
void BufferBase<T, _target>::upload(Arr<Type, N> const& data)
{
	ASSERT(id);
	ASSERT(dynamic == true || uploaded == false);
	if(!data.size()) return;

	bind();
	GLenum const usage = dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW;
	GLsizeiptr const size = sizeof(Type) * data.size();
	if (dynamic && uploaded) {
		glBufferData(Target, size, nullptr, usage);
		glBufferSubData(Target, 0, size, data.data());
	} else {
		glBufferData(Target, size, data.data(), usage);
		uploaded = true;
	}
}

template<TriviallyCopyable T, GLenum _target>
void BufferBase<T, _target>::upload(size_t elements, Type data[])
{
	ASSERT(data);
	ASSERT(id);
	ASSERT(dynamic == true || uploaded == false);
	if (!elements)
		return;

	bind();
	GLenum const usage = dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW;
	GLsizeiptr const size = sizeof(Type) * elements;
	if (dynamic && uploaded) {
		glBufferData(Target, size, nullptr, usage);
		glBufferSubData(Target, 0, size, data);
	} else {
		glBufferData(Target, size, data, usage);
		uploaded = true;
	}
}

template<TriviallyCopyable T, GLenum _target>
void BufferBase<T, _target>::bind() const
{
	ASSERT(id);

	detail::state.bindBuffer(Target, id);
}

template<PixelFmt F>
void Texture<F>::create(char const* const _name, uvec2 const _size)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(Format != PixelFmt::None);

	glGenTextures(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_TEXTURE, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	setFilter(Filter::Linear);
	resize(_size);

	L.debug(R"(Texture "%s" created)", name);
}

template<PixelFmt F>
void Texture<F>::destroy()
{
	ASSERT(id);

	detail::state.deleteTexture(GL_TEXTURE_2D, id);
	id = 0;
	size = {0, 0};
	filter = Filter::None;

	L.debug(R"(Texture "%s" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void Texture<F>::setFilter(Filter const _filter)
{
	ASSERT(_filter != Filter::None);
	if (filter == _filter)
		return;

	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, +_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, +_filter);
	filter = _filter;
}

template<PixelFmt F>
void Texture<F>::resize(uvec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	bind();
	glTexImage2D(GL_TEXTURE_2D, 0, +Format,
		_size.x, _size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	size = _size;
}

template<PixelFmt F>
template<template<UploadFmt, size_t> typename Arr, UploadFmt T, size_t N>
	requires ArrayContainer<Arr, T, N>
void Texture<F>::upload(Arr<T, N> const& data)
{
	ASSERT(data.size() == size.x * size.y);
	ASSERT(id);
	ASSERT(size.x > 0 && size.y > 0);
	ASSERT(Format != PixelFmt::DepthStencil);

	constexpr GLenum channels = [] {
		if constexpr (std::is_same_v<T, u8>)
			return GL_RED;
		else if constexpr (std::is_same_v<T, u8vec2>)
			return GL_RG;
		else if constexpr (std::is_same_v<T, u8vec3>)
			return GL_RGB;
		else if constexpr (std::is_same_v<T, u8vec4>)
			return GL_RGBA;
		else
			ASSERT("Invalid texture upload type");
		return GL_NONE;
	}();

	bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y,
		channels, GL_UNSIGNED_BYTE, data.data());
}

template<PixelFmt F>
template<UploadFmt T>
void Texture<F>::upload(T const data[], int const _channels)
{
	ASSERT(data);
	ASSERT(_channels >= 0 && _channels <= 4);
	ASSERT(id);
	ASSERT(size.x > 0 && size.y > 0);
	ASSERT(Format != PixelFmt::DepthStencil);

	const GLenum channels = [=] {
		if (!_channels) {
			if constexpr (std::is_same_v<T, u8>)
				return GL_RED;
			else if constexpr (std::is_same_v<T, u8vec2>)
				return GL_RG;
			else if constexpr (std::is_same_v<T, u8vec3>)
				return GL_RGB;
			else if constexpr (std::is_same_v<T, u8vec4>)
				return GL_RGBA;
			else
				ASSERT("Invalid texture upload type");
			return GL_NONE;
		} else {
			switch (_channels) {
			case 1: return GL_RED;
			case 2: return GL_RG;
			case 3: return GL_RGB;
			case 4: return GL_RGBA;
			default:
				ASSERT("Invalid texture upload type");
				return GL_NONE;
			}
		}
	}();

	bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y,
		channels, GL_UNSIGNED_BYTE, data);
}

template<PixelFmt F>
void Texture<F>::bind(TextureUnit const unit)
{
	ASSERT(id);

	detail::state.setTextureUnit(unit);
	detail::state.bindTexture(GL_TEXTURE_2D, id);
}

template<PixelFmt F>
void TextureMS<F>::create(char const* const _name, uvec2 const _size, Samples const _samples)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(Format != PixelFmt::None);
	ASSERT(+_samples >= 2);

	glGenTextures(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_TEXTURE, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	samples = _samples;
	resize(_size);

	L.debug(R"(Multisample texture "%s" created)", name);
}

template<PixelFmt F>
void TextureMS<F>::destroy()
{
	ASSERT(id);

	detail::state.deleteTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
	id = 0;
	size = {0, 0};
	samples = Samples::None;

	L.debug(R"(Multisample texture "%s" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void TextureMS<F>::resize(uvec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	bind();
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, +samples, +Format,
		_size.x, _size.y, GL_TRUE);
	size = _size;
}

template<PixelFmt F>
void TextureMS<F>::bind(TextureUnit const unit)
{
	ASSERT(id);

	detail::state.setTextureUnit(unit);
	detail::state.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
}

template<PixelFmt F>
void Renderbuffer<F>::create(char const* const _name, uvec2 const _size)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(Format != PixelFmt::None);

	glGenRenderbuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_RENDERBUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	resize(_size);

	L.debug(R"(Renderbuffer "%s" created)", name);
}

template<PixelFmt F>
void Renderbuffer<F>::destroy()
{
	ASSERT(id);

	detail::state.deleteRenderbuffer(id);
	id = 0;
	size = {0, 0};

	L.debug(R"(Renderbuffer "%s" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void Renderbuffer<F>::resize(uvec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	detail::state.bindRenderbuffer(id);
	glRenderbufferStorage(GL_RENDERBUFFER, +Format, _size.x, _size.y);
	size = _size;
}

template<PixelFmt F>
void RenderbufferMS<F>::create(char const* const _name, uvec2 const _size, Samples const _samples)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(Format != PixelFmt::None);
	ASSERT(+_samples >= 2);

	glGenRenderbuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_RENDERBUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	samples = _samples;
	resize(_size);

	L.debug(R"(Multisample renderbuffer "%s" created)", name);
}

template<PixelFmt F>
void RenderbufferMS<F>::destroy()
{
	ASSERT(id);

	detail::state.deleteRenderbuffer(id);
	id = 0;
	size = {0, 0};

	L.debug(R"(Multisample renderbuffer "%s" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void RenderbufferMS<F>::resize(uvec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	detail::state.bindRenderbuffer(id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, +samples, +Format,
		_size.x, _size.y);
	size = _size;
}

template<BufferTextureType T>
void BufferTexture<T>::create(char const* const _name, bool const dynamic)
{
	ASSERT(!id);
	ASSERT(_name);

	glGenTextures(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_TEXTURE, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;

	constexpr auto format = []() -> GLenum {
		if constexpr (std::is_same_v<T, f32>)
			return GL_R32F;
		if constexpr (std::is_same_v<T, vec2>)
			return GL_RG32F;
		if constexpr (std::is_same_v<T, vec4>)
			return GL_RGBA32F;
		if constexpr (std::is_same_v<T, u8>)
			return GL_R8;
		if constexpr (std::is_same_v<T, u8vec2>)
			return GL_RG8;
		if constexpr (std::is_same_v<T, u8vec4>)
			return GL_RGBA8;
		if constexpr (std::is_same_v<T, u32>)
			return GL_R32UI;
		if constexpr (std::is_same_v<T, uvec2>)
			return GL_RG32UI;
		if constexpr (std::is_same_v<T, uvec4>)
			return GL_RGBA32UI;
		if constexpr (std::is_same_v<T, i32>)
			return GL_R32I;
		if constexpr (std::is_same_v<T, ivec2>)
			return GL_RG32I;
		if constexpr (std::is_same_v<T, ivec4>)
			return GL_RGBA32I;
		if constexpr (std::is_same_v<T, mat4>)
			return GL_RGBA32F;
		L.fail("Unknown buffer texture type");
	}();

	storage.create(_name, dynamic);
	storage.bind();
	glBufferData(StorageBuffer::Target, 0, nullptr,
		dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW);
	bind(TextureUnit::_0);
	glTexBuffer(GL_TEXTURE_BUFFER, format, storage.id);

	L.debug(R"(Buffer texture "%s" created)", name);
}

template<BufferTextureType T>
void BufferTexture<T>::destroy()
{
	ASSERT(id);

	detail::state.deleteTexture(GL_TEXTURE_BUFFER, id);
	id = 0;
	size = {0, 0};
	storage.destroy();

	L.debug(R"(Buffer texture "%s" destroyed)", name);
	name = nullptr;
}

template<BufferTextureType T>
template<template<BufferTextureType, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, T, N>
void BufferTexture<T>::upload(Arr<T, N> data)
{
	storage.upload(data);
	size = {N, 1};
}

template<BufferTextureType T>
void BufferTexture<T>::bind(TextureUnit unit)
{
	ASSERT(id);

	detail::state.setTextureUnit(unit);
	detail::state.bindTexture(GL_TEXTURE_BUFFER, id);
}

template<GLSLType T>
void VertexArray::setAttribute(GLuint const index, VertexBuffer<T>& buffer, bool const instanced)
{
	ASSERT(index > 0 || index < attributes.size());
	if constexpr (std::is_same_v<T, mat4>)
		ASSERT(index + 3 < attributes.size());
	ASSERT(id);

	detail::setVaoAttribute<T>(*this, index, buffer, 0, instanced);
}

template<TriviallyCopyable T, GLSLType U>
void VertexArray::setAttribute(GLuint const index, VertexBuffer<T>& buffer, U T::*field,
	bool const instanced)
{
	ASSERT(index > 0 || index < attributes.size());
	if constexpr (std::is_same_v<U, mat4>)
		ASSERT(index + 3 < attributes.size());
	ASSERT(id);

	detail::setVaoAttribute<U>(*this, index, buffer, offset_of(field), instanced);
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

template<GLSLType T>
void Uniform<T>::setLocation(Shader const& shader, char const* const name)
{
	ASSERT(shader.id);
	ASSERT(name);

	location = glGetUniformLocation(shader.id, name);
	shaderId = shader.id;

	if (location == -1)
		L.warn(R"(Failed to get location for uniform "%s")", name);
}

template<GLSLType T>
void Uniform<T>::set(Type const _value)
{
	if (location == -1 || _value == value || shaderId == 0) return;

	detail::state.bindShader(shaderId);
	if constexpr (std::is_same_v<Type, float>)
		glUniform1f(location, _value);
	else if constexpr (std::is_same_v<Type, vec2>)
		glUniform2f(location, _value.x, _value.y);
	else if constexpr (std::is_same_v<Type, vec3>)
		glUniform3f(location, _value.x, _value.y, _value.z);
	else if constexpr (std::is_same_v<Type, vec4>)
		glUniform4f(location, _value.x, _value.y, _value.z, _value.w);
	else if constexpr (std::is_same_v<Type, int>)
		glUniform1i(location, _value);
	else if constexpr (std::is_same_v<Type, ivec2>)
		glUniform2i(location, _value.x, _value.y);
	else if constexpr (std::is_same_v<Type, ivec3>)
		glUniform3i(location, _value.x, _value.y, _value.z);
	else if constexpr (std::is_same_v<Type, ivec4>)
		glUniform4i(location, _value.x, _value.y, _value.z, _value.w);
	else if constexpr (std::is_same_v<Type, mat4>)
		glUniformMatrix4fv(location, 1, false, value_ptr(_value));
	else
		L.fail("Unknown uniform type");
	value = _value;
}

template<template<PixelFmt> typename T>
void Sampler<T>::setLocation(Shader const& shader, char const* const name, TextureUnit const _unit)
{
	ASSERT(shader.id);
	ASSERT(name);
	ASSERT(_unit != TextureUnit::None);

	location = glGetUniformLocation(shader.id, name);
	if (location == -1) {
		L.warn(R"(Failed to get location for sampler "%s")", name);
		return;
	}

	shader.bind();
	glUniform1i(location, +_unit - GL_TEXTURE0);
	unit = _unit;
}

template<template<PixelFmt> typename T>
template<PixelFmt F>
void Sampler<T>::set(T<F>& val)
{
	val.bind(unit);
}

template<BufferTextureType T>
void BufferSampler::set(BufferTexture<T>& val)
{
	val.bind(unit);
}

template<ShaderType T>
void Draw<T>::draw(Window* window)
{
	ASSERT(window || framebuffer || !params.viewport.zero());
	ASSERT(shader);
	// Ensure the element buffer format is GL_UNSIGNED_INT
	static_assert(std::is_same_v<ElementBuffer::Type, u32>);

	bool const instanced = (instances > 1);
	bool const indexed = (vertexarray && vertexarray->elements);
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
		params.viewport.size = framebuffer? framebuffer->size() : window->size.load();
		params.set();
		params.viewport.size = {0, 0};
	} else {
		params.set();
	}
	shader->bind();
	if (vertexarray)
		vertexarray->bind();
	if (framebuffer)
		framebuffer->bind();
	else
		Framebuffer::unbind();

	if (!instanced && !indexed)
		glDrawArrays(+mode, offset, vertices);
	if (instanced && !indexed)
		glDrawArraysInstanced(+mode, offset, vertices, instances);
	if (!instanced && indexed)
		glDrawElements(+mode, vertices, GL_UNSIGNED_INT,
			reinterpret_cast<GLvoid*>(offset * sizeof(u32)));
	if (instanced && indexed)
		glDrawElementsInstanced(+mode, vertices, GL_UNSIGNED_INT,
			reinterpret_cast<GLvoid*>(offset * sizeof(u32)), instances);
}

}
