#include "sys/opengl/base.hpp"

#include "base/log.hpp"

namespace minote {

GLObject::~GLObject()
{
#ifndef NDEBUG
	if (id)
		L.warn(R"(OpenGL object "{}" was never destroyed)", name);
#endif //NDEBUG
}

}
