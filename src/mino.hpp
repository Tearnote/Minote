#pragma once

#include <optional>
#include <array>
#include <glm/vec2.hpp>
#include "base/zip_view.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/util.hpp"

namespace minote {

using namespace base;
using namespace base::literals;

enum struct Mino: int {
	I,
	L,
	O,
	Z,
	T,
	J,
	S,
	Garbage,
};

enum struct Spin: int {
	_0,
	_90,
	_180,
	_270,
};

template<size_t N>
using Piece = std::array<glm::ivec2, N>;

constexpr auto minoColor(Mino mino) {
	switch (mino) {
	case Mino::I: return glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
	case Mino::L: return glm::vec4{1.0f, .22f, 0.0f, 1.0f};
	case Mino::O: return glm::vec4{1.0f, 1.0f, 0.0f, 1.0f};
	case Mino::Z: return glm::vec4{0.0f, 1.0f, 0.0f, 1.0f};
	case Mino::T: return glm::vec4{0.0f, 1.0f, 1.0f, 1.0f};
	case Mino::J: return glm::vec4{0.0f, 0.0f, 1.0f, 1.0f};
	case Mino::S: return glm::vec4{1.0f, 0.0f, 1.0f, 1.0f};
	case Mino::Garbage: return glm::vec4{.22f, .22f, .22f, 1.0f};
	default: return glm::vec4{1.0f, 0.0f, 1.0f, 1.0f};
	}
}

constexpr auto spinClockwise(Spin s, int times = 1) {
	return Spin{tmod(+s - times, 4)};
}

constexpr auto spinCounterClockwise(Spin s, int times = 1) {
	return Spin{tmod(+s + times, 4)};
}

template<size_t N>
constexpr auto rotatePiece(Piece<N> piece, Spin rotation) -> Piece<N> {
	Piece<N> result = piece;
	repeat(+rotation, [=] {
		for (auto[src, dst]: zip_view{piece, result})
			dst = glm::ivec2{-src.y, src.x};
	});
	return result;
}

template<size_t W, size_t H>
struct Grid {

	static constexpr auto Width = W;
	static constexpr auto Height = H;

	[[nodiscard]]
	auto get(glm::ivec2 position) const -> std::optional<Mino>;

	void set(glm::ivec2 position, Mino value);

private:

	std::array<std::optional<Mino>, Width * Height> m_grid = {};

};

}

#include "mino.tpp"
