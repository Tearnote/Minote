#include "sys/opengl/vertexarray.hpp"

#include <cstring>
#include "glad/glad.h"
#include "base/util.hpp"
#include "base/log.hpp"
#include "sys/opengl/state.hpp"

namespace minote {

void VertexArray::create(char const* const _name)
{
	DASSERT(!id);
	DASSERT(_name);

	glGenVertexArrays(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_VERTEX_ARRAY, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	attributes.fill(false);

	L.debug(R"(Vertex array "{}" created)", name);
}

void VertexArray::destroy()
{
	DASSERT(id);

	detail::state.deleteVertexArray(id);
	id = 0;

	L.debug(R"(Vertex array "{}" destroyed)", name);
	name = nullptr;
}

void VertexArray::bind()
{
	DASSERT(id);

	detail::state.bindVertexArray(id);
}

}
