// Some algorithms adapted from GLM: https://github.com/g-truc/glm
// They are licensed under The Happy Bunny License (licenses/HappyBunny.txt)

#include <algorithm>
#include "util/verify.hpp"
#include "util/util.hpp"

namespace minote {

template<usize Dim, arithmetic T>
constexpr vec<Dim, T>::vec(std::initializer_list<T> _list) {
	
	ASSUME(_list.size() == m_arr.size());
	std::copy(_list.begin(), _list.end(), m_arr.begin());
	
}

template<usize Dim, arithmetic T>
template<arithmetic U>
requires (!same_as<T, U>)
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
constexpr auto vec<Dim, T>::operator%=(vec<Dim, T> const& _other) -> vec<Dim, T>& {
	
	static_assert(std::is_integral_v<T>);
	
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
constexpr auto vec<Dim, T>::operator%=(T _other) -> vec<Dim, T>& {
	
	static_assert(std::is_integral_v<T>);
	
	for (auto i: iota(0_zu, Dim))
		m_arr[i] %= _other;
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

template<usize Dim, integral T>
constexpr auto operator%(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {
	
	auto result = _left;
	result %= _right;
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
		_left[0]*_right[1] - _right[0]*_left[1],
	};
	
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
constexpr auto operator%(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {
	
	auto result = _left;
	result %= _right;
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

template<usize Dim, floating_point T>
constexpr auto abs(vec<Dim, T> const& _vec) -> vec<Dim, T> {
	
	auto result = vec<Dim, T>();
	for (auto i: iota(0_zu, Dim))
		result[i] = abs(_vec[i]);
	return result;
	
}

template<usize Dim, floating_point T>
constexpr auto normalize(vec<Dim, T> const& _vec) -> vec<Dim, T> {
	
	if constexpr (Dim == 4) {
		auto norm = normalize(vec<3, T>(_vec));
		return vec<Dim, T>(norm, _vec[3]);
	}
	return _vec / length(_vec);
	
}

template<floating_point Prec>
constexpr qua<Prec>::qua(std::initializer_list<Prec> _list) {
	
	ASSUME(_list.size() == m_arr.size());
	std::copy(_list.begin(), _list.end(), m_arr.begin());
	
}

template<floating_point Prec>
template<usize N>
requires (N == 3 || N == 4)
constexpr qua<Prec>::qua(vec<N, Prec> const& _other) {
	
	m_arr[0] = 0;
	m_arr[1] = _other.x();
	m_arr[2] = _other.y();
	m_arr[3] = _other.z();
	
}

template<floating_point Prec>
constexpr auto qua<Prec>::angleAxis(Prec _angle, vec<3, Prec> _axis) -> qua<Prec> {
	
	ASSUME(isUnit(_axis));
	
	auto halfAngle = _angle / Prec(2);
	auto sinHalfAngle = sin(halfAngle);
	return qua<Prec>{
		cos(halfAngle),
		sinHalfAngle * _axis[0],
		sinHalfAngle * _axis[1],
		sinHalfAngle * _axis[2],
	};
	
}

template<floating_point Prec>
template<arithmetic U>
requires (!same_as<Prec, U>)
constexpr qua<Prec>::qua(qua<U> const& _other) {
	
	for (auto i: iota(0_zu, m_arr.size()))
		m_arr[i] = _other[i];
	
}

template<floating_point Prec>
constexpr auto operator*(qua<Prec> const& _l, qua<Prec> const& _r) -> qua<Prec> {
	
	return qua<Prec>{
		-_l.x() * _r.x() - _l.y() * _r.y() - _l.z() * _r.z() + _l.w() * _r.w(),
		 _l.x() * _r.w() + _l.y() * _r.z() - _l.z() * _r.y() + _l.w() * _r.x(),
		-_l.x() * _r.z() + _l.y() * _r.w() + _l.z() * _r.x() + _l.w() * _r.y(),
		 _l.x() * _r.y() - _l.y() * _r.x() + _l.z() * _r.w() + _l.w() * _r.z(),
	};
	
}

template<usize Dim, floating_point Prec>
constexpr mat<Dim, Prec>::mat(std::initializer_list<Prec> _list) {
	
	ASSUME(_list.size() == Dim * Dim);
	
	auto it = _list.begin();
	for (auto y: iota(0_zu, Dim))
	for (auto x: iota(0_zu, Dim)) {
		at(x, y) = *it;
		it += 1;
	}
	
}

template<usize Dim, floating_point Prec>
constexpr mat<Dim, Prec>::mat(std::initializer_list<vec<Dim, Prec>> _list) {
	
	ASSUME(_list.size() == Dim);
	
	auto it = _list.begin();
	for (auto y: iota(0_zu, Dim)) {
		for (auto x: iota(0_zu, Dim))
			at(x, y) = it->at(x);
		it += 1;
	}
	
}

template<usize Dim, floating_point Prec>
constexpr auto mat<Dim, Prec>::identity() -> mat<Dim, Prec> {
	
	auto result = mat<Dim, Prec>();
	result.fill(0);
	for (auto i: iota(0_zu, Dim))
		result.at(i, i) = 1;
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto mat<Dim, Prec>::translate(vec<3, Prec> _shift) -> mat<Dim, Prec> {
	
	static_assert(Dim == 4, "Translation matrix requires order of 4");
	
	auto result = mat<Dim, Prec>::identity();
	result.at(0, 3) = _shift[0];
	result.at(1, 3) = _shift[1];
	result.at(2, 3) = _shift[2];
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto mat<Dim, Prec>::rotate(vec<3, Prec> _axis, Prec _angle) -> mat<Dim, Prec> {
	
	ASSUME(isUnit(_axis));
	
	auto sinT = sin(_angle);
	auto cosT = cos(_angle);
	auto temp = _axis * (Prec(1) - cosT);
	
	auto result = mat<Dim, Prec>::identity();
	
	result.at(0, 0) = cosT + temp[0] * _axis[0];
	result.at(1, 0) = temp[0] * _axis[1] + sinT * _axis[2];
	result.at(2, 0) = temp[0] * _axis[2] - sinT * _axis[1];
	
	result.at(0, 1) = temp[1] * _axis[0] - sinT * _axis[2];
	result.at(1, 1) = cosT + temp[1] * _axis[1];
	result.at(2, 1) = temp[1] * _axis[2] + sinT * _axis[0];
	
	result.at(0, 2) = temp[2] * _axis[0] + sinT * _axis[1];
	result.at(1, 2) = temp[2] * _axis[1] - sinT * _axis[0];
	result.at(2, 2) = cosT + temp[2] * _axis[2];
	
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto mat<Dim, Prec>::rotate(qua<Prec> _quat) -> mat<Dim, Prec> {
	
	auto result = mat<Dim, Prec>::identity();
	
	result.at(0, 0) = 1.0f - 2.0f * (_quat.y() * _quat.y() + _quat.z() * _quat.z());
	result.at(1, 0) =        2.0f * (_quat.x() * _quat.y() + _quat.z() * _quat.w());
	result.at(2, 0) =        2.0f * (_quat.x() * _quat.z() - _quat.y() * _quat.w());
	
	result.at(0, 1) =        2.0f * (_quat.x() * _quat.y() - _quat.z() * _quat.w());
	result.at(1, 1) = 1.0f - 2.0f * (_quat.x() * _quat.x() + _quat.z() * _quat.z());
	result.at(2, 1) =        2.0f * (_quat.y() * _quat.z() + _quat.x() * _quat.w());
	
	result.at(0, 2) =        2.0f * (_quat.x() * _quat.z() + _quat.y() * _quat.w());
	result.at(1, 2) =        2.0f * (_quat.y() * _quat.z() - _quat.x() * _quat.w());
	result.at(2, 2) = 1.0f - 2.0f * (_quat.x() * _quat.x() + _quat.y() * _quat.y());
	
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto mat<Dim, Prec>::scale(vec<3, Prec> _scale) -> mat<Dim, Prec> {
	
	auto result = mat<Dim, Prec>::identity();
	for (auto i: iota(0_zu, 3_zu))
		result.at(i, i) = _scale[i];
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto mat<Dim, Prec>::scale(Prec _scale) -> mat<Dim, Prec> {
	
	auto result = mat<Dim, Prec>::identity();
	for (auto i: iota(0_zu, 3_zu))
		result(i, i) = _scale;
	return result;
	
}

template<usize Dim, floating_point Prec>
template<arithmetic U>
requires (!same_as<Prec, U>)
constexpr mat<Dim, Prec>::mat(mat<Dim, U> const& _other) {
	
	for (auto x: iota(0_zu, Dim))
	for (auto y: iota(0_zu, Dim))
		at(x, y) = _other.at(x, y);
	
}

template<usize Dim, floating_point Prec>
template<usize N>
requires (N != Dim)
constexpr mat<Dim, Prec>::mat(mat<N, Prec> const& _other) {
	
	if constexpr (Dim > N)
		*this = identity();
	
	constexpr auto Smaller = min(Dim, N);
	for (auto x: iota(0_zu, Smaller))
	for (auto y: iota(0_zu, Smaller))
		at(x, y) = _other.at(x, y);
	
}

template<usize Dim, floating_point Prec>
constexpr auto mat<Dim, Prec>::operator*=(Prec _other) -> mat<Dim, Prec>& {
	
	for (auto x: iota(0_zu, Dim))
	for (auto y: iota(0_zu, Dim))
		at(x, y) *= _other;
	return *this;
	
}

template<usize Dim, floating_point Prec>
constexpr auto mat<Dim, Prec>::operator/=(Prec _other) -> mat<Dim, Prec>& {
	
	for (auto x: iota(0_zu, Dim))
	for (auto y: iota(0_zu, Dim))
		at(x, y) /= _other;
	return *this;
	
}

template<usize Dim, floating_point Prec>
constexpr auto mul(mat<Dim, Prec> const& _left, mat<Dim, Prec> const& _right) -> mat<Dim, Prec> {
	
	static_assert(Dim == 3 || Dim == 4, "Unsupported matrix order for multiplication");
	
	auto result = mat<Dim, Prec>();
	
	if constexpr (Dim == 3) {
		result[0] = _left[0]*_right.at(0, 0) + _left[1]*_right.at(0, 1) + _left[2]*_right.at(0, 2);
		result[1] = _left[0]*_right.at(1, 0) + _left[1]*_right.at(1, 1) + _left[2]*_right.at(1, 2);
		result[2] = _left[0]*_right.at(2, 0) + _left[1]*_right.at(2, 1) + _left[2]*_right.at(2, 2);
	} else if constexpr (Dim == 4) {
		result[0] = _left[0]*_right.at(0, 0) + _left[1]*_right.at(0, 1) + _left[2]*_right.at(0, 2) + _left[3]*_right.at(0, 3);
		result[1] = _left[0]*_right.at(1, 0) + _left[1]*_right.at(1, 1) + _left[2]*_right.at(1, 2) + _left[3]*_right.at(1, 3);
		result[2] = _left[0]*_right.at(2, 0) + _left[1]*_right.at(2, 1) + _left[2]*_right.at(2, 2) + _left[3]*_right.at(2, 3);
		result[3] = _left[0]*_right.at(3, 0) + _left[1]*_right.at(3, 1) + _left[2]*_right.at(3, 2) + _left[3]*_right.at(3, 3);
	}
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto mul(vec<Dim, Prec> const& _left, mat<Dim, Prec> const& _right) -> vec<Dim, Prec> {
	
	auto result = vec<Dim, Prec>();
	
	for (auto i: iota(0_zu, Dim))
		result[i] = dot(_left, _right[i]);
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const& _left, Prec _right) -> mat<Dim, Prec> {
	
	auto result = _left;
	result *= _right;
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto operator/(mat<Dim, Prec> const& _left, Prec _right) -> mat<Dim, Prec> {
	
	auto result = _left;
	result /= _right;
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto transpose(mat<Dim, Prec> const& _mat) -> mat<Dim, Prec> {
	
	auto result = mat<Dim, Prec>();
	
	for (auto x: iota(0_zu, Dim))
	for (auto y: iota(0_zu, Dim))
		result.at(x, y) = _mat.at(y, x);
	return result;
	
}

template<usize Dim, floating_point Prec>
constexpr auto inverse(mat<Dim, Prec> const& _mat) -> mat<Dim, Prec> {
	
	static_assert(Dim == 3 || Dim == 4, "Unsupported matrix order for inversion");
	
	if constexpr (Dim == 3) {
		
		auto oneOverDet = Prec(1) / (
			+ _mat[0][0] * (_mat[1][1] * _mat[2][2] - _mat[2][1] * _mat[1][2])
			- _mat[1][0] * (_mat[0][1] * _mat[2][2] - _mat[2][1] * _mat[0][2])
			+ _mat[2][0] * (_mat[0][1] * _mat[1][2] - _mat[1][1] * _mat[0][2]));
		
		auto result = mat<3, Prec>();
		result[0][0] = + (_mat[1][1] * _mat[2][2] - _mat[2][1] * _mat[1][2]) * oneOverDet;
		result[1][0] = - (_mat[1][0] * _mat[2][2] - _mat[2][0] * _mat[1][2]) * oneOverDet;
		result[2][0] = + (_mat[1][0] * _mat[2][1] - _mat[2][0] * _mat[1][1]) * oneOverDet;
		result[0][1] = - (_mat[0][1] * _mat[2][2] - _mat[2][1] * _mat[0][2]) * oneOverDet;
		result[1][1] = + (_mat[0][0] * _mat[2][2] - _mat[2][0] * _mat[0][2]) * oneOverDet;
		result[2][1] = - (_mat[0][0] * _mat[2][1] - _mat[2][0] * _mat[0][1]) * oneOverDet;
		result[0][2] = + (_mat[0][1] * _mat[1][2] - _mat[1][1] * _mat[0][2]) * oneOverDet;
		result[1][2] = - (_mat[0][0] * _mat[1][2] - _mat[1][0] * _mat[0][2]) * oneOverDet;
		result[2][2] = + (_mat[0][0] * _mat[1][1] - _mat[1][0] * _mat[0][1]) * oneOverDet;
		return result;
		
	} else if constexpr (Dim == 4) {
		
		auto coef00 = _mat[2][2] * _mat[3][3] - _mat[3][2] * _mat[2][3];
		auto coef02 = _mat[1][2] * _mat[3][3] - _mat[3][2] * _mat[1][3];
		auto coef03 = _mat[1][2] * _mat[2][3] - _mat[2][2] * _mat[1][3];
		
		auto coef04 = _mat[2][1] * _mat[3][3] - _mat[3][1] * _mat[2][3];
		auto coef06 = _mat[1][1] * _mat[3][3] - _mat[3][1] * _mat[1][3];
		auto coef07 = _mat[1][1] * _mat[2][3] - _mat[2][1] * _mat[1][3];
		
		auto coef08 = _mat[2][1] * _mat[3][2] - _mat[3][1] * _mat[2][2];
		auto coef10 = _mat[1][1] * _mat[3][2] - _mat[3][1] * _mat[1][2];
		auto coef11 = _mat[1][1] * _mat[2][2] - _mat[2][1] * _mat[1][2];
		
		auto coef12 = _mat[2][0] * _mat[3][3] - _mat[3][0] * _mat[2][3];
		auto coef14 = _mat[1][0] * _mat[3][3] - _mat[3][0] * _mat[1][3];
		auto coef15 = _mat[1][0] * _mat[2][3] - _mat[2][0] * _mat[1][3];
		
		auto coef16 = _mat[2][0] * _mat[3][2] - _mat[3][0] * _mat[2][2];
		auto coef18 = _mat[1][0] * _mat[3][2] - _mat[3][0] * _mat[1][2];
		auto coef19 = _mat[1][0] * _mat[2][2] - _mat[2][0] * _mat[1][2];
		
		auto coef20 = _mat[2][0] * _mat[3][1] - _mat[3][0] * _mat[2][1];
		auto coef22 = _mat[1][0] * _mat[3][1] - _mat[3][0] * _mat[1][1];
		auto coef23 = _mat[1][0] * _mat[2][1] - _mat[2][0] * _mat[1][1];
		
		auto fac0 = vec<4, Prec>{coef00, coef00, coef02, coef03};
		auto fac1 = vec<4, Prec>{coef04, coef04, coef06, coef07};
		auto fac2 = vec<4, Prec>{coef08, coef08, coef10, coef11};
		auto fac3 = vec<4, Prec>{coef12, coef12, coef14, coef15};
		auto fac4 = vec<4, Prec>{coef16, coef16, coef18, coef19};
		auto fac5 = vec<4, Prec>{coef20, coef20, coef22, coef23};
		
		auto vec0 = vec<4, Prec>{_mat[1][0], _mat[0][0], _mat[0][0], _mat[0][0]};
		auto vec1 = vec<4, Prec>{_mat[1][1], _mat[0][1], _mat[0][1], _mat[0][1]};
		auto vec2 = vec<4, Prec>{_mat[1][2], _mat[0][2], _mat[0][2], _mat[0][2]};
		auto vec3 = vec<4, Prec>{_mat[1][3], _mat[0][3], _mat[0][3], _mat[0][3]};
		
		auto inv0 = vec1 * fac0 - vec2 * fac1 + vec3 * fac2;
		auto inv1 = vec0 * fac0 - vec2 * fac3 + vec3 * fac4;
		auto inv2 = vec0 * fac1 - vec1 * fac3 + vec3 * fac5;
		auto inv3 = vec0 * fac2 - vec1 * fac4 + vec2 * fac5;
		
		auto signA = vec<4, Prec>{Prec( 1), Prec(-1), Prec( 1), Prec(-1)};
		auto signB = vec<4, Prec>{Prec(-1), Prec( 1), Prec(-1), Prec( 1)};
		auto inverse = transpose(mat<4, Prec>{inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB});
		
		auto row0 = vec<4, Prec>{inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]};
		
		auto dot0 = _mat[0] * row0;
		auto dot1 = (dot0.x() + dot0.y()) + (dot0.z() + dot0.w());
		
		auto oneOverDet = Prec(1) / dot1;
		
		return inverse * oneOverDet;
		
	}
	
}

template<floating_point Prec>
constexpr auto look(vec<3, Prec> _pos, vec<3, Prec> _dir, vec<3, Prec> _up) -> mat<4, Prec> {
	
	ASSUME(isUnit(_dir));
	ASSUME(isUnit(_up));
	
	auto result = mat<4, Prec>::identity();
	
	auto s = normalize(cross(_up, _dir));
	auto u = cross(_dir, s);
	result[0] = float4(s,    -dot(s,    _pos));
	result[1] = float4(u,    -dot(u,    _pos));
	result[2] = float4(_dir, -dot(_dir, _pos));
	return result;
	
}

template<floating_point Prec>
constexpr auto perspective(Prec _vFov, Prec _aspectRatio, Prec _zNear) -> mat<4, Prec> {
	
	auto range = tan(_vFov / Prec(2)) * _zNear;
	auto left = -range * _aspectRatio;
	auto right = range * _aspectRatio;
	auto bottom = -range;
	auto top = range;
	
	auto result = mat<4, Prec>();
	result.fill(0);
	result.at(0, 0) = (Prec(2) * _zNear) / (right - left);
	result.at(1, 1) = (Prec(2) * _zNear) / (top - bottom);
	result.at(2, 3) = Prec(2) * _zNear;
	result.at(3, 2) = Prec(1);
	return result;
	
}

}
