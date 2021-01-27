#pragma once

#include "base/util.hpp"

namespace minote {

using namespace base;

template<size_t W, size_t H>
auto Grid<W, H>::get(glm::ivec2 position) const -> std::optional<Mino4> {
	if (position.x < 0 || position.x >= i32(Width) || position.y < 0)
		return Mino4::Garbage;
	if (position.y >= i32(Height))
		return std::nullopt;
	return m_grid[position.y * Width + position.x];
}

template<size_t W, size_t H>
void Grid<W, H>::set(glm::ivec2 position, Mino4 value) {
	if (position.x < 0 || position.x >= i32(Width) ||
		position.y < 0 || position.y >= i32(Height))
		return;
	m_grid[position.y * Width + position.x] = value;
}

template<size_t W, size_t H>
auto Grid<W, H>::stackHeight() -> size_t {
	for (auto y: rnrange_inc(Height - 1, 0_zu))
		for (auto x: nrange(0_zu, Width)) {
			if (get({x, y}))
				return y + 1;
		}
	return 0;
}

template<size_t W, size_t H>
auto Grid<W, H>::overlapsPiece(glm::ivec2 position, Piece4 const& piece) -> bool {
	for (auto block: piece) {
		if (get(position + block))
			return true;
	}
	return false;
}

}
