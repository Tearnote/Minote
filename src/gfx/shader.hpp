#pragma once

#include <utility>
#include <vector>
#include "incbin.h"
#include "types.hpp"

#define GET_SHADER(symbol) \
	INCBIN_EXTERN(symbol)

#define ADD_SHADER(pipelineCI, symbol, filename) \
	do { \
		auto datavec = std::vector(reinterpret_cast<uint const*>(g_##symbol##_data), reinterpret_cast<uint const*>(g_##symbol##_end)); \
		(pipelineCI).add_spirv(std::move(datavec), (filename)); \
	} while (false)
