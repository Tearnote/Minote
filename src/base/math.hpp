#pragma once

#include <initializer_list>
#include <concepts>
#include <numbers>
#include "base/container/array.hpp"
#include "base/concepts.hpp"
#include "base/types.hpp"

namespace minote::base {

//=== Constants

constexpr auto Tau = std::numbers::pi_v<f32> * 2;

//=== Scalar operations

using std::min;
using std::max;
using std::abs;
using std::sin;
using std::cos;
using std::tan;

template<arithmetic T, std::floating_point Prec = f32>
constexpr auto radians(T deg) -> Prec { return Prec(deg) * Tau / Prec(360); }

// True modulo operation (as opposed to remainder, which is operator% in C++.)
// The result is always positive and does not flip direction at zero.
// Example:
//  5 mod 4 = 1
//  4 mod 4 = 0
//  3 mod 4 = 3
//  2 mod 4 = 2
//  1 mod 4 = 1
//  0 mod 4 = 0
// -1 mod 4 = 3
// -2 mod 4 = 2
// -3 mod 4 = 1
// -4 mod 4 = 0
// -5 mod 4 = 3
template<std::integral T>
constexpr auto tmod(T num, T div) { return num % div + (num % div < 0) * div; }

template<arithmetic T>
constexpr auto clamp(T val, T vmin, T vmax) -> T {
	
	return max(vmin, min(val, vmax));
	
}

//=== Compound types

// Generic math vector, of any dimension between 2 to 4 and any underlying type
template<usize Dim, arithmetic T>
struct vec {
	
	static_assert(Dim >= 2 && Dim <= 4, "Vectors need to have 2, 3 or 4 components");
	
	//=== Creation
	
	constexpr vec() = default;
	explicit constexpr vec(T fillVal) { fill(fillVal); }
	constexpr vec(std::initializer_list<T>);
	
	//=== Conversions
	
	// Type cast
	template<arithmetic U>
	requires (!std::same_as<T, U>)
	explicit constexpr vec(vec<Dim, U> const&);
	
	// Dimension downcast
	template<usize N>
	requires (N > Dim)
	explicit constexpr vec(vec<N, T> const&);
	
	// Dimension upcast
	template<usize N>
	requires (N < Dim)
	constexpr vec(vec<N, T> const&, T fill);
	
	//=== Member access
	
	constexpr auto at(usize n) -> T& { return arr[n]; }
	constexpr auto at(usize n) const -> T { return arr[n]; }
	
	constexpr auto operator[](usize n) -> T& { return at(n); }
	constexpr auto operator[](usize n) const -> T { return at(n); }
	
	constexpr auto x() -> T& { static_assert(Dim >= 1); return arr[0]; }
	constexpr auto x() const -> T { static_assert(Dim >= 1); return arr[0]; }
	constexpr auto y() -> T& { static_assert(Dim >= 2); return arr[1]; }
	constexpr auto y() const -> T { static_assert(Dim >= 2); return arr[1]; }
	constexpr auto z() -> T& { static_assert(Dim >= 3); return arr[2]; }
	constexpr auto z() const -> T { static_assert(Dim >= 3); return arr[2]; }
	constexpr auto w() -> T& { static_assert(Dim >= 4); return arr[3]; }
	constexpr auto w() const -> T { static_assert(Dim >= 4); return arr[3]; }
	
	constexpr auto r() -> T& { return x(); }
	constexpr auto r() const -> T { return x(); }
	constexpr auto g() -> T& { return y(); }
	constexpr auto g() const -> T { return y(); }
	constexpr auto b() -> T& { return z(); }
	constexpr auto b() const -> T { return z(); }
	constexpr auto a() -> T& { return w(); }
	constexpr auto a() const -> T { return w(); }
	
	constexpr auto u() -> T& { return x(); }
	constexpr auto u() const -> T { return x(); }
	constexpr auto v() -> T& { return y(); }
	constexpr auto v() const -> T { return y(); }
	constexpr auto s() -> T& { return z(); }
	constexpr auto s() const -> T { return z(); }
	constexpr auto t() -> T& { return w(); }
	constexpr auto t() const -> T { return w(); }
	
	constexpr auto fill(T val) { arr.fill(val); }
	
	//=== Operations
	
	// Component-wise arithmetic
	
	constexpr auto operator+=(vec<Dim, T> const&) -> vec<Dim, T>&;
	constexpr auto operator-=(vec<Dim, T> const&) -> vec<Dim, T>&;
	constexpr auto operator*=(vec<Dim, T> const&) -> vec<Dim, T>&;
	constexpr auto operator/=(vec<Dim, T> const&) -> vec<Dim, T>&;
	
	// Scalar arithmetic
	
	constexpr auto operator*=(T) -> vec<Dim, T>&;
	constexpr auto operator/=(T) -> vec<Dim, T>&;
	
private:
	
