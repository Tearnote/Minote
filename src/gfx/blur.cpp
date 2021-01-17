#include "gfx/blur.hpp"

#include <algorithm>
#include <numeric>
#include <utility>
#include <cmath>
#include "base/zip_view.hpp"
#include "base/assert.hpp"

#ifdef DEBUG_GAUSS
#include <iostream>
#endif //DEBUG_GAUSS

namespace minote::gfx {

auto generateGaussParams(int radius) -> std::vector<GaussSample> {
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

	// Normalize the weights and create offsets
	std::vector<f32> weights;
	weights.reserve(level);
	auto sum = std::pow(2.0f, level - 1);
	for (f32 num: pascalRow)
		weights.push_back(num / sum);
	std::vector<f32> offsets;
	offsets.resize(level);
	std::iota(offsets.begin(), offsets.end(), -static_cast<f32>(level / 2));

	// Optimize the weights and offsets for linear sampling
	std::vector<f32> lWeights;
	std::vector<f32> lOffsets;
	lWeights.reserve(level);
	lOffsets.reserve(level);
	bool odd = radius % 2 == 1;
	u32 center = level / 2;
	for (size_t i = 0; i < center; i += 2) {
		f32 coeff1 = weights[i];
		f32 coeff2 = weights[i + 1];
		if (!odd && i + 1 == center)
			coeff2 /= 2.0f;
		f32 offs1 = offsets[i];
		f32 offs2 = offsets[i + 1];
		lWeights.push_back(coeff1 + coeff2);
		lOffsets.push_back((offs1 * coeff1 + offs2 * coeff2) / (coeff1 + coeff2));
	}
	if (odd) {
		lWeights.push_back(weights[center]);
		lOffsets.push_back(offsets[center]);
	}
	std::copy(lWeights.rbegin() + odd, lWeights.rend(), std::back_inserter(lWeights));
	std::transform(lOffsets.rbegin() + odd, lOffsets.rend(), std::back_inserter(lOffsets), [](auto n) {
		return -n;
	});

	// Zip up weights and offsets
	std::vector<GaussSample> result;
	result.reserve(lWeights.size());
	for (auto[weight, offset]: zip_view{lWeights, lOffsets}) {
		result.emplace_back(GaussSample{
			.weight = weight,
			.offset = offset,
		});
	}

#ifdef DEBUG_GAUSS
	std::cout << "Radius, level:\n";
	std::cout << radius << ", " << level << "\n";
	std::cout << "Integer coefficients:\n";
	for (auto num: pascalRow)
		std::cout << num << " ";
	std::cout << "\n";
	std::cout << "Normalized coefficients + offsets:\n";
	for (auto num: weights)
		std::cout << num << " ";
	std::cout << "\n";
	for (auto num: offsets)
		std::cout << num << " ";
	std::cout << "\n";
	std::cout << "Linear coefficients + offsets:\n";
	for (auto num: lWeights)
		std::cout << num << " ";
	std::cout << "\n";
	for (auto num: lOffsets)
		std::cout << num << " ";
	std::cout << "\n";
#endif //DEBUG_GAUSS
	return result;
}

}
