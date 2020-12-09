// Minote - sys/opengl/shader.hpp
// Type-safe wrapper for OpenGL shader programs and their uniforms and samplers

#pragma once

#include "glad/glad.h"
#include "base/hashmap.hpp"
#include "base/string.hpp"
#include "base/math.hpp"
#include "base/util.hpp"

namespace minote {

struct Shader {

	struct Uniform {

		Uniform(Shader& parent, string_view name, u32 type, size_t size);

	private:

		variant<f32, vec2, vec3, vec4> value;
		Shader& parent;
		i32 location;
		size_t size;

	};

	Shader(string_view name, string_view vertSrc, string_view fragSrc);

	~Shader();

	// Bind the shader program to OpenGL state, causing all future draws to invoke this shader.
	void bind() const;

private:

	u32 id;
	string name;
	hashmap<ID, Uniform> uniforms;

};

}
