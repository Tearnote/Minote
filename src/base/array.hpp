// Minote - base/array.hpp
// Standard array types. Includes std::array and varray, a static array with
// a variable number of elements up to a bound.

#pragma once

#include <array>
#include "base/util.hpp"

namespace minote {

using std::array;

template<typename T, size_t N>
struct varray {

	// Type of the varray element
	using Element = T;

	// Maximum number of elements the varray can hold
	static constexpr size_t Capacity = N;

	// Backing store array containing the elements
	array<Element, Capacity> buffer = {};

	// Number of elements currently in the varray
	size_t _size = 0;

	// Add a new element at the end and return a pointer to it, or nullptr
	// if the backing store is full. The returned memory is initialized to zero.
	auto produce() -> Element*;

	// Remove an element at a given index. Other elements are shifted to fill
    // the gap, which makes this O(n) and should be avoided for large arrays.
	void remove(size_t index);

	// Remove an element at a given index and put the last element in its place.
    // This removal is O(1), at the cost of arbitrarily changing the order
    // of remaining elements.
	void removeSwap(size_t index);

	// Clear the array by setting the number of elements to zero.
	void clear();

	// _size getter, for API compatibility with std::array.
	[[nodiscard]]
	auto size() const -> size_t { return _size; }

	// Retrieve the pointer to the internal data buffer, equivalent
	// to std::array.data().
	auto data() -> Element*;
	auto data() const -> Element const*;

	// Direct member access. Bounds checking is performed in debug mode.
	auto operator[](size_t index) -> Element&;
	auto operator[](size_t index) const -> Element const&;

};

// Array type providing size(), data() and operator[]
template<template<typename, size_t> typename T, typename Type, size_t N>
concept ArrayContainer = std::is_same_v<T<Type, N>, array<Type, N>> ||
    std::is_same_v<T<Type, N>, varray<Type, N>>;

}

#include "array.tpp"
