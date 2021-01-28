#pragma once

#include <stdexcept>
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

enum struct Mino4: int {
	I,
	L,
	O,
	Z,
	T,
	J,
	S,
	ShapeCount,
///////////////////
	Garbage,
	Count,
};

enum struct Spin: int {
	_0,
	_90,
	_180,
	_270,
};

using Piece4 = std::array<glm::ivec2, 4>;

constexpr auto minoColor(Mino4 mino) {
	switch (mino) {
	case Mino4::I: return glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
	case Mino4::L: return glm::vec4{1.0f, .22f, 0.0f, 1.0f};
	case Mino4::O: return glm::vec4{1.0f, 1.0f, 0.0f, 1.0f};
	case Mino4::Z: return glm::vec4{0.0f, 1.0f, 0.0f, 1.0f};
	case Mino4::T: return glm::vec4{0.0f, 1.0f, 1.0f, 1.0f};
	case Mino4::J: return glm::vec4{0.0f, 0.0f, 1.0f, 1.0f};
	case Mino4::S: return glm::vec4{1.0f, 0.0f, 1.0f, 1.0f};
	case Mino4::Garbage: return glm::vec4{.22f, .22f, .22f, 1.0f};
	default: return glm::vec4{1.0f, 0.0f, 1.0f, 1.0f};
	}
}

constexpr auto spinClockwise(Spin s, int times = 1) {
	return Spin{tmod(+s - times, 4)};
}

constexpr auto spinCounterClockwise(Spin s, int times = 1) {
	return Spin{tmod(+s + times, 4)};
}

constexpr auto rotatePiece(Piece4 piece, Spin rotation) -> Piece4 {
	if (rotation == Spin::_0) return piece;

	Piece4 result = piece;
	repeat(+rotation, [&] {
		for (auto[src, dst]: zip_view{piece, result})
			dst = {-src.y, src.x};
	});
	return rotatePiece(result, Spin(+rotation - 1));
}

constexpr auto minoPiece(Mino4 mino, Spin spin = Spin::_0) -> Piece4 {
	auto const result = [=] {
		switch(mino) {
		case Mino4::I: return std::to_array<Piece4::value_type>({{-1, 0}, {0, 0}, {1, 0}, {2, 0}});
		case Mino4::L: return std::to_array<Piece4::value_type>({{-1, 0}, {0, 0}, {1, 0}, {-1, -1}});
		case Mino4::O: return std::to_array<Piece4::value_type>({{0, 0}, {1, 0}, {0, -1}, {1, -1}});
		case Mino4::Z: return std::to_array<Piece4::value_type>({{-1, 0}, {0, 0}, {0, -1}, {1, -1}});
		case Mino4::T: return std::to_array<Piece4::value_type>({{-1, 0}, {0, 0}, {1, 0}, {0, -1}});
		case Mino4::J: return std::to_array<Piece4::value_type>({{-1, 0}, {0, 0}, {1, 0}, {1, -1}});
		case Mino4::S: return std::to_array<Piece4::value_type>({{0, 0}, {1, 0}, {-1, -1}, {0, -1}});
		default: throw std::logic_error{"Wrong piece type"};
		}
	}();
	return rotatePiece(result, spin);
}

template<size_t W, size_t H>
struct Grid {

	static constexpr auto Width = W;
	static constexpr auto Height = H;

	[[nodiscard]]
	auto get(glm::ivec2 position) const -> std::optional<Mino4>;

	void set(glm::ivec2 position, Mino4 value);

	auto stackHeight() -> size_t;

	auto overlaps(glm::ivec2 position, Piece4 const& piece) -> bool;

	void stamp(glm::ivec2 position, Piece4 const& piece, Mino4 value);

	void eraseRow(i32 height);

private:

	std::array<std::optional<Mino4>, Width * Height> m_grid = {};

};

}

#include "mino.tpp"
