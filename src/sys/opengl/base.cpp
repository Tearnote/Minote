#include "sys/opengl/base.hpp"

#include "base/util.hpp"
#include "base/log.hpp"

namespace minote {

GLObject::~GLObject()
{
#ifndef NDEBUG
	if (id)
		L.warn(R"(OpenGL object "%s" was never destroyed)", stringOrNull(name));
#endif //NDEBUG
}

}
