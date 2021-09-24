// Some algorithms are adapted from GLM code:
// https://github.com/g-truc/glm

#include <algorithm>
#include <cassert>
#include "base/util.hpp"

namespace minote::base {

using namespace base::literals;

template<usize Dim, arithmetic T>
constexpr vec<Dim, T>::vec(std::initializer_list<T> _list) {
	
	assert(_list.size() == m_arr.size());
	std::copy(_list.begin(), _list.end(), m_arr.begin());
	
}

template<usize Dim, arithmetic T>
template<arithmetic U>
requires (!std::same_as<T, U>)
constexpr vec<Dim, T>::vec(vec<Dim, U> const& _other) {
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] = T(_other[i]);
	
}

template<usize Dim, arithmetic T>
template<usize N>
requires (N > Dim)
constexpr vec<Dim, T>::vec(vec<N, T> const& _other) {
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] = _other[i];
	
}

template<usize Dim, arithmetic T>
template<usize N>
requires (N < Dim)
constexpr vec<Dim, T>::vec(vec<N, T> const& _other, T _fill) {
	
	m_arr.fill(_fill);
	for (auto i: iota(0_zu, N))
		m_arr[i] = _other[i];
	
}

template<usize Dim, arithmetic T>
constexpr auto vec<Dim, T>::operator+=(vec<Dim, T> const& _other) -> vec<Dim, T>& {
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] += _other[i];
	
	return *this;
	
}

template<usize Dim, arithmetic T>
constexpr auto vec<Dim, T>::operator-=(vec<Dim, T> const& _other) -> vec<Dim, T>& {
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] -= _other[i];
	
	return *this;
	
}

template<usize Dim, arithmetic T>
constexpr auto vec<Dim, T>::operator*=(vec<Dim, T> const& _other) -> vec<Dim, T>& {
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] *= _other[i];
	
	return *this;
	
}

template<usize Dim, arithmetic T>
constexpr auto vec<Dim, T>::operator/=(vec<Dim, T> const& _other) -> vec<Dim, T>& {
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] /= _other[i];
	
	return *this;
	
}

template<usize Dim, arithmetic T>
constexpr auto vec<Dim, T>::operator*=(T _other) -> vec<Dim, T>& {
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] *= _other;
	
	return *this;
	
}

template<usize Dim, arithmetic T>
constexpr auto vec<Dim, T>::operator/=(T _other) -> vec<Dim, T>& {
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] /= _other;
	
	return *this;
	
}

template<usize Dim, arithmetic T>
constexpr auto vec<Dim, T>::operator<<=(T _other) -> vec<Dim, T>& {
	
	static_assert(std::is_integral_v<T>);
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] <<= _other;
	
	return *this;
	
}

template<usize Dim, arithmetic T>
constexpr auto vec<Dim, T>::operator>>=(T _other) -> vec<Dim, T>& {
	
	static_assert(std::is_integral_v<T>);
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] >>= _other;
	
	return *this;
	
}

