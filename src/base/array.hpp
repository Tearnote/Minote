/**
 * Statically allocated variable-size array
 * @file
 * Keeps track of the number of elements. Uses a standard array as a backing
 * store.
 */

#pragma once

#include <cstdlib>
#include <array>
#include "base/util.hpp"

namespace minote {

// Bringing the non-resizeable array into scope
using std::array;

template<typename T, size_t N>
struct varray {

	using Element = T;
	static constexpr size_t Capacity = N;

	array<Element, Capacity> buffer = {}; ///< Array containing the elements
	size_t _size = 0; ///< Number of elements currently in #data

	/**
	 * Add a new element at the end and return a pointer to it.
	 * @return pointer to newly added element, or nullptr if backing store full
	 */
	auto produce() -> Element*;

	/**
	 * Remove an element at a given index. Other elements are shifted to fill
     * the gap, which makes this an O(n) operation that should be avoided
     * for any large arrays.
	 * @param index Index of the element to remove
	 */
	void remove(size_t index);

	/**
	 * Remove an element at a given index and put the last element in its place.
     * This removal is an O(1) operation, at the cost of changing the ordering
     * of remaining elements.
	 * @param index
	 */
	void removeSwap(size_t index);

	/**
	 * Clear the array by setting the number of elements to zero.
	 */
	void clear();

	/**
	 * Size getter, for compatibility with std::array.
	 * @return Number of elements currently in the array
	 */
	[[nodiscard]]
	auto size() const -> size_t { return _size; }

	/**
	 * Retrieve the raw data pointer. Equivalent to std::array.data().
	 * @return Pointer to the internal data buffer
	 */
	auto data() -> Element*;
	auto data() const -> Element const*;

	auto operator[](size_t index) -> Element&;
	auto operator[](size_t index) const -> Element const&;

};

/// Array type providing size, data() and operator[].
template<template<typename, size_t> typename T, typename Type, size_t N>
concept ArrayContainer = std::is_same_v<T<Type, N>, array<Type, N>> ||
    std::is_same_v<T<Type, N>, varray<Type, N>>;

}

#include "array.tpp"
