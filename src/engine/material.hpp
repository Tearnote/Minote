#pragma once

#include "base/types.hpp"

namespace minote::engine {

enum struct Material: base::u32 {
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
