#pragma once

#include <initializer_list>
#include <numbers>
#include "base/container/array.hpp"
#include "base/concepts.hpp"
#include "base/types.hpp"

namespace minote::base {

//=== Constants

template<floating_point T>
constexpr auto Pi_v = std::numbers::pi_v<T>;
constexpr auto Pi = Pi_v<f32>;

template<floating_point T>
constexpr auto Tau_v = Pi_v<T> * T(2.0);
constexpr auto Tau = Tau_v<f32>;

//=== Scalar operations

using std::min;
using std::max;
using std::abs;
using std::pow;
using std::sqrt;
using std::sin;
using std::cos;
using std::tan;

// Degrees to radians conversion
template<arithmetic T, floating_point Prec = f32>
constexpr auto radians(T deg) -> Prec { return Prec(deg) * Tau_v<Prec> / Prec(360); }

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
template<integral T>
constexpr auto tmod(T num, T div) { return num % div + (num % div < 0) * div; }

// Classic GLSL-style scalar clamp
template<arithmetic T>
constexpr auto clamp(T val, T vmin, T vmax) -> T { return max(vmin, min(val, vmax)); }

//=== Compound types

// Generic math vector, of any dimension between 2 to 4 and any underlying type
template<usize Dim, arithmetic T>
struct vec {
	
	static_assert(Dim >= 2 && Dim <= 4, "Vectors need to have 2, 3 or 4 components");
	
	//=== Creation
	
	// Uninitialized init
	constexpr vec() = default;
	
	// Fill the vector with copies of the value
	explicit constexpr vec(T fillVal) { fill(fillVal); }
	
	// Create the vector with provided component values
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
	
	//=== Vector operations
	
	// Component-wise arithmetic
	
	constexpr auto operator+=(vec<Dim, T> const&) -> vec<Dim, T>&;
	constexpr auto operator-=(vec<Dim, T> const&) -> vec<Dim, T>&;
	constexpr auto operator*=(vec<Dim, T> const&) -> vec<Dim, T>&; // Component-wise
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
constexpr auto operator*(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>; // Component-wise

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

// Component-wise absolute value
template<usize Dim, floating_point T>
constexpr auto abs(vec<Dim, T> const&) -> vec<Dim, T>;

// Vector length as Euclidean distance
template<usize Dim, floating_point T>
constexpr auto length(vec<Dim, T> const& v) -> T {
	
	static_assert(std::is_floating_point_v<T>);
	return sqrt(length2(v));
	
}

// Square of vector length (faster to compute than length)
template<usize Dim, std::floating_point T>
constexpr auto length2(vec<Dim, T> const& v) -> T { return dot(v, v); }

// true if vector has the length of 1 (within reasonable epsilon)
template<usize Dim, std::floating_point T>
constexpr auto isUnit(vec<Dim, T> const& v) -> bool { return (abs(length2(v) - 1) < (1.0 / 16.0)); }

// Constructs a vector in the same direction but length 1
template<usize Dim, std::floating_point T>
constexpr auto normalize(vec<Dim, T> const&) -> vec<Dim, T>;

//=== GLSL-like vector aliases

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
template<usize Dim, floating_point Prec>
struct mat {
	
	using col_t = vec<Dim, Prec>;
	
	static_assert(Dim >= 3 && Dim <= 4, "Matrices need to have order 3x3 or 4x4");
	
	//=== Creation
	
	// Uninitialized init
	constexpr mat() = default;
	
	// Compose a matrix out of all component values, in column-major order
	constexpr mat(std::initializer_list<Prec>);
	
	// Compose a matrix out of column vectors
	constexpr mat(std::initializer_list<col_t> list) { std::copy(list.begin(), list.end(), arr.begin()); }
	
	// Create a matrix that is a no-op on multiplication
	static constexpr auto identity() -> mat<Dim, Prec>;
	
	// Classic translation, rotation and scale matrices for vector manipulation
	
	static constexpr auto translate(vec<3, Prec> shift) -> mat<Dim, Prec>; // Cannot be mat3
	static constexpr auto rotate(vec<3, Prec> axis, Prec angle) -> mat<Dim, Prec>;
	static constexpr auto scale(vec<3, Prec> scale) -> mat<Dim, Prec>;
	static constexpr auto scale(Prec scale) -> mat<Dim, Prec>;
	
	//=== Conversion
	
	// Type cast
	template<arithmetic U>
	requires (!std::same_as<Prec, U>)
	explicit constexpr mat(mat<Dim, U> const&);
	
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
	
	//=== Operations
	
	// Scalar arithmetic
	
	constexpr auto operator*=(Prec) -> mat<Dim, Prec>&; // Component-wise
	constexpr auto operator/=(Prec) -> mat<Dim, Prec>&; // Component-wise
	
private:
	
	sarray<col_t, Dim> arr;
	
};

// Binary matrix operations

template<usize Dim, floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const&, mat<Dim, Prec> const&) -> mat<Dim, Prec>;

template<usize Dim, floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const&, vec<Dim, Prec> const&) -> vec<Dim, Prec>;

template<usize Dim, floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const&, Prec) -> mat<Dim, Prec>; // Component-wise
template<usize Dim, floating_point Prec>
constexpr auto operator*(Prec left, mat<Dim, Prec> const& right) -> mat<Dim, Prec> { return right * left; } // Component-wise

template<usize Dim, floating_point Prec>
constexpr auto operator/(mat<Dim, Prec> const&, Prec) -> mat<Dim, Prec>; // Component-wise

// Unary matrix operations

// Creates a matrix with rows transposed with columns
template<usize Dim, floating_point Prec>
constexpr auto transpose(mat<Dim, Prec> const&) -> mat<Dim, Prec>;

// Creates a matrix that results in identity when multiplied with the original (slow!)
template<usize Dim, floating_point Prec>
constexpr auto inverse(mat<Dim, Prec> const&) -> mat<Dim, Prec>;

// Specialized matrix generators

// Variant of lookAt matrix. Dir is a unit vector of the camera direction.
// Dir and Up are both required to be unit vectors
template<floating_point Prec = f32>
constexpr auto look(vec<3, Prec> pos, vec<3, Prec> dir, vec<3, Prec> up) -> mat<4, Prec>;

// Creates a perspective matrix. The matrix uses inverted infinite depth:
// 1.0 at zNear, 0.0 at infinity.
template<floating_point Prec = f32>
constexpr auto perspective(Prec vFov, Prec aspectRatio, Prec zNear) -> mat<4, Prec>;

//=== GLSL-like matrix aliases

using mat3 = mat<3, f32>;
using mat4 = mat<4, f32>;

//=== Conversion literals

namespace literals {

consteval auto operator""_cm(unsigned long long int val) -> f32 { return f64(val) * 0.000'001; }
consteval auto operator""_cm(long double val) -> f32 { return f64(val) * 0.000'001; }

consteval auto operator""_m(unsigned long long int val) -> f32 { return f64(val) * 0.001; }
consteval auto operator""_m(long double val) -> f32 { return f64(val) * 0.001; }

consteval auto operator""_km(unsigned long long int val) -> f32 { return val; }
consteval auto operator""_km(long double val) -> f32 { return val; }

consteval auto operator""_deg(unsigned long long int val) -> f32 { return radians(f64(val)); }
consteval auto operator""_deg(long double val) -> f32 { return radians(val); }

}

}

#include "base/math.tpp"