	sarray<T, Dim> arr;
	
};

// Binary vector operations

template<usize Dim, arithmetic T>
constexpr auto operator+(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, arithmetic T>
constexpr auto operator-(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, arithmetic T>
constexpr auto operator*(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, arithmetic T>
constexpr auto operator/(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, arithmetic T>
constexpr auto operator==(vec<Dim, T> const&, vec<Dim, T> const&) -> bool;

template<usize Dim, arithmetic T>
constexpr auto min(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, arithmetic T>
constexpr auto max(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, arithmetic T>
constexpr auto dot(vec<Dim, T> const&, vec<Dim, T> const&) -> T;

template<arithmetic T>
constexpr auto cross(vec<3, T> const&, vec<3, T> const&) -> vec<3, T>;

// Binary scalar operations

template<usize Dim, arithmetic T>
constexpr auto operator*(vec<Dim, T> const&, T) -> vec<Dim, T>;
template<usize Dim, arithmetic T>
constexpr auto operator*(T left, vec<Dim, T> const& right) -> vec<Dim, T> { return right * left; }

template<usize Dim, arithmetic T>
constexpr auto operator/(vec<Dim, T> const&, T) -> vec<Dim, T>;
template<usize Dim, arithmetic T>
constexpr auto operator/(T left, vec<Dim, T> const& right) -> vec<Dim, T> { return right / left; }

// Unary vector operations

template<usize Dim, std::floating_point T>
constexpr auto abs(vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, std::floating_point T>
constexpr auto length(vec<Dim, T> const& v) -> T { static_assert(std::is_floating_point_v<T>); return std::sqrt(length2(v)); }

template<usize Dim, std::floating_point T>
constexpr auto length2(vec<Dim, T> const& v) -> T { return dot(v, v); }

template<usize Dim, std::floating_point T>
constexpr auto isUnit(vec<Dim, T> const& v) -> bool { return (abs(length2(v) - 1) < (1.0 / 16.0)); }

template<usize Dim, std::floating_point T>
constexpr auto normalize(vec<Dim, T> const&) -> vec<Dim, T>;

//=== Aliases

using vec2 = vec<2, f32>;
using vec3 = vec<3, f32>;
using vec4 = vec<4, f32>;
using ivec2 = vec<2, i32>;
using ivec3 = vec<3, i32>;
using ivec4 = vec<4, i32>;
using uvec2 = vec<2, u32>;
using uvec3 = vec<3, u32>;
using uvec4 = vec<4, u32>;
using u8vec2 = vec<2, u8>;
using u8vec3 = vec<3, u8>;
using u8vec4 = vec<4, u8>;
using u16vec2 = vec<2, u16>;
using u16vec3 = vec<3, u16>;
using u16vec4 = vec<4, u16>;

// Generic matrix type, of order 3 or 4, and any floating-point precision
template<usize Dim, std::floating_point Prec>
struct mat {
	
	using col_t = vec<Dim, Prec>;
	
	static_assert(Dim >= 3 && Dim <= 4, "Matrices need to have order 3x3 or 4x4");
	
	//=== Creation
	
	constexpr mat() = default;
	constexpr mat(std::initializer_list<Prec>);
	constexpr mat(std::initializer_list<col_t> list) { std::copy(list.begin(), list.end(), arr.begin()); }
	static constexpr auto identity() -> mat<Dim, Prec>;
	static constexpr auto translate(vec<3, Prec> shift) -> mat<Dim, Prec>;
	static constexpr auto rotate(vec<3, Prec> axis, Prec angle) -> mat<Dim, Prec>;
	static constexpr auto scale(vec<3, Prec> scale) -> mat<Dim, Prec>;
	static constexpr auto scale(Prec scale) -> mat<Dim, Prec>;
	
	//=== Conversion
	
	// Dimension cast
	template<usize N>
	requires (N != Dim)
	explicit constexpr mat(mat<N, Prec> const&);
	
	//=== Member access
	
	constexpr auto at(usize x, usize y) -> Prec& { return arr[x][y]; }
	constexpr auto at(usize x, usize y) const -> Prec { return arr[x][y]; }
	
	constexpr auto operator[](usize x) -> col_t& { return arr[x]; }
	constexpr auto operator[](usize x) const -> col_t const& { return arr[x]; }
	
	constexpr auto fill(Prec val) { for (auto& col: arr) col.fill(val); }
	
private:
	
	sarray<col_t, Dim> arr;
	
};

using mat3 = mat<3, f32>;
using mat4 = mat<4, f32>;

//=== Operations

// Binary matrix operations

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const&, mat<Dim, Prec> const&) -> mat<Dim, Prec>;

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const&, vec<Dim, Prec> const&) -> vec<Dim, Prec>;

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const&, Prec) -> mat<Dim, Prec>;
template<usize Dim, std::floating_point Prec>
constexpr auto operator*(Prec left, mat<Dim, Prec> const& right) -> mat<Dim, Prec> { return right * left; }

template<usize Dim, std::floating_point Prec>
constexpr auto operator/(mat<Dim, Prec> const&, Prec) -> mat<Dim, Prec>;

// Unary matrix operations

template<usize Dim, std::floating_point Prec>
constexpr auto transpose(mat<Dim, Prec> const&) -> mat<Dim, Prec>;

template<usize Dim, std::floating_point Prec>
constexpr auto inverse(mat<Dim, Prec> const&) -> mat<Dim, Prec>;

// Specialized matrix generators

template<std::floating_point Prec = f32>
constexpr auto look(vec<3, Prec> pos, vec<3, Prec> dir, vec<3, Prec> up) -> mat<4, Prec>;

template<std::floating_point Prec = f32>
constexpr auto perspective(Prec vFov, Prec aspectRatio, Prec zNear) -> mat<4, Prec>;

//=== Conversion literals

namespace literals {

constexpr float operator""_m(unsigned long long int val) { return float(val) * 0.001f; }
constexpr float operator""_m(long double val) { return float(val) * 0.001f; }

constexpr float operator""_km(unsigned long long int val) { return float(val); }
constexpr float operator""_km(long double val) { return float(val); }

constexpr float operator""_deg(unsigned long long int val) { return radians(float(val)); }
constexpr float operator""_deg(long double val) { return radians(float(val)); }

}

}

#include "base/math.tpp"
