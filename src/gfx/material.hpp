#pragma once

#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

enum struct Material: u32 {
	Flat = 0,
	Phong = 1,
};

union MaterialData {

	struct FlatData {

		// nothing

	} flat;

	struct PhongData {

		float ambient;
		float diffuse;
		float specular;
		float shine;

	} phong;

};

}