template<usize Dim, arithmetic T>
constexpr auto operator+(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {
	
	auto result = _left;
	result += _right;
	return result;
	
}

template<usize Dim, arithmetic T>
constexpr auto operator-(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {
	
	auto result = _left;
	result -= _right;
	return result;
	
}

template<usize Dim, arithmetic T>
constexpr auto operator*(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {
	
	auto result = _left;
	result *= _right;
	return result;
	
}

template<usize Dim, arithmetic T>
constexpr auto operator/(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {
	
	auto result = _left;
	result /= _right;
	return result;
	
}

template<usize Dim, arithmetic T>
constexpr auto operator==(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> bool {
	
	for (auto i: iota(0_zu, Dim))
		if (_left[i] != _right[i])
			return false;
	
	return true;
	
}

template<usize Dim, arithmetic T>
constexpr auto min(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {
	
	auto result = vec<Dim, T>();
	
	for (auto i: iota(0_zu, Dim))
		result[i] = min(_left[i], _right[i]);
	
	return result;
	
}

template<usize Dim, arithmetic T>
constexpr auto max(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {
	
	auto result = vec<Dim, T>();
	
	for (auto i: iota(0_zu, Dim))
		result[i] = max(_left[i], _right[i]);
	
	return result;
	
}

template<usize Dim, arithmetic T>
constexpr auto dot(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> T {
	
	auto result = T(0);
	
	for (auto i: iota(0_zu, Dim))
		result += _left[i] * _right[i];
	
	return result;
	
}

template<arithmetic T>
constexpr auto cross(vec<3, T> const& _left, vec<3, T> const& _right) -> vec<3, T> {
	
	return vec<3, T>{
		_left[1]*_right[2] - _right[1]*_left[2],
		_left[2]*_right[0] - _right[2]*_left[0],
		_left[0]*_right[1] - _right[0]*_left[1]};
	
}

template<usize Dim, arithmetic T>
constexpr auto operator*(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {
	
	auto result = _left;
	result *= _right;
	return result;
	
}

template<usize Dim, arithmetic T>
constexpr auto operator/(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {
	
	auto result = _left;
	result /= _right;
	return result;
	
}

template<usize Dim, integral T>
constexpr auto operator<<(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {
	
	auto result = _left;
	result <<= _right;
	return result;
	
}

template<usize Dim, integral T>
constexpr auto operator>>(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {
	
	auto result = _left;
	result >>= _right;
	return result;
	
}

template<usize Dim, std::floating_point T>
constexpr auto abs(vec<Dim, T> const& _vec) -> vec<Dim, T> {
	
	auto result = vec<Dim, T>();
	
	for (auto i: iota(0_zu, Dim))
		result[i] = abs(_vec[i]);
	
	return result;
	
}

template<usize Dim, std::floating_point T>
constexpr auto normalize(vec<Dim, T> const& _vec) -> vec<Dim, T> {
	
	if constexpr (Dim == 4) {
		
		auto norm = normalize(vec<3, T>(_vec));
		return vec<Dim, T>(norm, _vec[3]);
		
	}
	
	return _vec / length(_vec);
	
}

template<usize Dim, std::floating_point Prec>
constexpr mat<Dim, Prec>::mat(std::initializer_list<Prec> _list) {
	
	assert(_list.size() == Dim * Dim);
	
	auto it = _list.begin();
	for (auto x: iota(0_zu, Dim))
		for (auto y: iota(0_zu, Dim)) {
			
			m_arr[x][y] = *it;
			it += 1;
			
		}
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::identity() -> mat<Dim, Prec> {
	
	auto result = mat<Dim, Prec>();
	result.fill(0);
	
	for (auto i: iota(0_zu, Dim))
		result[i][i] = 1;
	
	return result;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::translate(vec<3, Prec> _shift) -> mat<Dim, Prec> {
	
	static_assert(Dim == 4, "Translation matrix requires order of 4");
	
	auto result = mat<Dim, Prec>::identity();
	result[3][0] = _shift[0];
	result[3][1] = _shift[1];
	result[3][2] = _shift[2];
	return result;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::rotate(vec<3, Prec> _axis, Prec _angle) -> mat<Dim, Prec> {
	
	assert(isUnit(_axis));
	
	auto sinT = sin(_angle);
	auto cosT = cos(_angle);
	auto temp = _axis * (Prec(1) - cosT);
	
	auto result = mat<Dim, Prec>::identity();
	
	result[0][0] = cosT + temp[0] * _axis[0];
	result[0][1] = temp[0] * _axis[1] + sinT * _axis[2];
	result[0][2] = temp[0] * _axis[2] - sinT * _axis[1];
	
	result[1][0] = temp[1] * _axis[0] - sinT * _axis[2];
	result[1][1] = cosT + temp[1] * _axis[1];
	result[1][2] = temp[1] * _axis[2] + sinT * _axis[0];
	
	result[2][0] = temp[2] * _axis[0] + sinT * _axis[1];
	result[2][1] = temp[2] * _axis[1] - sinT * _axis[0];
	result[2][2] = cosT + temp[2] * _axis[2];
	
	return result;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::scale(vec<3, Prec> _scale) -> mat<Dim, Prec> {
	
	auto result = mat<Dim, Prec>::identity();
	
	for (auto i: iota(0_zu, 3_zu))
		result[i][i] = _scale[i];
	
	return result;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::scale(Prec _scale) -> mat<Dim, Prec> {
	
	auto result = mat<Dim, Prec>::identity();
	
	for (auto i: iota(0_zu, 3_zu))
		result[i][i] = _scale;
	
	return result;
	
}

template<usize Dim, std::floating_point Prec>
template<arithmetic U>
requires (!std::same_as<Prec, U>)
constexpr mat<Dim, Prec>::mat(mat<Dim, U> const& _other) {
	
	for (auto x: iota(0_zu, Dim))
		for (auto y: iota(0_zu, Dim))
			m_arr[x][y] = _other[x][y];
	
}

template<usize Dim, std::floating_point Prec>
template<usize N>
requires (N != Dim)
constexpr mat<Dim, Prec>::mat(mat<N, Prec> const& _other) {
	
	if constexpr (Dim > N)
		*this = identity();
	
	constexpr auto Smaller = std::min(Dim, N);
	for (auto x: iota(0_zu, Smaller))
		for (auto y: iota(0_zu, Smaller))
			m_arr[x][y] = _other[x][y];
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::operator*=(Prec _other) -> mat<Dim, Prec>& {
	
	for (auto x: iota(0_zu, Dim))
		for (auto y: iota(0_zu, Dim))
			m_arr[x][y] *= _other;
	
	return *this;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::operator/=(Prec _other) -> mat<Dim, Prec>& {
	
	for (auto x: iota(0_zu, Dim))
		for (auto y: iota(0_zu, Dim))
			m_arr[x][y] /= _other;
	
	return *this;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const& _left, mat<Dim, Prec> const& _right) -> mat<Dim, Prec> {
	
	static_assert(Dim == 3 || Dim == 4, "Unsupported matrix order for multiplication");
	
	auto result = mat<Dim, Prec>();
	
	if constexpr (Dim == 3) {
		
		result[0] = _left[0]*_right[0][0] + _left[1]*_right[0][1] + _left[2]*_right[0][2];
		result[1] = _left[0]*_right[1][0] + _left[1]*_right[1][1] + _left[2]*_right[1][2];
		result[2] = _left[0]*_right[2][0] + _left[1]*_right[2][1] + _left[2]*_right[2][2];
		
	} else if constexpr (Dim == 4) {
		
		result[0] = _left[0]*_right[0][0] + _left[1]*_right[0][1] + _left[2]*_right[0][2] + _left[3]*_right[0][3];
		result[1] = _left[0]*_right[1][0] + _left[1]*_right[1][1] + _left[2]*_right[1][2] + _left[3]*_right[1][3];
		result[2] = _left[0]*_right[2][0] + _left[1]*_right[2][1] + _left[2]*_right[2][2] + _left[3]*_right[2][3];
		result[3] = _left[0]*_right[3][0] + _left[1]*_right[3][1] + _left[2]*_right[3][2] + _left[3]*_right[3][3];
		
	}
	
	return result;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const& _left, vec<Dim, Prec> const& _right) -> vec<Dim, Prec> {
	
	auto leftT = transpose(_left);
	
	auto result = vec<Dim, Prec>();
	
	for (auto i: iota(0_zu, Dim))
		result[i] = dot(leftT[i], _right);
	
	return result;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const& _left, Prec _right) -> mat<Dim, Prec> {
	
	auto result = _left;
	result *= _right;
	return result;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto operator/(mat<Dim, Prec> const& _left, Prec _right) -> mat<Dim, Prec> {
	
	auto result = _left;
	result /= _right;
	return result;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto transpose(mat<Dim, Prec> const& _mat) -> mat<Dim, Prec> {
	
	auto result = mat<Dim, Prec>();
	
	for (auto x: iota(0_zu, Dim))
		for (auto y: iota(0_zu, Dim))
			result[x][y] = _mat[y][x];
	
	return result;
	
}

template<usize Dim, std::floating_point Prec>
constexpr auto inverse(mat<Dim, Prec> const& _mat) -> mat<Dim, Prec> {
	
	static_assert(Dim == 3 || Dim == 4, "Unsupported matrix order for inversion");
	
	if constexpr (Dim == 3) {
		
		auto oneOverDeterminant = Prec(1) / (
			+ _mat[0][0] * (_mat[1][1]*_mat[2][2] - _mat[2][1]*_mat[1][2])
			- _mat[1][0] * (_mat[0][1]*_mat[2][2] - _mat[2][1]*_mat[0][2])
			+ _mat[2][0] * (_mat[0][1]*_mat[1][2] - _mat[1][1]*_mat[0][2]));
		
		auto result = mat<3, Prec>();
		
		result[0][0] = + (_mat[1][1]*_mat[2][2] - _mat[2][1]*_mat[1][2]) * oneOverDeterminant;
		result[1][0] = - (_mat[1][0]*_mat[2][2] - _mat[2][0]*_mat[1][2]) * oneOverDeterminant;
		result[2][0] = + (_mat[1][0]*_mat[2][1] - _mat[2][0]*_mat[1][1]) * oneOverDeterminant;
		result[0][1] = - (_mat[0][1]*_mat[2][2] - _mat[2][1]*_mat[0][2]) * oneOverDeterminant;
		result[1][1] = + (_mat[0][0]*_mat[2][2] - _mat[2][0]*_mat[0][2]) * oneOverDeterminant;
		result[2][1] = - (_mat[0][0]*_mat[2][1] - _mat[2][0]*_mat[0][1]) * oneOverDeterminant;
		result[0][2] = + (_mat[0][1]*_mat[1][2] - _mat[1][1]*_mat[0][2]) * oneOverDeterminant;
		result[1][2] = - (_mat[0][0]*_mat[1][2] - _mat[1][0]*_mat[0][2]) * oneOverDeterminant;
		result[2][2] = + (_mat[0][0]*_mat[1][1] - _mat[1][0]*_mat[0][1]) * oneOverDeterminant;
		
		return result;
		
	} else if constexpr (Dim == 4) {
		
		auto coef00 = _mat[2][2]*_mat[3][3] - _mat[3][2]*_mat[2][3];
		auto coef02 = _mat[1][2]*_mat[3][3] - _mat[3][2]*_mat[1][3];
		auto coef03 = _mat[1][2]*_mat[2][3] - _mat[2][2]*_mat[1][3];
		
		auto coef04 = _mat[2][1]*_mat[3][3] - _mat[3][1]*_mat[2][3];
		auto coef06 = _mat[1][1]*_mat[3][3] - _mat[3][1]*_mat[1][3];
		auto coef07 = _mat[1][1]*_mat[2][3] - _mat[2][1]*_mat[1][3];
		
		auto coef08 = _mat[2][1]*_mat[3][2] - _mat[3][1]*_mat[2][2];
		auto coef10 = _mat[1][1]*_mat[3][2] - _mat[3][1]*_mat[1][2];
		auto coef11 = _mat[1][1]*_mat[2][2] - _mat[2][1]*_mat[1][2];
		
		auto coef12 = _mat[2][0]*_mat[3][3] - _mat[3][0]*_mat[2][3];
		auto coef14 = _mat[1][0]*_mat[3][3] - _mat[3][0]*_mat[1][3];
		auto coef15 = _mat[1][0]*_mat[2][3] - _mat[2][0]*_mat[1][3];
		
		auto coef16 = _mat[2][0]*_mat[3][2] - _mat[3][0]*_mat[2][2];
		auto coef18 = _mat[1][0]*_mat[3][2] - _mat[3][0]*_mat[1][2];
		auto coef19 = _mat[1][0]*_mat[2][2] - _mat[2][0]*_mat[1][2];
		
		auto coef20 = _mat[2][0]*_mat[3][1] - _mat[3][0]*_mat[2][1];
		auto coef22 = _mat[1][0]*_mat[3][1] - _mat[3][0]*_mat[1][1];
		auto coef23 = _mat[1][0]*_mat[2][1] - _mat[2][0]*_mat[1][1];
		
		auto fac0 = vec<4, Prec>{coef00, coef00, coef02, coef03};
		auto fac1 = vec<4, Prec>{coef04, coef04, coef06, coef07};
		auto fac2 = vec<4, Prec>{coef08, coef08, coef10, coef11};
		auto fac3 = vec<4, Prec>{coef12, coef12, coef14, coef15};
		auto fac4 = vec<4, Prec>{coef16, coef16, coef18, coef19};
		auto fac5 = vec<4, Prec>{coef20, coef20, coef22, coef23};
		
		auto v0 = vec<4, Prec>{_mat[1][0], _mat[0][0], _mat[0][0], _mat[0][0]};
		auto v1 = vec<4, Prec>{_mat[1][1], _mat[0][1], _mat[0][1], _mat[0][1]};
		auto v2 = vec<4, Prec>{_mat[1][2], _mat[0][2], _mat[0][2], _mat[0][2]};
		auto v3 = vec<4, Prec>{_mat[1][3], _mat[0][3], _mat[0][3], _mat[0][3]};
		
		auto inv0 = vec<4, Prec>{v1*fac0 - v2*fac1 + v3*fac2};
		auto inv1 = vec<4, Prec>{v0*fac0 - v2*fac3 + v3*fac4};
		auto inv2 = vec<4, Prec>{v0*fac1 - v1*fac3 + v3*fac5};
		auto inv3 = vec<4, Prec>{v0*fac2 - v1*fac4 + v2*fac5};
		
		auto signA = vec<4, Prec>{+1, -1, +1, -1};
		auto signB = vec<4, Prec>{-1, +1, -1, +1};
		auto inv = mat<4, Prec>{inv0*signA, inv1*signB, inv2*signA, inv3*signB};
		
		auto row0 = vec<4, Prec>{inv[0][0], inv[1][0], inv[2][0], inv[3][0]};
		
		auto dot0 = _mat[0] * row0;
		auto dot1 = (dot0.x() + dot0.y()) + (dot0.z() + dot0.w());
		
		auto oneOverDeterminant = Prec(1) / dot1;
		
		return inv * oneOverDeterminant;
		
	}
	
}

template<std::floating_point Prec>
constexpr auto look(vec<3, Prec> _pos, vec<3, Prec> _dir, vec<3, Prec> _up) -> mat<4, Prec> {
	
	assert(isUnit(_dir));
	assert(isUnit(_up));
	
	auto result = mat<4, Prec>::identity();
	
	auto s = normalize(cross(_up, _dir));
	auto u = cross(_dir, s);
	
	result[0][0] = s[0];
	result[1][0] = s[1];
	result[2][0] = s[2];
	result[0][1] = u[0];
	result[1][1] = u[1];
	result[2][1] = u[2];
	result[0][2] = _dir[0];
	result[1][2] = _dir[1];
	result[2][2] = _dir[2];
	result[3][0] = -dot(s, _pos);
	result[3][1] = -dot(u, _pos);
	result[3][2] = -dot(_dir, _pos);
	
	return result;
	
}

template<std::floating_point Prec>
constexpr auto perspective(Prec _vFov, Prec _aspectRatio, Prec _zNear) -> mat<4, Prec> {
	
	auto range = tan(_vFov / Prec(2)) * _zNear;
	auto left = -range * _aspectRatio;
	auto right = range * _aspectRatio;
	auto bottom = -range;
	auto top = range;
	
	auto result = mat<4, Prec>();
	result.fill(0);
	
	result[0][0] = (Prec(2) * _zNear) / (right - left);
	result[1][1] = (Prec(2) * _zNear) / (top - bottom);
	result[2][3] = Prec(1);
	result[3][2] = Prec(2) * _zNear;
	
	return result;
	
}

template<std::floating_point Prec>
constexpr qua<Prec>::qua(std::initializer_list<Prec> _list) {
	
	assert(_list.size() == m_arr.size());
	std::copy(_list.begin(), _list.end(), m_arr.begin());
	
}

template<std::floating_point Prec>
template<usize N>
requires (N == 3 || N == 4)
constexpr qua<Prec>::qua(vec<N, Prec> const& _other) {
	
	m_arr[0] = 0;
	m_arr[1] = _other.x();
	m_arr[2] = _other.y();
	m_arr[3] = _other.z();
	
}

template<std::floating_point Prec>
constexpr auto qua<Prec>::angleAxis(Prec _angle, vec<3, Prec> _axis) -> qua<Prec> {
	
	assert(isUnit(_axis));
	
	auto halfAngle = _angle / Prec(2);
	auto sinHalfAngle = sin(halfAngle);
	
	return qua<Prec>{
		cos(halfAngle),
		sinHalfAngle * _axis[0],
		sinHalfAngle * _axis[1],
		sinHalfAngle * _axis[2]};
	
}

template<std::floating_point Prec>
template<arithmetic U>
requires (!std::same_as<Prec, U>)
constexpr qua<Prec>::qua(qua<U> const& _other) {
	
	for (auto i: iota(0_zu, m_arr.size()))
		m_arr[i] = _other[i];
	
}

template<std::floating_point Prec>
constexpr auto operator*(qua<Prec> const& _l, qua<Prec> const& _r) -> qua<Prec> {
	
	return qua<Prec>{
		-_l.x() * _r.x() - _l.y() * _r.y() - _l.z() * _r.z() + _l.w() * _r.w(),
		 _l.x() * _r.w() + _l.y() * _r.z() - _l.z() * _r.y() + _l.w() * _r.x(),
		-_l.x() * _r.z() + _l.y() * _r.w() + _l.z() * _r.x() + _l.w() * _r.y(),
		 _l.x() * _r.y() - _l.y() * _r.x() + _l.z() * _r.w() + _l.w() * _r.z()};
	
}

}
