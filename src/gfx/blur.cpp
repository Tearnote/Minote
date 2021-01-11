#include "gfx/blur.hpp"

#include <algorithm>
#include <numeric>
#include <utility>
#include "base/assert.hpp"

namespace minote::gfx {

auto generateGaussParams(int radius) -> GaussParams {
	ASSERT(radius > 0);
	u32 const level = radius * 2 - 1;

	// Generate the matching row of the Pascal triangle
	std::vector<int> pascalRow;
	pascalRow.reserve(level);
	pascalRow.push_back(1);
	for (size_t i = 1; i < level; i += 1) {
		pascalRow.push_back(0);
		for (size_t j = pascalRow.size() - 1; j > 0; j -= 1) {
			pascalRow[j] += pascalRow[j - 1];
		}
	}

	// Normalize the coefficients and create offsets
	std::vector<f32> coefficients;
	coefficients.reserve(level);
	auto sum = static_cast<f32>(1 << (level - 1));
	for (f32 num: pascalRow)
		coefficients.push_back(num / sum);
	std::vector<f32> offsets;
	offsets.resize(level);
	std::iota(offsets.begin(), offsets.end(), -static_cast<f32>(level) / 2.0f);

	// Optimize the coefficients and offsets for linear sampling
	std::vector<f32> lCoefficients;
	std::vector<f32> lOffsets;
	lCoefficients.reserve(level);
	lOffsets.reserve(level);
	bool odd = radius % 2 == 1;
	u32 center = level / 2;
	for (size_t i = 0; i < center; i += 2) {
		f32 coeff1 = coefficients[i];
		f32 coeff2 = coefficients[i + 1];
		if (!odd && i + 1 == center)
			coeff2 /= 2.0f;
		f32 offs1 = offsets[i];
		f32 offs2 = offsets[i + 1];
		lCoefficients.push_back(coeff1 + coeff2);
		lOffsets.push_back((offs1 * coeff1 + offs2 * coeff2) / (coeff1 + coeff2));
	}
	if (odd) {
		lCoefficients.push_back(coefficients[center]);
		lOffsets.push_back(offsets[center]);
	}
	std::copy(lCoefficients.rbegin() + odd, lCoefficients.rend(), std::back_inserter(lCoefficients));
	std::transform(lOffsets.rbegin() + odd, lOffsets.rend(), std::back_inserter(lOffsets), [](auto n) {
		return -n;
	});

	return GaussParams{
		.coefficients = std::move(lCoefficients),
		.offsets = std::move(lOffsets),
	};
}

}
