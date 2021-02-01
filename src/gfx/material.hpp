#pragma once

#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

enum struct Pass: u32 {
	None = 0,
	Flat = 1,
	Phong = 2,
};

struct Material {

	float ambient;
	float diffuse;
	float specular;
	float shine;

};

}
